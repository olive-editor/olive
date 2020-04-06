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

#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QObject>

#include "common/constructors.h"
#include "decodercache.h"
#include "node/node.h"
#include "node/output/track/track.h"
#include "node/traverser.h"

OLIVE_NAMESPACE_ENTER

class RenderWorker : public QObject, public NodeTraverser
{
  Q_OBJECT
public:
  RenderWorker(DecoderCache* decoder_cache, QObject* parent = nullptr);

  bool Init();

  bool IsStarted();

public slots:
  void Close();

  void Render(OLIVE_NAMESPACE::NodeDependency path, qint64 job_time);

signals:
  void CompletedCache(OLIVE_NAMESPACE::NodeDependency dep, OLIVE_NAMESPACE::NodeValueTable data, qint64 job_time);

  void FootageUnavailable(OLIVE_NAMESPACE::StreamPtr stream, OLIVE_NAMESPACE::Decoder::RetrieveState state, const OLIVE_NAMESPACE::TimeRange& range, const OLIVE_NAMESPACE::rational& stream_time);

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

OLIVE_NAMESPACE_EXIT

#endif // RENDERWORKER_H
