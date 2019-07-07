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

  /**
   * @brief Deleted copy constructor
   */
  TaskManager(const TaskManager& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  TaskManager(TaskManager&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  TaskManager& operator=(const TaskManager& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  TaskManager& operator=(TaskManager&& other) = delete;

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
  void AddTask(TaskPtr t);

  /**
   * @brief Forcibly cancel all commands and clear them
   */
  void Clear();

  /**
   * @brief Undoable command for adding a Task to the TaskManager
   */
  class AddTaskCommand : public QUndoCommand {
  public:
    AddTaskCommand(TaskPtr t, QUndoCommand* parent = nullptr);

    /**
     * @brief Adds the Task to the TaskManager
     *
     * If there are available threads, TaskManager will start running it.
     */
    virtual void redo() override;

    /**
     * @brief Undoes adding the Task
     *
     * If the Task is running, it is cancelled. Then the Task is removed from the TaskManager and the Task's state is
     * reset.
     */
    virtual void undo() override;

  private:
    TaskPtr task_;
  };

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

  /**
   * @brief Internal task array
   */
  QVector<TaskPtr> tasks_;

  /**
   * @brief Constant set at run-time of how many Tasks can run concurrently
   *
   * Currently set in the Constructor to QThread::idealThreadCount()
   */
  int maximum_task_count_;

private slots:
  /**
   * @brief Callback when a Task's status changes
   *
   * When a Task is added, it's connected to this so TaskManager is alerted whenever a Task's status changes. It can
   * therefore keep track of Tasks starting, completing, or failing.
   *
   * @param status
   *
   * The new status of the Task
   */
  void TaskCallback(Task::Status status);

};

namespace olive {
extern TaskManager task_manager;
}

#endif // TASKMANAGER_H
