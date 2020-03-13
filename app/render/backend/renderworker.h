#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QObject>

#include "common/constructors.h"
#include "decodercache.h"
#include "node/node.h"
#include "node/output/track/track.h"
#include "node/traverser.h"

class RenderWorker : public QObject, public NodeTraverser
{
  Q_OBJECT
public:
  RenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

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

  virtual void FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table) = 0;

  virtual void ReportUnavailableFootage(StreamPtr stream, Decoder::RetrieveState state, const rational& stream_time);

  virtual void InputProcessingEvent(NodeInput *input, const TimeRange &input_time, NodeValueTable* table) override;

  virtual void ProcessNodeEvent(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable* output_params) override;

  StreamPtr ResolveStreamFromInput(NodeInput* input);

  DecoderPtr ResolveDecoderFromInput(StreamPtr stream);

  const NodeDependency& CurrentPath() const;

private:
  bool started_;

  DecoderCache* decoder_cache_;

  NodeDependency path_;

};

#endif // RENDERWORKER_H
