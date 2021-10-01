/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "previewaudiodevice.h"

namespace olive {

PreviewAudioDevice::PreviewAudioDevice(int bytes_per_frame, QObject *parent) :
  bytes_per_frame_(bytes_per_frame),
  notify_interval_(0),
  bytes_read_(0)
{
  // These pointers are always valid
  using_ = &internal_buffer_[0];
  pushing_ = &internal_buffer_[1];

  // Default to swap being true because we'll have nothing in the main buffer at first
  swap_requested_ = true;
}

PreviewAudioDevice::~PreviewAudioDevice()
{
  close();
}

bool PreviewAudioDevice::isSequential() const
{
  return true;
}

qint64 PreviewAudioDevice::readData(char *data, qint64 maxSize)
{
  if (swap_requested_) {
    SwapBuffers(kFullLock);
    swap_requested_ = false;
  }

  // This function should NEVER touch the buffer in `pushing_`
  qint64 copy_length = qMin(maxSize, qint64(using_->size()));

  if (copy_length) {
    qint64 new_bytes_read = bytes_read_ + copy_length;

    if (notify_interval_ > 0) {
      if ((bytes_read_ / notify_interval_) != (new_bytes_read / notify_interval_)) {
        emit Notify();
      }
    }

    bytes_read_ = new_bytes_read;

    memcpy(data, using_->constData(), copy_length);
    *using_ = using_->mid(copy_length);
  }

  if (using_->isEmpty() && !SwapBuffers(kTryLock)) {
    // Ask push function to swap if it can. If it can't, we'll catch it next read.
    swap_requested_ = true;
  }

  return copy_length;
}

qint64 PreviewAudioDevice::writeData(const char *data, qint64 length)
{
  // This function should NEVER touch the buffer in `using_`
  QMutexLocker locker(&lock_);
  pushing_->append(data, length);

  // If swap requested, do this now
  if (swap_requested_) {
    SwapBuffers(kDontLock);
    swap_requested_ = false;
  }

  return length;
}

bool PreviewAudioDevice::SwapBuffers(LockMethod m)
{
  switch (m) {
  case kDontLock:
    break;
  case kTryLock:
    if (!lock_.tryLock()) {
      return false;
    }
    break;
  case kFullLock:
    lock_.lock();
    break;
  }

  std::swap(using_, pushing_);

  if (m != kDontLock) {
    lock_.unlock();
  }

  return true;
}

}
