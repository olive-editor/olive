#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QObject>

#include "common/constructors.h"
#include "node/output/track/track.h"
#include "node/node.h"
#include "decodercache.h"

class RenderWorker : public QObject
{
  Q_OBJECT
public:
  RenderWorker(QObject* parent = nullptr);

  DISABLE_COPY_MOVE(RenderWorker)

  bool Init();

  bool IsStarted();

  bool IsAvailable();

public slots:
  void Close();

  void Render(NodeDependency path);

  NodeValueTable RenderAsSibling(NodeDependency dep);

signals:
  void RequestSibling(NodeDependency path);

  void CompletedCache(NodeDependency dep, NodeValueTable data);

protected:
  virtual bool InitInternal() = 0;

  virtual void CloseInternal() = 0;

  virtual NodeValueTable RenderInternal(const NodeDependency& path);

  virtual bool OutputIsAccelerated(Node *output);

  virtual void RunNodeAccelerated(Node *node, const NodeValueDatabase *input_params, NodeValueTable* output_params);

  StreamPtr ResolveStreamFromInput(NodeInput* input);
  DecoderPtr ResolveDecoderFromInput(StreamPtr stream);

  virtual FramePtr RetrieveFromDecoder(DecoderPtr decoder, const TimeRange& range) = 0;

  virtual void FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table) = 0;

  NodeValueTable ProcessNodeNormally(const NodeDependency &dep);

  virtual NodeValueTable RenderBlock(TrackOutput *track, const TimeRange& range) = 0;

  QAtomicInt working_;

private:
  bool started_;

  DecoderCache decoder_cache_;

};

#endif // RENDERWORKER_H
