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

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include "imagestream.h"

class VideoStream : public ImageStream
{
public:
  VideoStream();

  static const int64_t kEndTimestamp;

  virtual QString description() const override;

  /**
   * @brief Get this video stream's frame rate
   *
   * Used purely for metadata, rendering uses the timebase instead.
   */
  const rational& frame_rate() const;
  void set_frame_rate(const rational& frame_rate);

  int64_t get_closest_timestamp_in_frame_index(const int64_t& ts);
  void clear_frame_index();
  void append_frame_index(const int64_t& ts);
  //bool is_frame_index_empty();
  bool is_frame_index_ready();
  int64_t last_frame_index_timestamp();

  bool load_frame_index(const QString& s);
  bool save_frame_index(const QString& s);

private:
  rational frame_rate_;

  QVector<int64_t> frame_index_;

  QMutex index_access_lock_;

};

using VideoStreamPtr = std::shared_ptr<VideoStream>;

#endif // VIDEOSTREAM_H
