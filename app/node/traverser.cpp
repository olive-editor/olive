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
  QVector<NodeInput*> inputs = node->GetInputsIncludingArrays();

  foreach (NodeInput* input, inputs) {
    if (IsCancelled()) {
      return NodeValueDatabase();
    }

    TimeRange input_time = node->InputTimeAdjustment(input, range);

    database.Insert(input, ProcessInput(input, input_time));
  }

  AddGlobalsToDatabase(database, range);

  return database;
}

NodeValueTable NodeTraverser::ProcessInput(NodeInput* input, const TimeRange& range)
{
  if (input->is_connected()) {
    // Value will equal something from the connected node, follow it
    return GenerateTable(input->get_connected_node(), range);
  } else if (!input->IsArray()) {
    // Push onto the table the value at this time from the input
    QVariant input_value = input->get_value_at_time(range.in());

    NodeValueTable table;
    table.Push(input->data_type(), input_value, input->parentNode());
    return table;
  }

  return NodeValueTable();
}

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const TimeRange& range)
{
  if (n->IsTrack()) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    return GenerateBlockTable(static_cast<const TrackOutput*>(n), range);
  }

  // FIXME: Cache certain values here if we've already processed them before

  // Generate database of input values of node
  NodeValueDatabase database = GenerateDatabase(n, range);

  // By this point, the node should have all the inputs it needs to render correctly
  NodeValueTable table = n->Value(database);

  PostProcessTable(n, range, table);

  return table;
}

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const rational &in, const rational &out)
{
  return GenerateTable(n, TimeRange(in, out));
}

NodeValueTable NodeTraverser::GenerateBlockTable(const TrackOutput *track, const TimeRange &range)
{
  // By default, just follow the in point
  Block* active_block = track->BlockAtTime(range.in());

  NodeValueTable table;

  if (active_block) {
    table = GenerateTable(active_block, range);
  }

  return table;
}

QVariant NodeTraverser::ProcessVideoFootage(StreamPtr stream, const rational &input_time)
{
  Q_UNUSED(stream)
  Q_UNUSED(input_time)

  return QVariant();
}

QVariant NodeTraverser::ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time)
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

void NodeTraverser::AddGlobalsToDatabase(NodeValueDatabase &db, const TimeRange& range)
{
  // Insert global variables
  NodeValueTable global;
  global.Push(NodeParam::kFloat, range.in().toDouble(), nullptr, QStringLiteral("time_in"));
  global.Push(NodeParam::kFloat, range.out().toDouble(), nullptr, QStringLiteral("time_out"));
  db.Insert(QStringLiteral("global"), global);
}

void NodeTraverser::PostProcessTable(const Node *node, const TimeRange &range, NodeValueTable &output_params)
{
  bool got_cached_frame = false;

  // Convert footage to image/sample buffers
  QVariant cached_frame = GetCachedFrame(node, range.in());
  if (!cached_frame.isNull()) {
    output_params.Push(NodeParam::kTexture, cached_frame, node);

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

    if (v.type() == NodeParam::kFootage) {
      StreamPtr s = v.data().value<StreamPtr>();

      if (s) {
        if (s->type() == Stream::kVideo) {
          take_this_value_list = &video_footage_to_retrieve;
        } else if (s->type() == Stream::kAudio) {
          take_this_value_list = &audio_footage_to_retrieve;
        }
      }
    } else if (v.type() == NodeParam::kShaderJob) {
      take_this_value_list = &shader_jobs_to_run;
    } else if (v.type() == NodeParam::kSampleJob) {
      take_this_value_list = &sample_jobs_to_run;
    } else if (v.type() == NodeParam::kGenerateJob) {
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
      StreamPtr stream = v.data().value<StreamPtr>();

      if (stream->footage()->IsValid()) {
        QVariant value = ProcessVideoFootage(stream, range.in());

        if (!value.isNull()) {
          output_params.Push(NodeParam::kTexture, value, node);
        }
      }
    }

    // Run shaders
    foreach (const NodeValue& v, shader_jobs_to_run) {
      QVariant value = ProcessShader(node, range, v.data().value<ShaderJob>());

      if (!value.isNull()) {
        output_params.Push(NodeParam::kTexture, value, node);
      }
    }

    // Run generate jobs
    foreach (const NodeValue& v, generate_jobs_to_run) {
      QVariant value = ProcessFrameGeneration(node, v.data().value<GenerateJob>());

      if (!value.isNull()) {
        output_params.Push(NodeParam::kTexture, value, node);
      }
    }
  }

  // Retrieve audio samples
  foreach (const NodeValue& v, audio_footage_to_retrieve) {
    StreamPtr stream = v.data().value<StreamPtr>();

    if (stream->footage()->IsValid()) {
      QVariant value = ProcessAudioFootage(v.data().value<StreamPtr>(), range);

      if (!value.isNull()) {
        output_params.Push(NodeParam::kSamples, value, node);
      }
    }
  }

  // Run any accelerated shader jobs
  foreach (const NodeValue& v, sample_jobs_to_run) {
    QVariant value = ProcessSamples(node, range, v.data().value<SampleJob>());

    if (!value.isNull()) {
      output_params.Push(NodeParam::kSamples, value, node);
    }
  }
}

}
