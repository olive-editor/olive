/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef CINEFORMSECTION_H
#define CINEFORMSECTION_H

#include <QComboBox>

#include "codecsection.h"

namespace olive {

class CineformSection : public CodecSection
{
  Q_OBJECT
public:
  CineformSection(QWidget *parent = nullptr);

  virtual void AddOpts(EncodingParams* params) override;

  virtual void SetOpts(const EncodingParams *p) override;

private:
  QComboBox *quality_combobox_;

};

}

#endif // CINEFORMSECTION_H
