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

#ifndef AVFRAMEPTR_H
#define AVFRAMEPTR_H

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <memory>
#include <QDateTime>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class AVFrameWrapper {
public:
  AVFrameWrapper() {
    frame_ = av_frame_alloc();
  }

  virtual ~AVFrameWrapper() {
    av_frame_free(&frame_);
  }

  DISABLE_COPY_MOVE(AVFrameWrapper)

  inline AVFrame* frame() const {
    return frame_;
  }

private:
  AVFrame* frame_;

};

using AVFramePtr = std::shared_ptr<AVFrameWrapper>;

OLIVE_NAMESPACE_EXIT

#endif // AVFRAMEPTR_H
