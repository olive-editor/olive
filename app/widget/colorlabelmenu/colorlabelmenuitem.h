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

#ifndef COLORLABELMENUITEM_H
#define COLORLABELMENUITEM_H

#include <QLabel>
#include <QWidget>

#include "widget/colorwheel/colorpreviewbox.h"

namespace olive {

class ColorLabelMenuItem : public QWidget
{
public:
  ColorLabelMenuItem(QWidget* parent = nullptr);

  void SetText(const QString& text);

  void SetColor(const Color& color);

private:
  ColorPreviewBox* box_;
  QLabel* label_;

};

}

#endif // COLORLABELMENUITEM_H
