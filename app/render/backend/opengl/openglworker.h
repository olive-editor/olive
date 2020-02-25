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
  OpenGLWorker(VideoRenderFrameCache* frame_cache, DecoderCache *decoder_cache,
               QObject* parent = nullptr);

signals:
  void RequestFrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table);

  void RequestRunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params);

  void RequestTextureToBuffer(const QVariant& texture, void *buffer);

protected:
  virtual void FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params) override;

  virtual void TextureToBuffer(const QVariant& texture, void *buffer) override;

};

#endif // OPENGLPROCESSOR_H
