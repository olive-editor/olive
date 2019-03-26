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

#ifndef SILENCEDIALOG_H
#define SILENCEDIALOG_H

#include <QDialog>
#include <QCheckBox>

#include "timeline/clip.h"
#include "ui/labelslider.h"

class AutoCutSilenceDialog : public QDialog
{
  Q_OBJECT
public:
  AutoCutSilenceDialog(QWidget* parent, QVector<int> clips);
public slots:
  virtual int exec() override;
private slots:
  virtual void accept() override;
private:
  void cut_silence();

  QVector<int> clips_;

  LabelSlider* attack_threshold;
  LabelSlider* release_threshold;
  LabelSlider* attack_time;
  LabelSlider* release_time;

  int default_attack_threshold;
  int current_attack_threshold;
  int default_release_threshold;
  int current_release_threshold;
  int default_attack_time;
  int current_attack_time;
  int default_release_time;
  int current_release_time;
};

#endif // SILENCEDIALOG_H
