#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QObject>

#include "common/cancelableobject.h"
#include "common/constructors.h"
#include "node/output/track/track.h"
#include "node/node.h"
#include "decodercache.h"

class RenderWorker : public QObject, public CancelableObject
{
  Q_OBJECT
public:
  RenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

  DISABLE_COPY_MOVE(RenderWorker)

  bool Init();

  bool IsStarted();

public slots:
  void Close();

  void Render(NodeDependency CurrentPath, qint64 job_time);

signals:
  void CompletedCache(NodeDependency dep, NodeValueTable data, qint64 job_time);

  void FootageUnavailable(StreamPtr stream, Decoder::RetrieveState state, const TimeRange& range, const rational& stream_time);

protected:
  virtual bool InitInternal() = 0;

  virtual void CloseInternal() = 0;

  virtual NodeValueTable RenderInternal(const NodeDependency& CurrentPath, const qint64& job_time);

  virtual void RunNodeAccelerated(const Node *node, const TimeRange& range, const NodeValueDatabase &input_params, NodeValueTable* output_params);

  StreamPtr ResolveStreamFromInput(NodeInput* input);
  DecoderPtr ResolveDecoderFromInput(StreamPtr stream);

  virtual FramePtr RetrieveFromDecoder(DecoderPtr decoder, const TimeRange& range) = 0;

  virtual void FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable* table) = 0;

  NodeValueTable ProcessNode(const NodeDependency &dep);

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range) = 0;

  NodeValueTable ProcessInput(const NodeInput* input, const TimeRange &range);

  virtual void ReportUnavailableFootage(StreamPtr stream, Decoder::RetrieveState state, const rational& stream_time);

  const NodeDependency& CurrentPath() const;

private:
  NodeValueDatabase GenerateDatabase(const Node *node, const TimeRange &range);

  bool started_;

  DecoderCache* decoder_cache_;

  NodeDependency path_;

};

#endif // RENDERWORKER_H
