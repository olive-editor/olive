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

#ifndef COLORSPACECOMBOBOX_H
#define COLORSPACECOMBOBOX_H

#include <QComboBox>

#include "node/color/colormanager/colormanager.h"


namespace olive {

class ColorSpaceComboBox : public QComboBox
{
  Q_OBJECT
public:
  ColorSpaceComboBox(ColorManager* color_manager, QString categories, bool use_family = true, QWidget* parent = nullptr);

  virtual void showPopup() override;

private:

  ColorManager* color_manager_;//

  QString categories_;

  bool use_family_;

};

}

#endif // COLORSPACECOMBOBOX_H