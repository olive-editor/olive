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

#ifndef MAINSTATUSBAR_H
#define MAINSTATUSBAR_H

#include <QProgressBar>
#include <QStatusBar>

#include "task/taskmanager.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief Shows abbreviated information from a TaskManager object
 */
class MainStatusBar : public QStatusBar
{
  Q_OBJECT
public:
  MainStatusBar(QWidget* parent = nullptr);

  void ConnectTaskManager(TaskManager* manager);

signals:
  void DoubleClicked();

protected:
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;

private slots:
  void UpdateStatus();

private:
  TaskManager* manager_;

  QProgressBar* bar_;

};

OLIVE_NAMESPACE_EXIT

#endif // MAINSTATUSBAR_H
