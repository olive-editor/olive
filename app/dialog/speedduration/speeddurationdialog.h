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

#ifndef SPEEDDURATIONDIALOG_H
#define SPEEDDURATIONDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "undo/undocommand.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/rationalslider.h"

namespace olive {

class SpeedDurationDialog : public QDialog
{
  Q_OBJECT
public:
  explicit SpeedDurationDialog(const QVector<ClipBlock*> &clips, const rational &timebase, QWidget *parent = nullptr);

public slots:
  virtual void accept() override;

signals:

private:
  static rational GetLengthAdjustment(const rational &original_length, double original_speed, double new_speed, const rational &timebase);

  static double GetSpeedAdjustment(double original_speed, const rational &original_length, const rational &new_length);

  QVector<ClipBlock*> clips_;

  FloatSlider *speed_slider_;

  RationalSlider *dur_slider_;

  QCheckBox *link_box_;

  QCheckBox *reverse_box_;

  QCheckBox *maintain_audio_pitch_box_;

  QCheckBox *ripple_box_;

  QComboBox *loop_combo_;

  int start_reverse_;

  int start_maintain_audio_pitch_;

  double start_speed_;

  rational start_duration_;

  int start_loop_;

  rational timebase_;

private slots:
  void SpeedChanged(double s);

  void DurationChanged(const rational &r);

};

}

#endif // SPEEDDURATIONDIALOG_H
