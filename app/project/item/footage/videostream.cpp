/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "videostream.h"

#include <QFile>

const int64_t VideoStream::kEndTimestamp = AV_NOPTS_VALUE;

VideoStream::VideoStream()
{
  set_type(kVideo);
}

QString VideoStream::description() const
{
  return QCoreApplication::translate("Stream", "%1: Video - %2x%3").arg(QString::number(index()),
                                                                        QString::number(width()),
                                                                        QString::number(height()));
}

const rational &VideoStream::frame_rate() const
{
  return frame_rate_;
}

void VideoStream::set_frame_rate(const rational &frame_rate)
{
  frame_rate_ = frame_rate;
}

int64_t VideoStream::get_closest_timestamp_in_frame_index(const int64_t &ts)
{
  QMutexLocker locker(&index_access_lock_);

  if (frame_index_.isEmpty()) {
    return -1;
  }

  if (ts <= 0) {
    return frame_index_.first();
  }

  int index_size = frame_index_.size();

  if (frame_index_.last() == kEndTimestamp) {
    index_size--;
  }

  // Use index to find closest frame in file
  for (int i=0;i<index_size;i++) {
    int64_t this_ts = frame_index_.at(i);

    if (this_ts == ts) {
      return ts;
    } else if (this_ts > ts) {
      return frame_index_.at(i - 1);
    }
  }

  if (frame_index_.last() == kEndTimestamp) {
    // Index is done
    return frame_index_.last();
  } else {
    // Index is not done yet
    return -1;
  }
}

void VideoStream::clear_frame_index()
{
  QMutexLocker locker(&index_access_lock_);

  frame_index_.clear();
}

void VideoStream::append_frame_index(const int64_t &ts)
{
  QMutexLocker locker(&index_access_lock_);

  frame_index_.append(ts);
}

bool VideoStream::is_frame_index_ready()
{
  QMutexLocker locker(&index_access_lock_);

  return !frame_index_.isEmpty() && frame_index_.last() == VideoStream::kEndTimestamp;
}

int64_t VideoStream::last_frame_index_timestamp()
{
  QMutexLocker locker(&index_access_lock_);

  return frame_index_.last();
}

bool VideoStream::load_frame_index(const QString &s)
{
  // Load index from file
  QFile index_file(s);

  if (index_file.exists() && index_file.open(QFile::ReadOnly)) {
    QMutexLocker locker(&index_access_lock_);

    // Resize based on filesize
    frame_index_.resize(static_cast<size_t>(index_file.size()) / sizeof(int64_t));

    // Read frame index into vector
    index_file.read(reinterpret_cast<char*>(frame_index_.data()),
                    index_file.size());

    index_file.close();

    return true;
  }

  return false;
}

bool VideoStream::save_frame_index(const QString &s)
{
  QFile index_file(s);

  if (index_file.open(QFile::WriteOnly)) {
    // Write index in binary
    QMutexLocker locker(&index_access_lock_);

    index_file.write(reinterpret_cast<const char*>(frame_index_.constData()),
                     frame_index_.size() * static_cast<int>(sizeof(int64_t)));

    index_file.close();

    return true;
  }

  return false;
}
