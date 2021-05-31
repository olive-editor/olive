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

#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <memory>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QLinkedList>
#include <QMutex>
#include <QTimer>
#include <stdint.h>

#include "common/define.h"

namespace olive {

/**
 * @brief MemoryPool base class
 *
 * A custom memory system that allocates can allocate several objects in a large chunk (as opposed to several small
 * allocations). Improves performance and memory consumption.
 *
 * As a class, this base is usable by setting the template to an object of your choosing. The pool will then allocate
 * `(element_count * sizeof(T))` per arena. Arenas are allocated and destroyed on the fly - when an arena fills up,
 * another is allocated.
 *
 * `Get()` will return an ElementPtr. The original desired data can be accessed through ElementPtr::data(). This data
 * will belong to the caller until ElementPtr goes out of scope and the memory is freed back into the pool.
 */
class MemoryPool : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief Constructor
   * @param element_count
   *
   * Number of elements per arena
   */
  MemoryPool(int element_count)
  {
    element_count_ = element_count;

    clear_timer_ = new QTimer();
    clear_timer_->setInterval(kMaxEmptyArenaLife);
    clear_timer_->moveToThread(qApp->thread());
    connect(clear_timer_, &QTimer::timeout, this, &MemoryPool::ClearEmptyArenas, Qt::DirectConnection);
    QMetaObject::invokeMethod(clear_timer_, "start", Qt::QueuedConnection);
  }

  /**
   * @brief Destructor
   *
   * Deletes all arenas.
   */
  virtual ~MemoryPool()
  {
    Clear();
    clear_timer_->deleteLater();
  }

  DISABLE_COPY_MOVE(MemoryPool)

  /**
   * @brief Clears all arenas, freeing all of their memory
   *
   * Note that this function is not safe, any elements that are still out there will be invalid
   * and accessing them will cause a crash. You'll need to make sure all elements are already
   * relinquished before then.
   */
  void Clear()
  {
    qDeleteAll(arenas_);
    arenas_.clear();
  }

  /**
   * @brief Returns whether any arenas are successfully allocated
   */
  inline bool IsAllocated() const
  {
    return !arenas_.empty();
  }

  /**
   * @brief Returns current number of allocated arenas
   */
  inline int GetArenaCount() const
  {
    return arenas_.size();
  }

  class Arena;

  /**
   * @brief A handle for a chunk of memory in an arena
   *
   * Calling Get() on the pool or arena will return a shared pointer to an element which will contain a pointer to
   * the desired object/data in data(). When Element is destroyed (i.e. when ElementPtr goes out of scope), the memory
   * is released back into the pool so it can be used by another class.
   */
  class Element {
  public:
    /**
     * @brief Element Constructor
     *
     * There is no need to use this outside of the memory pool's internal functions.
     */
    Element(Arena* parent, uint8_t* data)
    {
      parent_ = parent;
      data_ = data;
      accessed_ = QDateTime::currentMSecsSinceEpoch();
    }

    /**
     * @brief Element Destructor
     *
     * Automatically releases this element's memory back to the arena it was retrieved from.
     */
    ~Element()
    {
      release();
    }

    DISABLE_COPY_MOVE(Element)

    /**
     * @brief Access data represented in the pool
     */
    inline uint8_t* data() const
    {
      return data_;
    }

    inline const int64_t& timestamp() const
    {
      return timestamp_;
    }

    inline void set_timestamp(const int64_t& timestamp)
    {
      timestamp_ = timestamp;
    }

    /**
     * @brief Register that this element has been accessed
     *
     * \see last_accessed()
     */
    inline void access()
    {
      accessed_ = QDateTime::currentMSecsSinceEpoch();
    }

    /**
     * @brief Returns the last time `access()` was called on this function
     *
     * Useful for determining the relative age of an element (i.e. if it hasn't been accessed for a certain amount of
     * time, it can probably be freed back into the pool). This requires all usages to call `access()`.
     */
    inline const int64_t& last_accessed() const
    {
      return accessed_;
    }

    void release()
    {
      if (data_) {
        parent_->Release(this);
        data_ = nullptr;
      }
    }

  private:
    Arena* parent_;

    uint8_t* data_;

    int64_t timestamp_;

    int64_t accessed_;

  };

  using ElementPtr = std::shared_ptr<Element>;

  /**
   * @brief A memory pool arena - a subsection of memory
   *
   * The pool itself does not store memory, it stores "arenas". This is so that the pool can handle the situation of
   * an arena becoming full with no more memory to lend. A pool can automatically allocate another arena and continue
   * providing memory (and freeing arenas when they're no longer in use).
   */
  class Arena {
  public:
    Arena(MemoryPool* parent)
    {
      parent_ = parent;
      data_ = nullptr;
      allocated_sz_ = 0;
      empty_time_ = QDateTime::currentMSecsSinceEpoch();
    }

    ~Arena()
    {
      std::list<Element*> copy = lent_elements_;
      foreach (Element* e, copy) {
        e->release();
      }

      delete [] data_;
    }

    DISABLE_COPY_MOVE(Arena)

    /**
     * @brief Returns an element if there is free memory to do so
     */
    ElementPtr Get()
    {
      QMutexLocker locker(&lock_);

      for (int i=0;i<available_.size();i++) {
        if (available_.at(i)) {
          // This buffer is available
          available_.replace(i, false);

          ElementPtr e = std::make_shared<Element>(this,
                                                   reinterpret_cast<uint8_t*>(data_ + i * element_sz_));
          lent_elements_.push_back(e.get());

          return e;
        }
      }

      return nullptr;
    }

    /**
     * @brief Releases an element back into the pool for use elsewhere
     */
    void Release(Element* e)
    {
      QMutexLocker locker(&lock_);
      quintptr diff = reinterpret_cast<quintptr>(e->data()) - reinterpret_cast<quintptr>(data_);

      int index = diff / element_sz_;

      available_.replace(index, true);

      lent_elements_.remove(e);

      if (lent_elements_.empty()) {
        empty_time_ = QDateTime::currentMSecsSinceEpoch();
      }
    }

    int GetUsageCount()
    {
      QMutexLocker locker(&lock_);
      return lent_elements_.size();
    }

    bool Allocate(size_t ele_sz, size_t nb_elements)
    {
      if (IsAllocated()) {
        return true;
      }

      element_sz_ = ele_sz;

      allocated_sz_ = element_sz_ * nb_elements;

      if ((data_ = new uint8_t[allocated_sz_])) {
        available_.resize(nb_elements);
        available_.fill(true);

        return true;
      } else {
        available_.clear();

        return false;
      }
    }

    inline int GetElementCount() const
    {
      return available_.size();
    }

    inline bool IsAllocated() const
    {
      return data_;
    }

    inline qint64 GetTimeArenaWasMadeEmpty()
    {
      QMutexLocker locker(&lock_);
      return empty_time_;
    }

  private:
    MemoryPool* parent_;

    uint8_t* data_;

    size_t allocated_sz_;

    QVector<bool> available_;

    QMutex lock_;

    size_t element_sz_;

    std::list<Element*> lent_elements_;

    qint64 empty_time_;

  };

  /**
   * @brief Retrieves an element from an available arena
   */
  ElementPtr Get()
  {
    QMutexLocker locker(&lock_);

    // Attempt to get an element from an arena
    foreach (Arena* a, arenas_) {
      ElementPtr e = a->Get();

      if (e) {
        return e;
      }
    }

    // All arenas were empty, we'll need to create a new one
    if (arenas_.empty()) {
      qDebug() << "No arenas, creating new...";
    } else {
      qDebug() << "All arenas are full, creating new...";
    }

    size_t ele_sz = GetElementSize();

    if (!ele_sz) {
      qCritical() << "Failed to create arena, element size was 0";
      return nullptr;
    }

    if (element_count_ <= 0) {
      qCritical() << "Failed to create arena, element count was invalid:" << element_count_;
      return nullptr;
    }

    Arena* a = new Arena(this);
    if (!a->Allocate(ele_sz, element_count_)) {
      qCritical() << "Failed to create arena, allocation failed. Out of memory?";
      delete a;
      return nullptr;
    }

    arenas_.push_back(a);
    return a->Get();
  }

protected:
  /**
   * @brief The size of each element
   *
   * Override this to use a custom size (e.g. a char array where T = char but the element size is > 1)
   */
  virtual size_t GetElementSize()
  {
    return sizeof(uint8_t);
  }

private:
  int element_count_;

  std::list<Arena*> arenas_;

  QMutex lock_;

  QTimer *clear_timer_;

  static const qint64 kMaxEmptyArenaLife = 5000;

private slots:
  void ClearEmptyArenas()
  {
    QMutexLocker locker(&lock_);

    const qint64 min_time = QDateTime::currentMSecsSinceEpoch() - kMaxEmptyArenaLife;

    for (auto it=arenas_.begin(); it!=arenas_.end(); ) {
      Arena* arena = (*it);

      if (arena->GetUsageCount() == 0 && arena->GetTimeArenaWasMadeEmpty() <= min_time) {
        qDebug() << "Removing an empty arena";
        delete arena;
        it = arenas_.erase(it);
      } else {
        it++;
      }
    }
  }

};

}

#endif // MEMORYPOOL_H
