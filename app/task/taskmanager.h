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

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QtConcurrent/QtConcurrent>
#include <QVector>
#include <QUndoCommand>

#include "task/task.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief An object that manages background Task objects, handling their start and end
 *
 * TaskManager handles the life of a Task object. After a new Task is created, it should be sent to TaskManager through
 * AddTask(). TaskManager will take ownership of the task and add it to a queue until it system resources are available
 * for it to run. Currently, TaskManager will run no more Tasks than there are threads on the system (one task per
 * thread). As Tasks finished, TaskManager will start the next in the queue.
 */
class TaskManager : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief TaskManager Constructor
   */
  TaskManager();

  /**
   * @brief TaskManager Destructor
   *
   * Ensures all Tasks are deleted
   */
  virtual ~TaskManager();

  static void CreateInstance();

  static void DestroyInstance();

  static TaskManager* instance();

  int GetTaskCount() const;

  Task* GetFirstTask() const;

  void CancelTaskAndWait(Task* t);

public slots:
  /**
   * @brief Add a new Task
   *
   * Adds a new Task to the queue. If there are available threads to run it, it'll also run immediately. Otherwise,
   * it'll be placed into the queue and run when resources are available.
   *
   * NOTE: This function is NOT thread-safe and is currently intended to only be used from the main/GUI thread.
   *
   * NOTE: A Task object should only be added once. Adding the same Task object more than once will result in undefined
   * behavior.
   *
   * @param t
   *
   * The task to add and run. TaskManager takes ownership of this Task and will be responsible for freeing it.
   */
  void AddTask(Task *t);

  void CancelTask(Task* t);

signals:
  /**
   * @brief Signal emitted when a Task is added by AddTask()
   *
   * @param t
   *
   * Task that was added
   */
  void TaskAdded(Task* t);

  /**
   * @brief Signal emitted when any change to the running task list has been made
   */
  void TaskListChanged();

  /**
   * @brief Signal emitted when a task is deleted
   */
  void TaskRemoved(Task* t);

  /**
   * @brief Signal emitted when a task fails
   */
  void TaskFailed(Task* t);

private:
  /**
   * @brief Internal task array
   */
  QHash<QFutureWatcher<bool>*, Task*> tasks_;

  /**
   * @brief Internal list of failed tasks
   */
  QLinkedList<Task*> failed_tasks_;

  /**
   * @brief Task thread pool
   */
  QThreadPool thread_pool_;

  /**
   * @brief TaskManager singleton instance
   */
  static TaskManager* instance_;

private slots:
  void TaskFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // TASKMANAGER_H
