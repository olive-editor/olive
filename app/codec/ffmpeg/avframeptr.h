#ifndef AVFRAMEPTR_H
#define AVFRAMEPTR_H

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <memory>
#include <QDateTime>

#include "common/constructors.h"
#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class AVFrameWrapper {
public:
  AVFrameWrapper() {
    frame_ = av_frame_alloc();
    birthtime_ = QDateTime::currentMSecsSinceEpoch();
    accessed_ = birthtime_;
  }

  virtual ~AVFrameWrapper() {
    av_frame_free(&frame_);
  }

  DISABLE_COPY_MOVE(AVFrameWrapper)

  inline AVFrame* frame() const {
    return frame_;
  }

  inline const qint64& birthtime() const {
    return birthtime_;
  }

  inline const qint64& last_accessed() const {
    return accessed_;
  }

  void access() {
    accessed_ = QDateTime::currentMSecsSinceEpoch();
  }

private:
  AVFrame* frame_;

  qint64 birthtime_;

  qint64 accessed_;

};

using AVFramePtr = std::shared_ptr<AVFrameWrapper>;

OLIVE_NAMESPACE_EXIT

#endif // AVFRAMEPTR_H
