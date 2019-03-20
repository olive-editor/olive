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

#ifndef CLIPQUEUE_H
#define CLIPQUEUE_H

extern "C" {
#include <libavformat/avformat.h>
}

#include <QVector>
#include <QMutex>

/**
 * @brief The ClipQueue class
 *
 * A fairly simple wrapper for a QVector and QMutex that cleans up AVFrames automatically when removing them.
 */
class ClipQueue {
public:
  /**
   * @brief ClipQueue Constructor
   */
  ClipQueue();

  /**
   * @brief ClipQueue Destructor
   *
   * Automatically clears queue freeing any memory consumed by any AVFrames
   */
  ~ClipQueue();

  // Thread safety (QMutex compatible)
  /**
   * @brief Lock queue mutex
   *
   * Used for multithreading to ensure queue is only accessed by one thread at a time. See QMutex::lock() for more
   * information.
   */
  void lock();

  /**
   * @brief Try to lock queue mutex
   *
   * Used for multithreading to ensure queue is only accessed by one thread at a time. Tries to lock, but doesn't block
   * the calling thread and wait if it can't lock it. See QMutex::tryLock() for more information.
   *
   * @return
   *
   * **TRUE** if the lock succeeded, **FALSE** if not.
   */
  bool tryLock();

  /**
   * @brief Unlock queue mutex
   *
   * Used for multithreading to ensure queue is only accessed by one thread at a time. See QMutex::unlock() for more
   * information.
   */
  void unlock();

  // Array handling (QVector compatible)
  /**
   * @brief Add a frame to the end of the queue
   *
   * @param frame
   *
   * The frame to add
   */
  void append(AVFrame* frame);

  /**
   * @brief Retrieve a frame at a certain index
   *
   * @param i
   *
   * Index to retrieve frame from
   *
   * @return
   *
   * AVFrame at this index
   */
  AVFrame* at(int i);

  /**
   * @brief Retrieve first frame in the queue
   *
   * @return
   *
   * The first AVFrame in the queue
   */
  AVFrame* first();

  /**
   * @brief Retrieve last frame in the queue
   *
   * @return
   *
   * The last AVFrame in the queue
   */
  AVFrame* last();

  /**
   * @brief Remove first frame in the queue
   *
   * Frees all memory occupied by this frame and removes it from the queue
   */
  void removeFirst();

  /**
   * @brief Remove last frame in the queue
   *
   * Frees all memory occupied by this frame and removes it from the queue
   */
  void removeLast();

  /**
   * @brief Remove frame in the queue at a certain index
   *
   * Frees all memory occupied by this frame and removes it from the queue
   *
   * @param i
   *
   * Index to remove a frame at
   */
  void removeAt(int i);

  /**
   * @brief Clear entire queue
   *
   * Frees all memory occupied by all frames and clears the entire queue
   */
  void clear();

  /**
   * @brief Retrieve current size of the queue
   *
   * @return
   *
   * Current the current size of the queue. All indexes in the queue are guaranteed to be valid references to an
   * AVFrame.
   */
  int size();

  /**
   * @brief Returns whether the queue is empty of not.
   *
   * @return
   *
   * **TRUE** if the queue is empty and contains no frames, **FALSE** if not.
   */
  bool isEmpty();

  /**
   * @brief Returns whether the queue contains a frame or not
   *
   * @return
   *
   * **TRUE** if the queue contains the specified frame, **FALSE** if not.
   */
  bool contains(AVFrame* frame);

private:
  QVector<AVFrame*> queue;
  QMutex queue_lock;
};

#endif // CLIPQUEUE_H
