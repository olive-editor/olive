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

#ifndef TASK_H
#define TASK_H

#include <memory>
#include <QObject>

#include "common/cancelableobject.h"

/**
 * @brief A base class for background tasks running in Olive.
 *
 * Tasks are multithreaded by design (i.e. they will always spawn
 * a new thread and run in it).
 *
 * To subclass your own Task, override Action() and return TRUE on success or FALSE on failure. Note that a Task can
 * provide a "negative" output and still have succeeded. For example, the ProbeTask's role is to determine whether a
 * certain media file can be used in Olive. Even if the probe *fails* to find a Decoder for this file, the Task itself
 * has *succeeded* at discovering this. A failure of ProbeTask would indicate a catastrophic failure meaning it was
 * unable to determine anything about the file.
 *
 * Tasks should be used with the TaskManager which will manage starting and deleting them. It'll also only start as
 * many Tasks as there are threads on the system as to not overload them.
 *
 * Tasks support "dependency tasks", i.e. a Task that should be complete before another Task begins.
 */
class Task : public QObject, public CancelableObject
{
  Q_OBJECT
public:
  /**
   * @brief Task Constructor
   */
  Task();

  /**
   * @brief Retrieve the current title of this Task
   */
  const QString& GetTitle();

public slots:
  /**
   * @brief Try to start this Task
   *
   * The main function for starting this Task. If this task is currently waiting, this function will start a new thread
   * and set the status to kWorking.
   *
   * This function also checks its dependency Tasks and will only start if all of them are complete. If they are still
   * working, this function will return FALSE and the status will continue to be kWaiting. If any of them failed, this
   * Task will also fail - this function will return FALSE and the status will be set to kError.
   */
  void Start();

  /**
   * @brief Cancel the Task
   *
   * Sends a signal to the Task to stop as soon as possible. Always call this directly or connect with
   * Qt::DirectConnection, or else it'll be queued *after* the task has already finished.
   */
  void Cancel();

protected:
  /**
   * @brief The main Task function which is run in a separate thread
   *
   * Action() is the function that gets called once the separate thread has been created. This function should be
   * overridden in subclasses.
   *
   * It's also recommended to emit ProgressChanged() throughout your Action() so that any attached ProgressBars can
   * show accurate progress information.
   *
   * @return
   *
   * TRUE if the Task could complete successfully. FALSE if not. Note that FALSE should only be returned if the Task
   * could not finish, not if the Task found a negative result (see Task documentation for details). Before returning
   * FALSE, it's recommended to use set_error() to signal to the user what caused the failure.
   */
  virtual void Action() = 0;

  /**
   * @brief Set the error message
   *
   * It is recommended to use this if your Action() function ever returns FALSE to tell the user why the failure
   * occurred.
   */
  void SetErrorText(const QString& s);

  /**
   * @brief Set the Task title
   *
   * Used in the UI Task Manager to distinguish Tasks from each other. Generally this should be set in the constructor
   * and shouldn't need to change during the life of the Task. To show an error message, it's recommended to use
   * set_error() instead.
   */
  void SetTitle(const QString& s);

signals:
  /**
   * @brief Signal emitted whenever progress is made
   *
   * Emit this throughout Action() to update any attached ProgressBars on the progress of this Task.
   *
   * @param p
   *
   * A value (percentage) between 0 and 100.
   */
  void ProgressChanged(int p);

  void Succeeded();

  void Failed(const QString& error);

  void Finished();

  /**
   * @brief Signal emitted when this Task is removed from TaskManager
   */
  void Removed();

private:
  QString title_;

  QString error_;

};

#endif // TASK_H
