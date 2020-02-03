#ifndef OPENGLPROCESSOR_H
#define OPENGLPROCESSOR_H

#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "../videorenderworker.h"
#include "openglframebuffer.h"
#include "openglshadercache.h"
#include "opengltexturecache.h"

class OpenGLWorker : public VideoRenderWorker {
  Q_OBJECT
public:
  OpenGLWorker(VideoRenderFrameCache* frame_cache,
               QObject* parent = nullptr);

signals:
  void RequestFrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table);

  void RequestRunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params);

  void RequestTextureToBuffer(const QVariant& texture, QByteArray& buffer);

protected:
  virtual void FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params) override;

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) override;

};

#endif // OPENGLPROCESSOR_H
