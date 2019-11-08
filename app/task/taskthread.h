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

#ifndef TASKTHREAD_H
#define TASKTHREAD_H

#include <QThread>

class Task;

/**
 * @brief An internal class only used by Task.
 *
 * TaskThread is a simple QThread subclass designed to create a thread and run the
 * Task's Action() function. It also stores the result of Action() which can be read using result() when the thread
 * signals that it has finished().
 */
class TaskThread : public QThread
{
public:
  TaskThread(Task* parent);

  virtual void run() override;

  bool result();

private:
  Task* parent_;

  bool result_ = false;
};

#endif // TASKTHREAD_H
