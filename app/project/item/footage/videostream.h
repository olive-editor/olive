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

  virtual QString description() const override;

  /**
   * @brief Get this video stream's frame rate
   *
   * Used purely for metadata, rendering uses the timebase instead.
   */
  const rational& frame_rate() const;
  void set_frame_rate(const rational& frame_rate);

private:
  rational frame_rate_;

};

using VideoStreamPtr = std::shared_ptr<VideoStream>;

#endif // VIDEOSTREAM_H
