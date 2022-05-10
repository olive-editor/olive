/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "threading/threadticket.h"

#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace olive {

enum class RenderTicketPriority { kHigh = 0, kNormal };

class ThreadPool : public QObject
{
  Q_OBJECT
public:
  using TaskType = RenderTicketPtr;
  ThreadPool(unsigned threads, QObject *parent);

  DISABLE_COPY_MOVE(ThreadPool)

  virtual void RunTicket(RenderTicketPtr ticket) const = 0;
  void AddTicket(RenderTicketPtr ticket, RenderTicketPriority priority = RenderTicketPriority::kNormal);
  bool RemoveTicket(RenderTicketPtr ticket);

  virtual ~ThreadPool() override;

private:
  void thread_exec();

  std::vector<std::thread> worker_threads_;
  std::deque<TaskType> tasks_;
  std::mutex task_mutex_;
  std::condition_variable cond_;
  std::atomic_bool end_threadp_{false};

};

}  // namespace olive

#endif  // THREADPOOL_H
