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

#ifndef TASK_H
#define TASK_H

#include <memory>
#include <QDateTime>
#include <QObject>

#include "common/cancelableobject.h"

OLIVE_NAMESPACE_ENTER

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
  Task() :
    title_(tr("Task")),
    error_(tr("Unknown error")),
    start_time_(0)
  {
  }

  /**
   * @brief Retrieve the current title of this Task
   */
  const QString& GetTitle() const
  {
    return title_;
  }

  /**
   * @brief Returns the error that occurred if Run() returns false
   */
  const QString& GetError() const
  {
    return error_;
  }

  const qint64& GetStartTime() const
  {
    return start_time_;
  }

public slots:
  /**
   * @brief Run this task
   *
   * @return True if the task completed successfully, false if not.
   *
   * \see GetError() if this returns false.
   */
  bool Start()
  {
    start_time_ = QDateTime::currentMSecsSinceEpoch();

    return Run();
  }

  /**
   * @brief Reset state so that Run() can be called again.
   *
   * Override this if your class holds any persistent state that should be cleared/modified before
   * it's safe for Run() to run again.
   */
  virtual void Reset(){}

  /**
   * @brief Cancel the Task
   *
   * Sends a signal to the Task to stop as soon as possible. Always call this directly or connect
   * with Qt::DirectConnection, or else it'll be queued *after* the task has already finished.
   */
  void Cancel()
  {
    CancelableObject::Cancel();
  }

protected:
  virtual bool Run() = 0;

  /**
   * @brief Set the error message
   *
   * It is recommended to use this if your Action() function ever returns FALSE to tell the user why the failure
   * occurred.
   */
  void SetError(const QString& s)
  {
    error_ = s;
  }

  /**
   * @brief Set the Task title
   *
   * Used in the UI Task Manager to distinguish Tasks from each other. Generally this should be set in the constructor
   * and shouldn't need to change during the life of the Task. To show an error message, it's recommended to use
   * set_error() instead.
   */
  void SetTitle(const QString& s)
  {
    title_ = s;
  }

signals:
  /**
   * @brief Signal emitted whenever progress is made
   *
   * Emit this throughout Action() to update any attached ProgressBars on the progress of this Task.
   *
   * @param p
   *
   * A progress value between 0.0 and 1.0.
   */
  void ProgressChanged(double d);

private:
  QString title_;

  QString error_;

  qint64 start_time_;

};

OLIVE_NAMESPACE_EXIT

#endif // TASK_H
