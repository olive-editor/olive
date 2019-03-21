/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#ifndef CLIPPROPERTIESDIALOG_H
#define CLIPPROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "timeline/clip.h"
#include "ui/labelslider.h"

/**
 * @brief The ClipPropertiesDialog class
 *
 * A dialog for setting Clip properties, accessible by right clicking a Clip and clicking "Properties".
 */
class ClipPropertiesDialog : public QDialog {
    Q_OBJECT
public:
  /**
   * @brief ClipPropertiesDialog Constructor
   * @param parent
   *
   * Parent widget.
   *
   * @param clips
   *
   * Array of Clip objects to set the properties of.
   */
  ClipPropertiesDialog(QWidget* parent, QVector<Clip*> clips);
protected:
  /**
   * @brief Accept override. Saves the current properties to the array of Clips.
   */
  virtual void accept() override;
private:
  /**
   * @brief Internal clip array (set in the constructor)
   */
  QVector<Clip*> clips_;

  /**
   * @brief Widget for setting the Clip names
   */
  QLineEdit* clip_name_field_;

  /**
   * @brief Widget for setting the Clip durations
   */
  LabelSlider* duration_field_;
};

#endif // CLIPPROPERTIESDIALOG_H
