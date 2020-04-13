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

#include "framepool.h"

OLIVE_NAMESPACE_ENTER

FramePool::FramePool()
{
  data_ = nullptr;
}

bool FramePool::Allocate(int width, int height, PixelFormat::Format format, int nb_elements)
{
  delete [] data_;

  element_sz_ = width * height * PixelFormat::BytesPerPixel(format);

  if ((data_ = new char[element_sz_ * nb_elements])) {
    // Only allocate available array if data allocation succeeded
    available_.resize(nb_elements);
    available_.fill(true);

    return true;
  } else {
    available_.clear();

    return false;
  }
}

void FramePool::Destroy()
{
  delete [] data_;
  available_.clear();
}

FramePool::~FramePool()
{
  delete [] data_;
}

FramePool::ElementPtr FramePool::Get()
{
  for (int i=0;i<available_.size();i++) {
    if (available_.at(i)) {
      // This buffer is available
      available_.replace(i, false);
      return std::make_shared<Element>(this, data_ + i * element_sz_);
    }
  }

  return nullptr;
}

void FramePool::Release(FramePool::Element *e)
{
  quintptr offs = reinterpret_cast<quintptr>(e->data());
  quintptr start = reinterpret_cast<quintptr>(data_);

  quintptr diff = offs - start;

  int index = diff / element_sz_;

  available_.replace(index, true);
}

FramePool::Element::Element(FramePool *parent, char *data)
{
  parent_ = parent;
  data_ = data;
  accessed_ = QDateTime::currentMSecsSinceEpoch();
}

FramePool::Element::~Element()
{
  parent_->Release(this);
}

OLIVE_NAMESPACE_EXIT
