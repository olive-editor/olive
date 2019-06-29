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

#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QScrollArea>
#include <QVBoxLayout>

#include "widget/taskview/taskviewitem.h"

class TaskView : public QScrollArea
{
  Q_OBJECT
public:
  TaskView(QWidget* parent);

public slots:
  void AddTask(Task* t);

private:
  QWidget* central_widget_;
  QVBoxLayout* layout_;
};

#endif // TASKVIEW_H
