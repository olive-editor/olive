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
