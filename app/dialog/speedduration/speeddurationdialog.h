/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QDialog>

#include "node/block/clip/clip.h"
#include "undo/undocommand.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/rationalslider.h"

namespace olive {

class SpeedDurationDialog : public QDialog
{
  Q_OBJECT
public:
  explicit SpeedDurationDialog(const QVector<ClipBlock*> &clips, const rational &timebase, QWidget *parent = nullptr);

  class SetSpeedCommand : public UndoCommand
  {
  public:
    SetSpeedCommand(ClipBlock *clip, double new_speed) :
      clip_(clip),
      new_speed_(new_speed)
    {}

    virtual void redo() override;

    virtual void undo() override;

    virtual Project *GetRelevantProject() const override
    {
      return clip_->project();
    }

  private:
    ClipBlock *clip_;
    double new_speed_;
    double old_speed_;

  };

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

  QCheckBox *ripple_box_;

  double start_speed_;

  rational start_duration_;

  rational timebase_;

private slots:
  void SpeedChanged(double s);

  void DurationChanged(const rational &r);

};

}

#endif // SPEEDDURATIONDIALOG_H
