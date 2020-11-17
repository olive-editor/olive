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

#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QDialog>

#include "render/color.h"
#include "render/colormanager.h"
#include "render/managedcolor.h"
#include "widget/colorwheel/colorgradientwidget.h"
#include "widget/colorwheel/colorspacechooser.h"
#include "widget/colorwheel/colorvalueswidget.h"
#include "widget/colorwheel/colorwheelwidget.h"

namespace olive {

class ColorDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief ColorDialog Constructor
   *
   * @param color_manager
   *
   * The ColorManager to use for color management. This must be valid.
   *
   * @param start
   *
   * The color to start with. This must be in the color_manager's reference space
   *
   * @param input_cs
   *
   * The input range that the user should see. The start color will be converted to this for UI object.
   *
   * @param parent
   *
   * QWidget parent.
   */
  ColorDialog(ColorManager* color_manager, const ManagedColor &start = Color(1.0f, 1.0f, 1.0f), QWidget* parent = nullptr);

  /**
   * @brief Retrieves the color selected by the user
   *
   * The color is always returned in the ColorManager's reference space (usually scene linear).
   */
  ManagedColor GetSelectedColor() const;

  QString GetColorSpaceInput() const;

  ColorTransform GetColorSpaceOutput() const;

private:
  ColorManager* color_manager_;

  ColorWheelWidget* color_wheel_;

  ColorValuesWidget* color_values_widget_;

  ColorGradientWidget* hsv_value_gradient_;

  ColorProcessorPtr input_to_ref_processor_;

  ColorSpaceChooser* chooser_;

private slots:
  void ColorSpaceChanged(const QString& input, const ColorTransform &output);

};

}

#endif // COLORDIALOG_H
