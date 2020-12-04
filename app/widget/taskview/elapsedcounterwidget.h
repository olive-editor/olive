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

#ifndef ELAPSEDCOUNTERWIDGET_H
#define ELAPSEDCOUNTERWIDGET_H

#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "common/define.h"

namespace olive {

class ElapsedCounterWidget : public QWidget
{
  Q_OBJECT
public:
  ElapsedCounterWidget(QWidget* parent = nullptr);

  void SetProgress(double d);

  void Start();
  void Start(qint64 start_time);

public slots:
  void Stop();

private:
  QLabel* elapsed_lbl_;

  QLabel* remaining_lbl_;

  double last_progress_;

  QTimer elapsed_timer_;

  qint64 start_time_;

private slots:
  void UpdateTimers();

};

}

#endif // ELAPSEDCOUNTERWIDGET_H
