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

#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>

#include "project/item/footage/footage.h"

/**
 * @brief The MediaPropertiesDialog class
 *
 * A dialog for setting properties on Media. This can be loaded from any part of the application provided it's given
 * a valid Media object.
 */
class FootagePropertiesDialog : public QDialog {
  Q_OBJECT
public:
  /**
   * @brief MediaPropertiesDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow or Project panel.
   *
   * @param i
   *
   * Media object to set properties for.
   */
  FootagePropertiesDialog(QWidget *parent, FootagePtr footage);
private:
  /**
   * @brief ComboBox for interlacing setting
   */
  QComboBox* interlacing_box;

  /**
   * @brief Media name text field
   */
  QLineEdit* name_box;

  /**
   * @brief Internal pointer to Media object (set in constructor)
   */
  FootagePtr footage_;

  /**
   * @brief A list widget for listing the tracks in Media
   */
  QListWidget* track_list;

  /**
   * @brief Frame rate to conform to
   */
  QDoubleSpinBox* conform_fr;

  /**
   * @brief Setting for associated/premultiplied alpha
   */
  QCheckBox* premultiply_alpha_setting;

  /**
   * @brief Setting for this media's color space
   */
  QComboBox* input_color_space;
private slots:
  /**
   * @brief Overridden accept function for saving the properties back to the Media class
   */
  void accept();
};

#endif // MEDIAPROPERTIESDIALOG_H
