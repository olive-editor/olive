/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "sharedmemoryregion.h"

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#endif

namespace olive
{
namespace ipc
{

SharedMemoryRegion::SharedMemoryRegion()
	: size_(0)
	, data_(nullptr)
	, mode_(kAttach)
#if defined(Q_OS_WIN)
	, handle_(nullptr)
#else
	, fd_(-1)
#endif
{
}

SharedMemoryRegion::~SharedMemoryRegion()
{
	Close();
}

QString SharedMemoryRegion::MakeKey(qint64 owner_pid, int worker_index)
{
	return QStringLiteral("olive-rw-%1-%2").arg(owner_pid).arg(worker_index);
}

#if defined(Q_OS_WIN)

bool SharedMemoryRegion::Open(const QString &key, size_t size, Mode mode)
{
	Close();

	key_ = key;
	size_ = size;
	mode_ = mode;

	// Windows global mapping names live in the Local\ namespace by default for the session.
	const QString mapping_name = QStringLiteral("Local\\") + key;
	const std::wstring wname = mapping_name.toStdWString();

	if (mode == kCreate) {
		const DWORD size_high =
			static_cast<DWORD>((quint64(size) >> 32) & 0xFFFFFFFF);
		const DWORD size_low = static_cast<DWORD>(quint64(size) & 0xFFFFFFFF);
		handle_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
									 PAGE_READWRITE, size_high, size_low,
									 wname.c_str());
		if (!handle_) {
			error_ = QStringLiteral("CreateFileMapping failed: %1")
						 .arg(GetLastError());
			return false;
		}
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			error_ =
				QStringLiteral("Shared memory key already exists: %1").arg(key);
			CloseHandle(handle_);
			handle_ = nullptr;
			return false;
		}
	} else {
		handle_ = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, wname.c_str());
		if (!handle_) {
			error_ =
				QStringLiteral("OpenFileMapping failed: %1").arg(GetLastError());
			return false;
		}
	}

	data_ = MapViewOfFile(handle_, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (!data_) {
		error_ = QStringLiteral("MapViewOfFile failed: %1").arg(GetLastError());
		CloseHandle(handle_);
		handle_ = nullptr;
		return false;
	}

	if (mode == kCreate) {
		memset(data_, 0, size);
	}
	return true;
}

void SharedMemoryRegion::Close()
{
	if (data_) {
		UnmapViewOfFile(data_);
		data_ = nullptr;
	}
	if (handle_) {
		CloseHandle(handle_);
		handle_ = nullptr;
	}
	size_ = 0;
}

#else // POSIX

bool SharedMemoryRegion::Open(const QString &key, size_t size, Mode mode)
{
	Close();

	key_ = key;
	size_ = size;
	mode_ = mode;

	// POSIX shared memory names must start with a single slash and contain no others.
	shm_name_ = QStringLiteral("/") + QString(key).replace('/', '_');
	const QByteArray name_bytes = shm_name_.toUtf8();

	int oflag = O_RDWR;
	if (mode == kCreate) {
		oflag |= O_CREAT | O_EXCL;
		// Clear any stale segment left by a crashed previous run with the same name.
		shm_unlink(name_bytes.constData());
	}

	fd_ = shm_open(name_bytes.constData(), oflag, 0600);
	if (fd_ < 0) {
		error_ = QStringLiteral("shm_open(%1) failed: %2")
					 .arg(shm_name_, QString::fromUtf8(strerror(errno)));
		return false;
	}

	if (mode == kCreate) {
		if (ftruncate(fd_, off_t(size)) != 0) {
			error_ = QStringLiteral("ftruncate failed: %1")
						 .arg(QString::fromUtf8(strerror(errno)));
			::close(fd_);
			fd_ = -1;
			shm_unlink(name_bytes.constData());
			return false;
		}
	} else {
		// mmap() succeeds even beyond the real segment size and only faults
		// (SIGBUS) on access, so verify the segment is large enough up front.
		struct stat st;
		if (fstat(fd_, &st) != 0) {
			error_ = QStringLiteral("fstat failed: %1")
						 .arg(QString::fromUtf8(strerror(errno)));
			::close(fd_);
			fd_ = -1;
			return false;
		}
		if (st.st_size < off_t(size)) {
			error_ = QStringLiteral(
						 "shared memory segment is %1 bytes, smaller than the requested %2")
						 .arg(qint64(st.st_size))
						 .arg(qint64(size));
			::close(fd_);
			fd_ = -1;
			return false;
		}
	}

	data_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
	if (data_ == MAP_FAILED) {
		error_ = QStringLiteral("mmap failed: %1")
					 .arg(QString::fromUtf8(strerror(errno)));
		data_ = nullptr;
		::close(fd_);
		fd_ = -1;
		if (mode == kCreate) {
			shm_unlink(name_bytes.constData());
		}
		return false;
	}

	if (mode == kCreate) {
		memset(data_, 0, size);
	}
	return true;
}

void SharedMemoryRegion::Close()
{
	if (data_) {
		munmap(data_, size_);
		data_ = nullptr;
	}
	if (fd_ >= 0) {
		::close(fd_);
		fd_ = -1;
	}
	if (mode_ == kCreate && !shm_name_.isEmpty()) {
		// Only the owner unlinks, so the name is freed once both sides have unmapped.
		shm_unlink(shm_name_.toUtf8().constData());
		shm_name_.clear();
	}
	size_ = 0;
}

#endif

} // namespace ipc
} // namespace olive