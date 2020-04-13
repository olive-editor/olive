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

#ifndef FRAMEPOOL_H
#define FRAMEPOOL_H

#include <QDateTime>
#include <memory>

#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

class FramePool
{
public:
  FramePool();

  ~FramePool();

  bool Allocate(int width, int height, PixelFormat::Format format, int nb_elements);

  void Destroy();

  DISABLE_COPY_MOVE(FramePool)

  class Element {
  public:
    Element(FramePool* parent, char* data);
    ~Element();

    inline char* data() const {
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
    FramePool* parent_;

    char* data_;

    int64_t timestamp_;

    int64_t accessed_;

  };

  using ElementPtr = std::shared_ptr<Element>;

  ElementPtr Get();

  void Release(Element* e);

private:
  char* data_;

  int element_sz_;

  QVector<bool> available_;

};

OLIVE_NAMESPACE_EXIT

#endif // FRAMEPOOL_H
