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

class SpeedDialog : public QDialog
{
  Q_OBJECT
public:
  SpeedDialog(QWidget* parent, QVector<Clip*> clips);

  void run();
private slots:
  void percent_update();
  void duration_update();
  void frame_rate_update();
  void accept();
private:
  QVector<Clip*> clips_;

  LabelSlider* percent;
  LabelSlider* duration;
  LabelSlider* frame_rate;

  QCheckBox* reverse;
  QCheckBox* maintain_pitch;
  QCheckBox* ripple;

  double default_frame_rate;
  double current_frame_rate;
  double current_percent;
  long default_length;
  long current_length;
};

#endif // SPEEDDIALOG_H
