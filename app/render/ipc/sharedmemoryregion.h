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

#ifndef IPC_SHAREDMEMORYREGION_H
#define IPC_SHAREDMEMORYREGION_H

#include <cstddef>
#include <QString>

namespace olive
{
namespace ipc
{

/**
 * @brief A named, fixed-size shared memory segment mapped into the process address space.
 *
 * One process Create()s the segment (owner); the peer process Attach()es to it by the same key.
 * The mapping is a raw contiguous byte range accessible via data() — the IPC ring buffers and frame
 * slot pools are laid out inside it. Nothing here is locked; synchronization is entirely the
 * caller's responsibility via the lock-free structures placed in the mapping.
 *
 * We deliberately use the raw OS primitives (POSIX shm_open + mmap, Windows CreateFileMapping +
 * MapViewOfFile) rather than QSharedMemory: QSharedMemory carries an implicit semaphore and a 1-byte
 * header convention, attaches/detaches with reference counting we don't want, and historically has
 * cross-platform lifetime quirks. For a render pipeline pushing large frames we want a plain mmap.
 */
class SharedMemoryRegion {
public:
	enum Mode {
		/// Create (and own) the segment. Fails if it already exists; unlinks on destruction.
		kCreate,
		/// Attach to a segment created by the peer. Does not unlink on destruction.
		kAttach
	};

	SharedMemoryRegion();
	~SharedMemoryRegion();

	SharedMemoryRegion(const SharedMemoryRegion &) = delete;
	SharedMemoryRegion &operator=(const SharedMemoryRegion &) = delete;

	/**
   * @brief Open the segment identified by `key` with the given `size` in bytes.
   *
   * `key` is a short identifier (no leading slash needed; the platform prefix is added internally).
   * Returns true on success. On failure, error() carries a human-readable reason.
   */
	bool Open(const QString &key, size_t size, Mode mode);

	/**
   * @brief Unmap and (if owner) unlink the segment. Called automatically by the destructor.
   */
	void Close();

	bool IsValid() const
	{
		return data_ != nullptr;
	}

	void *data() const
	{
		return data_;
	}

	size_t size() const
	{
		return size_;
	}

	const QString &key() const
	{
		return key_;
	}

	const QString &error() const
	{
		return error_;
	}

	/**
   * @brief Build a unique segment key for a worker, e.g. "olive-rw-<pid>-<index>".
   *
   * Centralized so the owner and the spawned worker agree on the same name.
   */
	static QString MakeKey(qint64 owner_pid, int worker_index);

private:
	QString key_;
	size_t size_;
	void *data_;
	Mode mode_;
	QString error_;

#if defined(Q_OS_WIN)
	void *handle_; // HANDLE from CreateFileMapping/OpenFileMapping
#else
	int fd_; // file descriptor from shm_open
	QString shm_name_; // the platform-prefixed name actually passed to shm_open
#endif
};

} // namespace ipc
} // namespace olive

#endif // IPC_SHAREDMEMORYREGION_H