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

#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <memory>
#include <QDateTime>
#include <stdint.h>

#include <QDebug>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

template <typename T>
class MemoryPool
{
public:
  MemoryPool() {
    data_ = nullptr;
  }

  ~MemoryPool() {
    delete [] data_;
  }

  DISABLE_COPY_MOVE(MemoryPool)

  bool Allocate(int nb_elements) {
    delete [] data_;

    size_t ele_sz = GetElementSize();

    if (!ele_sz) {
      return false;
    }

    if ((data_ = new char[ele_sz * nb_elements])) {
      available_.resize(nb_elements);
      available_.fill(true);

      return true;
    } else {
      available_.clear();

      return false;
    }
  }

  void Destroy() {
    delete [] data_;
    data_ = nullptr;

    available_.clear();
  }

  inline bool IsAllocated() const {
    return data_;
  }

  inline int GetElementCount() const {
    return available_.size();
  }

  class Element {
  public:
    Element(MemoryPool* parent, T* data) {
      parent_ = parent;
      data_ = data;
      accessed_ = QDateTime::currentMSecsSinceEpoch();
    }

    ~Element() {
      parent_->Release(this);
    }

    inline T* data() const {
      return data_;
    }

    inline const int64_t& timestamp() const {
      return timestamp_;
    }

    inline void set_timestamp(const int64_t& timestamp) {
      timestamp_ = timestamp;
    }

    inline void access() {
      accessed_ = QDateTime::currentMSecsSinceEpoch();
    }

    inline const int64_t& last_accessed() const {
      return accessed_;
    }

  private:
    MemoryPool* parent_;

    T* data_;

    int64_t timestamp_;

    int64_t accessed_;

  };

  using ElementPtr = std::shared_ptr<Element>;

  ElementPtr Get() {
    for (int i=0;i<available_.size();i++) {
      if (available_.at(i)) {
        // This buffer is available
        available_.replace(i, false);

        return std::make_shared<Element>(this, reinterpret_cast<T*>(data_ + i * GetElementSize()));
      }
    }

    // FIXME: Allocate a new "arena"
    return nullptr;
  }

  void Release(Element* e) {
    quintptr diff = reinterpret_cast<quintptr>(e->data()) - reinterpret_cast<quintptr>(data_);

    int index = diff / GetElementSize();

    available_.replace(index, true);
  }

protected:
  virtual size_t GetElementSize() {
    return sizeof(T);
  }

private:
  char* data_;

  QVector<bool> available_;

};

OLIVE_NAMESPACE_EXIT

#endif // MEMORYPOOL_H
