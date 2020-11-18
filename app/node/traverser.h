/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "codec/decoder.h"
#include "common/cancelableobject.h"
#include "node/output/track/track.h"
#include "project/item/footage/stream.h"
#include "value.h"

namespace olive {

class NodeTraverser : public CancelableObject
{
public:
  NodeTraverser() = default;

  NodeValueTable GenerateTable(const Node *n, const TimeRange &range);
  NodeValueTable GenerateTable(const Node *n, const rational &in, const rational& out);

  NodeValueDatabase GenerateDatabase(const Node *node, const TimeRange &range);

protected:
  NodeValueTable ProcessInput(NodeInput *input, const TimeRange &range);

  virtual NodeValueTable GenerateBlockTable(const TrackOutput *track, const TimeRange& range);

  virtual QVariant ProcessVideoFootage(StreamPtr stream, const rational &input_time);

  virtual QVariant ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time);

  virtual QVariant ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job);

  virtual QVariant ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job);

  virtual QVariant ProcessFrameGeneration(const Node *node, const GenerateJob& job);

  virtual QVariant GetCachedFrame(const Node *node, const rational &time);

  static void AddGlobalsToDatabase(NodeValueDatabase& db, const TimeRange &range);

private:
  void PostProcessTable(const Node *node, const TimeRange &range, NodeValueTable &output_params);

};

}

#endif // NODETRAVERSER_H
