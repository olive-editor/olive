/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef COLORLABELMENU_H
#define COLORLABELMENU_H

#include "colorlabelmenuitem.h"
#include "widget/menu/menu.h"

namespace olive {

class ColorLabelMenu : public Menu
{
  Q_OBJECT
public:
  ColorLabelMenu(QWidget* parent = nullptr);

  virtual void changeEvent(QEvent* event) override;

signals:
  void ColorSelected(int i);

private:
  void Retranslate();

  QVector<ColorLabelMenuItem*> color_items_;

private slots:
  void ActionTriggered();

};

}

#endif // COLORLABELMENU_H
