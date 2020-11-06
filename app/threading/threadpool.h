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

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QThread>

#include "common/cancelableobject.h"
#include "threading/threadticket.h"

OLIVE_NAMESPACE_ENTER

class ThreadPoolThread;

class ThreadPool : public QObject
{
  Q_OBJECT
public:
  ThreadPool(QThread::Priority priority = QThread::InheritPriority, int threads = 0, QObject* parent = nullptr);

  virtual ~ThreadPool() override;

  RenderTicketPtr Queue();

  virtual void RunTicket(RenderTicketPtr ticket) const = 0;

public slots:
  void AddTicket(OLIVE_NAMESPACE::RenderTicketPtr ticket, bool prioritize = false);

private:
  void RunNext();

  QVector<ThreadPoolThread*> all_threads_;

  std::list<ThreadPoolThread*> available_threads_;

  std::list<RenderTicketPtr> ticket_queue_;

private slots:
  void ThreadDone();

};

class ThreadPoolThread : public QThread, public CancelableObject
{
  Q_OBJECT
public:
  ThreadPoolThread(ThreadPool* parent);

  virtual ~ThreadPoolThread() override;

  void RunTicket(RenderTicketPtr ticket);

protected:
  virtual void run() override;

  virtual void CancelEvent() override;

signals:
  void Done();

private:
  ThreadPool* pool_;

  RenderTicketPtr ticket_;

  QMutex mutex_;

  QWaitCondition wait_cond_;

};

OLIVE_NAMESPACE_EXIT

#endif // THREADPOOL_H
