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
#include <QFileInfo>

#include "common/timecodefunctions.h"
#include "common/xmlutils.h"
#include "footage.h"
#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

VideoStream::VideoStream() :
  start_time_(0),
  is_image_sequence_(false),
  is_generating_proxy_(false),
  using_proxy_(0)
{
  set_type(kVideo);
}

void VideoStream::LoadCustomParameters(QXmlStreamReader* reader)
{
  ImageStream::LoadCustomParameters(reader);
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("proxylevel")) {
      int divider = reader->readElementText().toInt();
      QString proxy_filename = generate_proxy_name(divider);

      if (QFileInfo::exists(proxy_filename)) {
        QFile index_file(proxy_filename);

        if (index_file.open(QFile::ReadOnly)) {
          QVector<int64_t> index(index_file.size() / sizeof(int64_t));
          index_file.read(reinterpret_cast<char *>(index.data()), index_file.size());
          index_file.close();
          set_proxy(divider, index);
          qInfo() << "Proxy set for:" << Stream::footage()->filename();
        }
      } else {
        qInfo() << "Proxy missing for:" << Stream::footage()->filename();
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void VideoStream::SaveCustomParameters(QXmlStreamWriter *writer) const
{
  ImageStream::SaveCustomParameters(writer);
  writer->writeTextElement("proxylevel", QString::number(using_proxy_));
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

const int64_t &VideoStream::start_time() const
{
  return start_time_;
}

void VideoStream::set_start_time(const int64_t &start_time)
{
  start_time_ = start_time;
  emit ParametersChanged();
}

bool VideoStream::is_image_sequence() const
{
  return is_image_sequence_;
}

void VideoStream::set_image_sequence(bool e)
{
  is_image_sequence_ = e;
}

bool VideoStream::is_generating_proxy()
{
  QMutexLocker locker(proxy_access_lock());

  return is_generating_proxy_;
}

bool VideoStream::try_start_proxy()
{
  QMutexLocker locker(proxy_access_lock());

  if (is_generating_proxy_) {
    return false;
  }

  is_generating_proxy_ = true;

  return true;
}

int VideoStream::using_proxy()
{
  QMutexLocker locker(proxy_access_lock());

  return using_proxy_;
}

void VideoStream::set_proxy(const int &divider, const QVector<int64_t> &index)
{
  QMutexLocker locker(proxy_access_lock());

  using_proxy_ = divider;
  frame_index_ = index;
  is_generating_proxy_ = false;
}

int64_t VideoStream::get_closest_timestamp_in_frame_index(const rational &time)
{
  // Get rough approximation of what the timestamp would be in this timebase
  int64_t target_ts = Timecode::time_to_timestamp(time, timebase());

  // Find closest actual timebase in the file
  return get_closest_timestamp_in_frame_index(target_ts);
}

int64_t VideoStream::get_closest_timestamp_in_frame_index(int64_t timestamp)
{
  QMutexLocker locker(proxy_access_lock());

  if (!frame_index_.isEmpty()) {
    if (timestamp <= frame_index_.first()) {
      return frame_index_.first();
    } else if (timestamp >= frame_index_.last()) {
      return frame_index_.last();
    } else {
      // Use index to find closest frame in file
      for (int i=1;i<frame_index_.size();i++) {
        int64_t this_ts = frame_index_.at(i);

        if (this_ts == timestamp) {
          return timestamp;
        } else if (this_ts > timestamp) {
          return frame_index_.at(i - 1);
        }
      }
    }
  }

  return -1;
}

QString VideoStream::generate_proxy_name(int divider)
{
  return FileFunctions::GetMediaIndexFilename(
      FileFunctions::GetUniqueFileIdentifier(Stream::footage()->filename()))
      .append(QString::number(footage()->get_first_stream_of_type(kVideo)->index()))
      .append('d')
      .append(QString::number(divider));
}

/*
void VideoStream::clear_frame_index()
{
  {
    QMutexLocker locker(&index_access_lock_);

    frame_index_.clear();
  }

  emit IndexChanged();
}

void VideoStream::append_frame_index(const int64_t &ts)
{
  {
    QMutexLocker locker(&index_access_lock_);

    frame_index_.append(ts);
  }

  emit IndexChanged();
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
    {
      QMutexLocker locker(&index_access_lock_);

      // Resize based on filesize
      frame_index_.resize(static_cast<size_t>(index_file.size()) / sizeof(int64_t));

      // Read frame index into vector
      index_file.read(reinterpret_cast<char*>(frame_index_.data()),
                      index_file.size());
    }

    index_file.close();

    emit IndexChanged();

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
*/

OLIVE_NAMESPACE_EXIT
