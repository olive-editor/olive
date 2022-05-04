/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

ThreadPool::ThreadPool(unsigned threads, QObject *parent)
  : QObject(parent)
{
  if (threads == 0) {
    threads = std::thread::hardware_concurrency();
  }

  for (unsigned i = 0; i < threads; i += 1) {
    worker_threads_.emplace_back(std::bind(&ThreadPool::thread_exec, this));
  }
}

void ThreadPool::AddTicket(RenderTicketPtr ticket, RenderTicketPriority priority) {
  std::lock_guard<std::mutex> lock(task_mutex_);

  if (priority == RenderTicketPriority::kHigh) {
    tasks_.emplace_front(std::move(ticket));
  } else {
    tasks_.emplace_back(std::move(ticket));
  }

  cond_.notify_one();
}

bool ThreadPool::RemoveTicket(RenderTicketPtr ticket)
{
  std::lock_guard<std::mutex> lock(task_mutex_);

  const auto it = std::find(tasks_.begin(), tasks_.end(), ticket);
  if (it == tasks_.end()) {
    return false;
  }

  tasks_.erase(it);
  return true;
}

void ThreadPool::thread_exec() {
  while (true) {
    TaskType task;

    {
      std::unique_lock<std::mutex> lock(task_mutex_);
      cond_.wait(lock, [this]{ return this->end_threadp_ || !this->tasks_.empty(); });

      if (this->end_threadp_ && this->tasks_.empty()) {
        break;
      }

      task = std::move(tasks_.front());
      tasks_.pop_front();
    }

    RunTicket(task);
  }
}

ThreadPool::~ThreadPool()
{
  end_threadp_ = true;
  cond_.notify_all();

  for (auto &e : worker_threads_) {
    e.join();
  }
}

}
