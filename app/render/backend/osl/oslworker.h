#ifndef OSLWORKER_H
#define OSLWORKER_H

#include "../videorenderworker.h"

class OSLWorker : public VideoRenderWorker
{
  Q_OBJECT
public:
  OSLWorker(VideoRenderFrameCache* frame_cache,
            QObject* parent = nullptr);

protected:
  virtual void FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params) override;

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) override;

};

#endif // OSLWORKER_H
