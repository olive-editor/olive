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

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QPushButton>

#include "render/colormanager.h"
#include "render/managedcolor.h"

OLIVE_NAMESPACE_ENTER

class ColorButton : public QPushButton
{
  Q_OBJECT
public:
  ColorButton(ColorManager* color_manager, QWidget* parent = nullptr);

  const ManagedColor& GetColor() const;

public slots:
  void SetColor(const ManagedColor& c);

signals:
  void ColorChanged(const ManagedColor& c);

private slots:
  void ShowColorDialog();

private:
  void UpdateColor();

  ColorManager* color_manager_;

  ManagedColor color_;

  ColorProcessorPtr color_processor_;

};

OLIVE_NAMESPACE_EXIT

#endif // COLORBUTTON_H
