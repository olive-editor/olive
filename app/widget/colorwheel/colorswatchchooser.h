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

#ifndef COLORSWATCHCHOOSER_H
#define COLORSWATCHCHOOSER_H

#include "node/color/colormanager/colormanager.h"
#include "widget/colorbutton/colorbutton.h"

namespace olive {

class ColorSwatchChooser : public QWidget
{
  Q_OBJECT
public:
  ColorSwatchChooser(ColorManager *manager, QWidget *parent = nullptr);

public slots:
  void SetCurrentColor(const ManagedColor &c)
  {
    current_ = c;
  }

signals:
  void ColorClicked(const ManagedColor &c);

private:
  void SetDefaultColor(int index);

  static QString GetSwatchFilename();

  void LoadSwatches();
  void SaveSwatches();

  static const int kRowCount = 4;
  static const int kColCount = 8;
  static const int kBtnCount = kRowCount*kColCount;
  ColorButton *buttons_[kBtnCount];

  ManagedColor current_;
  ColorButton *menu_btn_;

private slots:
  void HandleButtonClick();

  void HandleContextMenu();

  void SaveCurrentColor();

  void ResetMenuButton();

};

}

#endif // COLORSWATCHCHOOSER_H
