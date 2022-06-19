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

#include "threadpool.h"

namespace olive {

ThreadPool::ThreadPool(unsigned threads, QObject *parent) :
  QObject(parent)
{
  if (threads == 0) {
    threads = std::thread::hardware_concurrency();
  }

  for (unsigned i = 0; i < threads; i += 1) {
    worker_threads_.emplace_back(std::bind(&ThreadPool::thread_exec, this, &tasks_, &task_mutex_, &cond_));
  }

  // Make single reserved thread for high priority tasks (usually audio) so they don't get stuck
  // behind a lot of slow tasks
  high_thread_ = std::thread(std::thread(std::bind(&ThreadPool::thread_exec, this, &high_tasks_, &high_mutex_, &high_cond_)));
}

void ThreadPool::AddTicket(RenderTicketPtr ticket, RenderTicketPriority priority)
{
  if (priority == RenderTicketPriority::kHigh) {
    std::lock_guard<std::mutex> lock(high_mutex_);
    high_tasks_.emplace_back(std::move(ticket));
    high_cond_.notify_one();
  } else {
    std::lock_guard<std::mutex> lock(task_mutex_);
    tasks_.emplace_back(std::move(ticket));
    cond_.notify_one();
  }
}

bool ThreadPool::RemoveTicket(RenderTicketPtr ticket)
{
  {
    std::lock_guard<std::mutex> lock(task_mutex_);
    const auto it = std::find(tasks_.begin(), tasks_.end(), ticket);
    if (it != tasks_.end()) {
      tasks_.erase(it);
      return true;
    }
  }

  {
    std::lock_guard<std::mutex> lock(high_mutex_);
    const auto it = std::find(high_tasks_.begin(), high_tasks_.end(), ticket);
    if (it != high_tasks_.end()) {
      high_tasks_.erase(it);
      return true;
    }
  }

  return false;
}

void ThreadPool::thread_exec(std::deque<TaskType> *queue, std::mutex *mutex, std::condition_variable *cond)
{
  while (true) {
    TaskType task;

    {
      std::unique_lock<std::mutex> lock(*mutex);
      cond->wait(lock, [this, queue]{ return this->end_threadp_ || !queue->empty(); });

      if (this->end_threadp_ && queue->empty()) {
        break;
      }

      task = std::move(queue->front());
      queue->pop_front();
    }

    RunTicket(task);
  }
}

ThreadPool::~ThreadPool()
{
  end_threadp_ = true;
  cond_.notify_all();
  high_cond_.notify_all();

  for (auto &e : worker_threads_) {
    e.join();
  }
  high_thread_.join();
}

}
