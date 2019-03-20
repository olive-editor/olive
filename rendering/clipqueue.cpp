/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "clipqueue.h"


ClipQueue::ClipQueue()
{

}

ClipQueue::~ClipQueue()
{
  clear();
}

void ClipQueue::lock()
{
  queue_lock.lock();
}

bool ClipQueue::tryLock()
{
  return queue_lock.tryLock();
}

void ClipQueue::unlock()
{
  queue_lock.unlock();
}

void ClipQueue::append(AVFrame *frame)
{
  queue.append(frame);
}

AVFrame *ClipQueue::at(int i)
{
  return queue.at(i);
}

AVFrame *ClipQueue::first()
{
  return queue.first();
}

AVFrame *ClipQueue::last()
{
  return queue.last();
}

void ClipQueue::removeFirst()
{
  removeAt(0);
}

void ClipQueue::removeLast()
{
  removeAt(queue.size()-1);
}

void ClipQueue::removeAt(int i)
{
  av_frame_free(&queue[i]);
  queue.removeAt(i);
}

void ClipQueue::clear()
{
  while (queue.size() > 0) {
    removeAt(0);
  }
}

int ClipQueue::size()
{
  return queue.size();
}

bool ClipQueue::isEmpty()
{
  return queue.isEmpty();
}

bool ClipQueue::contains(AVFrame *frame)
{
  return queue.contains(frame);
}
