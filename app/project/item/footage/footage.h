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

#ifndef FOOTAGE_H
#define FOOTAGE_H

#include "project/item/item.h"
#include "project/item/footage/audiostream.h"
#include "project/item/footage/videostream.h"

#include <QList>

class Footage : public Item
{
public:
  Footage();

  const QString& filename();
  void set_filename(const QString& s);

  virtual Type type() const override;

private:
  QString filename_;

  QList<VideoStream> video_streams_;
  QList<AudioStream> audio_streams_;
};

#endif // FOOTAGE_H
