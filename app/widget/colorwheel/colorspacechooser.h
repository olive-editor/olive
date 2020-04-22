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

#ifndef COLORSPACECHOOSER_H
#define COLORSPACECHOOSER_H

#include <QComboBox>
#include <QGroupBox>

#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

class ColorSpaceChooser : public QGroupBox
{
  Q_OBJECT
public:
  ColorSpaceChooser(ColorManager* color_manager, bool enable_input_field = true, bool enable_display_fields = true, QWidget* parent = nullptr);

  QString input() const;
  ColorTransform output() const;

  void set_input(const QString& s);
  void set_output(const ColorTransform& out);

signals:
  void InputColorSpaceChanged(const QString& input);

  void OutputColorSpaceChanged(const ColorTransform& out);

  void ColorSpaceChanged(const QString& input, const ColorTransform& out);

private slots:
  void UpdateViews(const QString &display);

private:
  ColorManager* color_manager_;

  QComboBox* input_combobox_;

  QComboBox* display_combobox_;

  QComboBox* view_combobox_;

  QComboBox* look_combobox_;

private slots:
  void ComboBoxChanged();

};

OLIVE_NAMESPACE_EXIT

#endif // COLORSPACECHOOSER_H
