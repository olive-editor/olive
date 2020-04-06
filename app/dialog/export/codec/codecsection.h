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

#ifndef CODECSECTION_H
#define CODECSECTION_H

#include <QWidget>

#include "codec/encoder.h"

OLIVE_NAMESPACE_ENTER

class CodecSection : public QWidget
{
public:
  CodecSection(QWidget* parent = nullptr);

  virtual void AddOpts(EncodingParams* params){}

};

OLIVE_NAMESPACE_EXIT

#endif // CODECSECTION_H
