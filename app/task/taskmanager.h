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

#include <QVector>
#include <QUndoCommand>

#include "common/constructors.h"
#include "task/task.h"

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

  DISABLE_COPY_MOVE(TaskManager)

  static void CreateInstance();

  static void DestroyInstance();

  static TaskManager* instance();

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

signals:
  /**
   * @brief Signal emitted when a Task is added by AddTask()
   *
   * @param t
   *
   * Task that was added
   */
  void TaskAdded(Task* t);

private:
  /**
   * @brief The Status enum
   *
   * All states that a Task can be in. When subclassing, you don't need to set the Task's status as the base class
   * does that automatically.
   */
  enum TaskStatus {
    /// This Task is yet to start
    kWaiting,

    /// This Task is currently running (see Action())
    kWorking,

    /// This Task has completed successfully
    kFinished,

    /// This Task failed and could not complete
    kError
  };

  struct TaskContainer {
    Task* task;
    TaskStatus status;
  };

  struct ThreadContainer {
    QThread* thread;
    bool active;
  };

  /**
   * @brief Scan through the task queue and start any Tasks that are able to start
   *
   * This function is run whenever a Task is added and whenever a Task finishes. It determines how many Tasks are
   * currently running and therefore how many Tasks can be started (if any). It will then start ones that can.
   *
   * This function is aware of "dependency Tasks" and if a Task is waiting but has a dependency that hasn't finished,
   * it will skip to the next one.
   *
   * Like AddTask, this function is NOT thread-safe and currently only intended to be run from the main thread.
   */
  void StartNextWaiting();

  /**
   * @brief Removes the Task from the queue and deletes it
   *
   * Recommended for use after a Task has completed or errorred.
   *
   * @param t
   *
   * Task to delete
   */
  void DeleteTask(Task* t);

  void TaskFinished(Task *task);

  TaskStatus GetTaskStatus(Task* t);

  void SetTaskStatus(Task* t, TaskStatus status);

  /**
   * @brief Internal task array
   */
  QVector<TaskContainer> tasks_;

  /**
   * @brief Background threads to run tasks on
   */
  QVector<ThreadContainer> threads_;

  /**
   * @brief Value for how many threads are currently active
   */
  int active_thread_count_;

  /**
   * @brief TaskManager singleton instance
   */
  static TaskManager* instance_;

private slots:
  void TaskSucceeded();

  void TaskFailed();

};

#endif // TASKMANAGER_H
