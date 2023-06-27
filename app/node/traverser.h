/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef NODETRAVERSER_H
#define NODETRAVERSER_H

#include <QVector2D>

#include "codec/decoder.h"
#include "common/cancelableobject.h"
#include "common/qtutils.h"
#include "node/output/track/track.h"
#include "render/job/cachejob.h"
#include "render/cancelatom.h"
#include "render/job/footagejob.h"
#include "render/job/colortransformjob.h"
#include "render/job/footagejob.h"
#include "value.h"

namespace olive {

class NodeTraverser
{
public:
  NodeTraverser();

  NodeValueTable GenerateTable(const Node *n, const TimeRange &range, const Node *next_node = nullptr);

  virtual NodeValueDatabase GenerateDatabase(const Node *node, const TimeRange &range);

  NodeValueRow GenerateRow(NodeValueDatabase *database, const Node *node, const TimeRange &range);
  NodeValueRow GenerateRow(const Node *node, const TimeRange &range);

  NodeValue GenerateRowValue(const Node *node, const QString &input, NodeValueTable *table, const TimeRange &time);
  NodeValue GenerateRowValueElement(const Node *node, const QString &input, int element, NodeValueTable *table, const TimeRange &time);
  int GenerateRowValueElementIndex(const Node::ValueHint &hint, NodeValue::Type preferred_type, const NodeValueTable *table);
  int GenerateRowValueElementIndex(const Node *node, const QString &input, int element, const NodeValueTable *table);

  void Transform(QTransform *transform, const Node *start, const Node *end, const TimeRange &range);

  const VideoParams& GetCacheVideoParams() const
  {
    return video_params_;
  }

  void SetCacheVideoParams(const VideoParams& params)
  {
    video_params_ = params;
  }

  const AudioParams& GetCacheAudioParams() const
  {
    return audio_params_;
  }

  void SetCacheAudioParams(const AudioParams& params)
  {
    audio_params_ = params;
  }

protected:
  NodeValueTable ProcessInput(const Node *node, const QString &input, const TimeRange &range);

  void ProcessInputElement(NodeValueTableArray &array_tbl, const Node *node, const QString &input, int element, const TimeRange &range);

  virtual void ProcessVideoFootage(TexturePtr destination, const FootageJob *stream, const rational &input_time){}

  virtual void ProcessAudioFootage(SampleBuffer &destination, const FootageJob *stream, const TimeRange &input_time){}

  virtual void ProcessShader(TexturePtr destination, const Node *node, const ShaderJob *job){}

  virtual void ProcessColorTransform(TexturePtr destination, const Node *node, const ColorTransformJob *job){}

  virtual void ProcessSamples(SampleBuffer &destination, const Node *node, const TimeRange &range, const SampleJob &job){}

  virtual void ProcessFrameGeneration(TexturePtr destination, const Node *node, const GenerateJob *job){}

  virtual void ConvertToReferenceSpace(TexturePtr destination, TexturePtr source, const QString &input_cs){}

  virtual TexturePtr ProcessVideoCacheJob(const CacheJob *val);

  virtual TexturePtr CreateTexture(const VideoParams &p)
  {
    return CreateDummyTexture(p);
  }

  virtual SampleBuffer CreateSampleBuffer(const AudioParams &params, int sample_count)
  {
    // Return dummy by default
    return SampleBuffer();
  }

  SampleBuffer CreateSampleBuffer(const AudioParams &params, const rational &length)
  {
    if (params.is_valid()) {
      return CreateSampleBuffer(params, params.time_to_samples(length));
    } else {
      return SampleBuffer();
    }
  }

  QVector2D GenerateResolution() const;

  bool IsCancelled()
  {
    return cancel_ && cancel_->IsCancelled();
  }

  bool HeardCancel() const
  {
    return cancel_ && cancel_->HeardCancel();
  }

  CancelAtom *GetCancelPointer() const { return cancel_; }
  void SetCancelPointer(CancelAtom *cancel) { cancel_ = cancel; }

  void ResolveJobs(NodeValue &value);
  void ResolveAudioJobs(NodeValue &value);

  Block *GetCurrentBlock() const
  {
    return block_stack_.empty() ? nullptr : block_stack_.back();
  }

  LoopMode loop_mode() const { return loop_mode_; }

  virtual bool UseCache() const { return false; }

private:
  TexturePtr CreateDummyTexture(const VideoParams &p);

  VideoParams video_params_;

  AudioParams audio_params_;

  CancelAtom *cancel_;

  const Node *transform_start_;
  const Node *transform_now_;
  QTransform *transform_;

  std::list<Block*> block_stack_;

  LoopMode loop_mode_;

  QHash<const Node*, QHash<TimeRange, NodeValueTable> > value_cache_;
  QHash<Texture*, TexturePtr> resolved_texture_cache_;

};

}

#endif // NODETRAVERSER_H
