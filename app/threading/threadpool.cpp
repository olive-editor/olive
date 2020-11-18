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

#include "threadpool.h"

namespace olive {

ThreadPool::ThreadPool(QThread::Priority priority, int threads, QObject *parent) :
  QObject(parent)
{
  all_threads_.resize(threads ? threads : QThread::idealThreadCount());

  // Create threads
  for (int i=0; i<all_threads_.size(); i++) {
    ThreadPoolThread* t  = new ThreadPoolThread(this);

    // Add to vector of all threads
    all_threads_[i] = t;

    // Append to list of available threads
    available_threads_.push_back(t);

    // Connect done signal
    connect(t, &ThreadPoolThread::Done, this, &ThreadPool::ThreadDone);

    // Start the thread at the given priority
    t->start(priority);
  }
}

ThreadPool::~ThreadPool()
{
  foreach (ThreadPoolThread* thread, all_threads_) {
    thread->Cancel();
    thread->wait();
    delete thread;
  }
}

void ThreadPool::AddTicket(RenderTicketPtr ticket, bool prioritize)
{
  if (prioritize) {
    ticket_queue_.push_front(ticket);
  } else {
    ticket_queue_.push_back(ticket);
  }

  RunNext();
}

void ThreadPool::RunNext()
{
  while (!ticket_queue_.empty() && !available_threads_.empty()) {
    // Run function
    RenderTicketPtr ticket = ticket_queue_.front();
    ticket_queue_.pop_front();

    if (!ticket->WasCancelled()) {
      ThreadPoolThread* thread = available_threads_.front();
      available_threads_.pop_front();

      // Run the ticket in the thread, which actually just calls our virtual function RunTicket
      thread->RunTicket(ticket);
    }
  }
}

void ThreadPool::ThreadDone()
{
  ThreadPoolThread* thread = static_cast<ThreadPoolThread*>(sender());

  available_threads_.push_back(thread);

  RunNext();
}

ThreadPoolThread::ThreadPoolThread(ThreadPool *parent)
{
  pool_ = parent;

  // Ensures mutex is definitely locked by the time the thread is running
  mutex_.lock();
}

ThreadPoolThread::~ThreadPoolThread()
{
  mutex_.unlock();
}

void ThreadPoolThread::RunTicket(RenderTicketPtr ticket)
{
  mutex_.lock();
  ticket_ = ticket;
  wait_cond_.wakeAll();
  mutex_.unlock();
}

void ThreadPoolThread::run()
{
  while (true) {
    wait_cond_.wait(&mutex_);

    if (ticket_) {
      pool_->RunTicket(ticket_);
      ticket_ = nullptr;
    }

    if (IsCancelled()) {
      break;
    } else {
      emit Done();
    }
  }
}

void ThreadPoolThread::CancelEvent()
{
  wait_cond_.wakeAll();
}

}
