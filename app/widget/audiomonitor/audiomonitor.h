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

#ifndef AUDIOMONITORWIDGET_H
#define AUDIOMONITORWIDGET_H

#include <QTimer>
#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class AudioMonitor : public QWidget
{
  Q_OBJECT
public:
  AudioMonitor(QWidget* parent = nullptr);

public slots:
  void SetValues(QVector<double> values);
  void Clear();

protected:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;

private:
  QVector<double> values_;
  QVector<bool> peaked_;

  QTimer clear_timer_;
};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOMONITORWIDGET_H
