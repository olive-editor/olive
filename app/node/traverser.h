/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "node/output/track/track.h"
#include "render/job/footagejob.h"
#include "value.h"

namespace olive {

class NodeTraverser : public CancelableObject
{
public:
  NodeTraverser() = default;

  NodeValueTable GenerateTable(const Node *n, const QString &output, const TimeRange &range);
  NodeValueTable GenerateTable(const NodeOutput& output, const TimeRange &range)
  {
    return GenerateTable(output.node(), output.output(), range);
  }

  NodeValueDatabase GenerateDatabase(const Node *node, const QString &output, const TimeRange &range);

  const VideoParams& GetCacheVideoParams() const
  {
    return video_params_;
  }

  void SetCacheVideoParams(const VideoParams& params)
  {
    video_params_ = params;
  }

  static int GetChannelCountFromJob(const GenerateJob& job);

protected:
  NodeValueTable ProcessInput(const Node *node, const QString &input, const TimeRange &range);

  virtual NodeValueTable GenerateBlockTable(const Track *track, const TimeRange& range);

  virtual QVariant ProcessVideoFootage(const FootageJob &stream, const rational &input_time);

  virtual QVariant ProcessAudioFootage(const FootageJob &stream, const TimeRange &input_time);

  virtual QVariant ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job);

  virtual QVariant ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job);

  virtual QVariant ProcessFrameGeneration(const Node *node, const GenerateJob& job);

  virtual QVariant GetCachedTexture(const QByteArray& hash);

  virtual void SaveCachedTexture(const QByteArray& hash, const QVariant& texture);

  virtual bool CanCacheFrames()
  {
    return false;
  }

  void AddGlobalsToDatabase(NodeValueDatabase& db, const TimeRange &range) const;

  QVector2D GenerateResolution() const;

private:
  void PostProcessTable(const Node *node, const QString &output, const TimeRange &range, NodeValueTable &output_params);

  VideoParams video_params_;

};

}

#endif // NODETRAVERSER_H
