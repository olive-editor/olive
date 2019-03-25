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

#ifndef SPEEDDIALOG_H
#define SPEEDDIALOG_H

#include <QDialog>
#include <QCheckBox>

#include "timeline/clip.h"
#include "ui/labelslider.h"

/**
 * @brief The SpeedDialog class
 *
 * A dialog for setting the speed of one or more Clips. This can be run from anywhere provided it's given a valid
 * array of Clips.
 *
 * It's preferable ot
 */
class SpeedDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief SpeedDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow or Timeline panel.
   *
   * @param clips
   *
   * A valid array of Clips to change the speed of.
   */
  SpeedDialog(QWidget* parent, QVector<Clip*> clips);
public slots:
  /**
   * @brief Override of exec() to set up current Clip speed data just before opening
   *
   * @return
   *
   * The result of QDialog::exec(), a DialogCode result.
   */
  virtual int exec() override;
private slots:
  /**
   * @brief Override of accept() to perform the selected changes on the Clips
   */
  virtual void accept() override;

  /**
   * @brief Slot when the speed percentage field is changed by the user
   *
   * The three fields (percent, duration, and frame rate) all work in tandem to create a speed multipler for the
   * Clip. Each has a slot for when one of the fields changes to update the others appropriately so they all have the
   * same speed multipler.
   */
  void percent_update();

  /**
   * @brief Slot when the duration field is changed by the user
   *
   * The three fields (percent, duration, and frame rate) all work in tandem to create a speed multipler for the
   * Clip. Each has a slot for when one of the fields changes to update the others appropriately so they all have the
   * same speed multipler.
   */
  void duration_update();

  /**
   * @brief Slot when the frame rate field is changed by the user
   *
   * The three fields (percent, duration, and frame rate) all work in tandem to create a speed multipler for the
   * Clip. Each has a slot for when one of the fields changes to update the others appropriately so they all have the
   * same speed multipler.
   */
  void frame_rate_update();
private:
  /**
   * @brief Internal array of Clip objects
   */
  QVector<Clip*> clips_;

  /**
   * @brief Speed percentage field
   */
  LabelSlider* percent;

  /**
   * @brief Duration field
   */
  LabelSlider* duration;

  /**
   * @brief Frame rate field
   */
  LabelSlider* frame_rate;

  /**
   * @brief UI widget for setting the Clip's reverse value
   */
  QCheckBox* reverse;

  /**
   * @brief UI widget for setting the Clip's maintain pitch value
   */
  QCheckBox* maintain_pitch;

  /**
   * @brief UI widget for setting whether to ripple Clips around these changes or not
   */
  QCheckBox* ripple;
};

#endif // SPEEDDIALOG_H
