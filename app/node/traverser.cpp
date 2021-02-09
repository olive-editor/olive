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

#include "traverser.h"

#include "node.h"

namespace olive {

NodeValueDatabase NodeTraverser::GenerateDatabase(const Node* node, const TimeRange &range)
{
  NodeValueDatabase database;

  // We need to insert tables into the database for each input
  foreach (const QString& input, node->inputs()) {
    if (IsCancelled()) {
      return NodeValueDatabase();
    }

    database.Insert(input, ProcessInput(node, input, range));
  }

  AddGlobalsToDatabase(database, range);

  return database;
}

NodeValueTable NodeTraverser::ProcessInput(const Node* node, const QString& input, const TimeRange& range)
{
  // If input is connected, retrieve value directly
  if (node->IsInputConnected(input)) {

    TimeRange adjusted_range = node->InputTimeAdjustment(input, -1, range);

    // Value will equal something from the connected node, follow it
    return GenerateTable(node->GetConnectedOutput(input), adjusted_range);

  } else {

    // Store node
    QVariant return_val;

    if (node->InputIsArray(input)) {

      // Value is an array, we will return a list of NodeValueTables
      QVector<NodeValueTable> array_tbl(node->InputArraySize(input));

      for (int i=0; i<array_tbl.size(); i++) {
        NodeValueTable& sub_tbl = array_tbl[i];
        TimeRange adjusted_range = node->InputTimeAdjustment(input, i, range);

        if (node->IsInputConnected(input, i)) {
          sub_tbl = GenerateTable(node->GetConnectedOutput(input, i), adjusted_range);
        } else {
          QVariant input_value = node->GetValueAtTime(input, adjusted_range.in(), i);
          sub_tbl.Push(node->GetInputDataType(input), input_value, node);
        }
      }

      return_val = QVariant::fromValue(array_tbl);

    } else {

      // Not connected or an array, just pull the immediate
      TimeRange adjusted_range = node->InputTimeAdjustment(input, -1, range);

      return_val = node->GetValueAtTime(input, adjusted_range.in());

    }

    NodeValueTable return_table;
    return_table.Push(node->GetInputDataType(input), return_val, node, true);
    return return_table;

  }
}

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const QString& output, const TimeRange& range)
{
  const Track* track = dynamic_cast<const Track*>(n);
  if (track) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    return GenerateBlockTable(track, range);
  }

  // FIXME: Cache certain values here if we've already processed them before

  // Generate database of input values of node
  NodeValueDatabase database = GenerateDatabase(n, range);

  // By this point, the node should have all the inputs it needs to render correctly
  NodeValueTable table = n->Value(output, database);

  PostProcessTable(n, range, table);

  return table;
}

NodeValueTable NodeTraverser::GenerateBlockTable(const Track *track, const TimeRange &range)
{
  // By default, just follow the in point
  Block* active_block = track->BlockAtTime(range.in());

  NodeValueTable table;

  if (active_block) {
    table = GenerateTable(active_block, Track::TransformRangeForBlock(active_block, range));
  }

  return table;
}

QVariant NodeTraverser::ProcessVideoFootage(VideoStream *stream, const rational &input_time)
{
  Q_UNUSED(stream)
  Q_UNUSED(input_time)

  return QVariant();
}

QVariant NodeTraverser::ProcessAudioFootage(AudioStream *stream, const TimeRange &input_time)
{
  Q_UNUSED(stream)
  Q_UNUSED(input_time)

  return QVariant();
}

QVariant NodeTraverser::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(job)

  return QVariant();
}

QVariant NodeTraverser::ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(job)

  return QVariant();
}

QVariant NodeTraverser::ProcessFrameGeneration(const Node *node, const GenerateJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(job)

  return QVariant();
}

QVariant NodeTraverser::GetCachedFrame(const Node *node, const rational &time)
{
  Q_UNUSED(node)
  Q_UNUSED(time)

  return QVariant();
}

void NodeTraverser::AddGlobalsToDatabase(NodeValueDatabase &db, const TimeRange& range) const
{
  // Insert global variables
  NodeValueTable global;
  global.Push(NodeValue::kFloat, range.in().toDouble(), nullptr, false, QStringLiteral("time_in"));
  global.Push(NodeValue::kFloat, range.out().toDouble(), nullptr, false, QStringLiteral("time_out"));
  global.Push(NodeValue::kVec2, GenerateResolution(), nullptr, false, QStringLiteral("resolution"));

  db.Insert(QStringLiteral("global"), global);
}

void NodeTraverser::PostProcessTable(const Node *node, const TimeRange &range, NodeValueTable &output_params)
{
  bool got_cached_frame = false;

  // Convert footage to image/sample buffers
  QVariant cached_frame = GetCachedFrame(node, range.in());
  if (!cached_frame.isNull()) {
    output_params.Push(NodeValue::kTexture, cached_frame, node);

    // No more to do here
    got_cached_frame = true;
  }

  // Strip out any jobs or footage
  QList<NodeValue> video_footage_to_retrieve;
  QList<NodeValue> audio_footage_to_retrieve;
  QList<NodeValue> shader_jobs_to_run;
  QList<NodeValue> sample_jobs_to_run;
  QList<NodeValue> generate_jobs_to_run;

  for (int i=0; i<output_params.Count(); i++) {
    const NodeValue& v = output_params.at(i);
    QList<NodeValue>* take_this_value_list = nullptr;

    if (v.type() == NodeValue::kFootage) {
      Stream* s = Node::ValueToPtr<Stream>(v.data());

      if (s) {
        if (s->type() == Stream::kVideo) {
          take_this_value_list = &video_footage_to_retrieve;
        } else if (s->type() == Stream::kAudio) {
          take_this_value_list = &audio_footage_to_retrieve;
        }
      }
    } else if (v.type() == NodeValue::kShaderJob) {
      take_this_value_list = &shader_jobs_to_run;
    } else if (v.type() == NodeValue::kSampleJob) {
      take_this_value_list = &sample_jobs_to_run;
    } else if (v.type() == NodeValue::kGenerateJob) {
      take_this_value_list = &generate_jobs_to_run;
    }

    if (take_this_value_list) {
      take_this_value_list->append(output_params.TakeAt(i));
      i--;
    }
  }

  if (!got_cached_frame) {
    // Retrieve video frames
    foreach (const NodeValue& v, video_footage_to_retrieve) {
      // Assume this is a VideoStream, we did a type check earlier in the function
      VideoStream* stream = Node::ValueToPtr<VideoStream>(v.data());

      if (stream->footage()->IsValid()) {
        QVariant value = ProcessVideoFootage(stream, range.in());

        if (!value.isNull()) {
          output_params.Push(NodeValue::kTexture, value, node);
        }
      }
    }

    // Run shaders
    foreach (const NodeValue& v, shader_jobs_to_run) {
      QVariant value = ProcessShader(node, range, v.data().value<ShaderJob>());

      if (!value.isNull()) {
        output_params.Push(NodeValue::kTexture, value, node);
      }
    }

    // Run generate jobs
    foreach (const NodeValue& v, generate_jobs_to_run) {
      QVariant value = ProcessFrameGeneration(node, v.data().value<GenerateJob>());

      if (!value.isNull()) {
        output_params.Push(NodeValue::kTexture, value, node);
      }
    }
  }

  // Retrieve audio samples
  foreach (const NodeValue& v, audio_footage_to_retrieve) {
    // Assume this is an AudioStream, we did a type check earlier in the function
    AudioStream* stream = Node::ValueToPtr<AudioStream>(v.data());

    if (stream->footage()->IsValid()) {
      QVariant value = ProcessAudioFootage(stream, range);

      if (!value.isNull()) {
        output_params.Push(NodeValue::kSamples, value, node);
      }
    }
  }

  // Run any accelerated shader jobs
  foreach (const NodeValue& v, sample_jobs_to_run) {
    QVariant value = ProcessSamples(node, range, v.data().value<SampleJob>());

    if (!value.isNull()) {
      output_params.Push(NodeValue::kSamples, value, node);
    }
  }
}

}
