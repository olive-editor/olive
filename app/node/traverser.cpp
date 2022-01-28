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

#include "traverser.h"

#include "node.h"
#include "render/job/footagejob.h"
#include "render/rendermanager.h"

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

  return database;
}

NodeValueRow NodeTraverser::GenerateRow(NodeValueDatabase *database, const Node *node, const TimeRange &range)
{
  // Generate row
  NodeValueRow row;
  for (auto it=database->begin(); it!=database->end(); it++) {
    // Get hint for which value should be pulled
    NodeValue value = GenerateRowValue(node, it.key(), &it.value());
    row.insert(it.key(), value);
  }

  return row;
}

NodeValueRow NodeTraverser::GenerateRow(const Node *node, const TimeRange &range)
{
  // Generate database of input values of node
  NodeValueDatabase database = GenerateDatabase(node, range);

  return GenerateRow(&database, node, range);
}

NodeValue NodeTraverser::GenerateRowValue(const Node *node, const QString &input, NodeValueTable *table)
{
  NodeValue value = GenerateRowValueElement(node, input, -1, table);

  if (value.array()) {
    // Resolve each element of array
    QVector<NodeValueTable> tables = value.data().value<QVector<NodeValueTable> >();
    QVector<NodeValue> output(tables.size());

    for (int i=0; i<tables.size(); i++) {
      output[i] = GenerateRowValueElement(node, input, i, &tables[i]);
    }

    value = NodeValue(value.type(), QVariant::fromValue(output), value.source(), value.array(), value.tag());
  }

  return value;
}

NodeValue NodeTraverser::GenerateRowValueElement(const Node *node, const QString &input, int element, NodeValueTable *table)
{
  return GenerateRowValueElement(node->GetValueHintForInput(input, element), node->GetInputDataType(input), table);
}

NodeValue NodeTraverser::GenerateRowValueElement(const Node::ValueHint &hint, NodeValue::Type preferred_type, NodeValueTable *table)
{
  int value_index = GenerateRowValueElementIndex(hint, preferred_type, table);

  if (value_index == -1) {
    // If value was -1, try getting the last value
    value_index = table->Count() - 1;
  }

  if (value_index == -1) {
    // If value is still -1, assume the table is empty and return nothing
    return NodeValue();
  }

  return table->TakeAt(value_index);
}

int NodeTraverser::GenerateRowValueElementIndex(const Node::ValueHint &hint, NodeValue::Type preferred_type, const NodeValueTable *table)
{
  QVector<NodeValue::Type> types = hint.types();

  if (types.isEmpty()) {
    types.append(preferred_type);
  }

  if (hint.index() == -1) {
    // Get most recent value with this type and tag
    return table->GetValueIndex(types, hint.tag());
  } else {
    // Try to find value at this index
    int index = table->Count() - 1 - hint.index();
    int diff = 0;

    while (index + diff < table->Count() && index - diff >= 0) {
      if (index + diff < table->Count() && types.contains(table->at(index + diff).type())) {
        return index + diff;
      }
      if (index - diff >= 0 && types.contains(table->at(index - diff).type())) {
        return index - diff;
      }
      diff++;
    }

    return -1;
  }
}

int NodeTraverser::GenerateRowValueElementIndex(const Node *node, const QString &input, int element, const NodeValueTable *table)
{
  return GenerateRowValueElementIndex(node->GetValueHintForInput(input, element), node->GetInputDataType(input), table);
}

NodeGlobals NodeTraverser::GenerateGlobals(const VideoParams &params, const TimeRange &time)
{
  return NodeGlobals(QVector2D(params.width(), params.height()), params.pixel_aspect_ratio(), time);
}

int NodeTraverser::GetChannelCountFromJob(const GenerateJob &job)
{
  int max_channel_count = 0;

  // Find maximum channel count
  for (auto it=job.GetValues().cbegin(); it!=job.GetValues().cend(); it++) {
    if (it.value().type() == NodeValue::kTexture) {
      TexturePtr tex = it.value().data().value<TexturePtr>();
      if (tex) {
        max_channel_count = qMax(max_channel_count, tex->channel_count());
      }
    }
  }
  if (max_channel_count == 0) {
    max_channel_count = VideoParams::kRGBChannelCount;
  }

  switch (job.GetAlphaChannelRequired()) {
  case GenerateJob::kAlphaForceOn:
    return VideoParams::kRGBAChannelCount;
  case GenerateJob::kAlphaForceOff:
    if (max_channel_count >= 1 && max_channel_count < VideoParams::kRGBChannelCount) {
      return max_channel_count;
    } else {
      return VideoParams::kRGBChannelCount;
    }
  case GenerateJob::kAlphaAuto:
    return max_channel_count;
  }

  // Default fallback, should never get here
  return VideoParams::kRGBAChannelCount;
}

NodeValueTable NodeTraverser::ProcessInput(const Node* node, const QString& input, const TimeRange& range)
{
  // If input is connected, retrieve value directly
  if (node->IsInputConnected(input)) {

    TimeRange adjusted_range = node->InputTimeAdjustment(input, -1, range);

    // Value will equal something from the connected node, follow it
    return GenerateTable(node->GetConnectedOutput(input), node->GetValueHintForInput(input), adjusted_range);

  } else {

    // Store node
    QVariant return_val;
    bool is_array = node->InputIsArray(input);

    if (is_array) {

      // Value is an array, we will return a list of NodeValueTables
      QVector<NodeValueTable> array_tbl(node->InputArraySize(input));

      for (int i=0; i<array_tbl.size(); i++) {
        NodeValueTable& sub_tbl = array_tbl[i];
        TimeRange adjusted_range = node->InputTimeAdjustment(input, i, range);

        if (node->IsInputConnected(input, i)) {
          sub_tbl = GenerateTable(node->GetConnectedOutput(input, i), node->GetValueHintForInput(input, i), adjusted_range);
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
    return_table.Push(node->GetInputDataType(input), return_val, node, is_array);
    return return_table;

  }
}

NodeTraverser::NodeTraverser() :
  cancel_(nullptr)
{
}

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const Node::ValueHint &hint, const TimeRange& range)
{
  const Track* track = dynamic_cast<const Track*>(n);
  if (track) {
    // If the range is not wholly contained in this Block, we'll need to do some extra processing
    return GenerateBlockTable(track, range);
  }

  // FIXME: Cache certain values here if we've already processed them before

  // Generate row for node
  NodeValueDatabase database = GenerateDatabase(n, range);
  NodeValueRow row = GenerateRow(&database, n, range);

  //qDebug() << "FIXME: Implement pre-process of row";

  // Generate output table
  NodeValueTable table = database.Merge();

  // By this point, the node should have all the inputs it needs to render correctly
  n->Value(row, GenerateGlobals(video_params_, range), &table);

  // Post-process table
  PostProcessTable(n, hint, range, table);

  return table;
}

NodeValueTable NodeTraverser::GenerateBlockTable(const Track *track, const TimeRange &range)
{
  // By default, just follow the in point
  Block* active_block = track->BlockAtTime(range.in());

  NodeValueTable table;

  if (active_block) {
    table = GenerateTable(active_block, track->GetValueHintForInput(Track::kBlockInput, track->GetArrayIndexFromBlock(active_block)), Track::TransformRangeForBlock(active_block, range));
  }

  return table;
}

TexturePtr NodeTraverser::ProcessVideoFootage(const FootageJob &stream, const rational &input_time)
{
  Q_UNUSED(input_time)

  // Create dummy texture with footage params
  return CreateDummyTexture(stream.video_params());
}

SampleBufferPtr NodeTraverser::ProcessAudioFootage(const FootageJob& stream, const TimeRange &input_time)
{
  Q_UNUSED(stream)
  Q_UNUSED(input_time)

  return SampleBuffer::Create();
}

TexturePtr NodeTraverser::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(job)

  // Create dummy texture with sequence params
  VideoParams tex_params = video_params_;
  tex_params.set_channel_count(GetChannelCountFromJob(job));
  return CreateDummyTexture(tex_params);
}

SampleBufferPtr NodeTraverser::ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(job)

  return SampleBuffer::Create();
}

TexturePtr NodeTraverser::ProcessFrameGeneration(const Node *node, const GenerateJob &job)
{
  Q_UNUSED(node)
  Q_UNUSED(job)

  // Create dummy texture with sequence params
  VideoParams tex_params = video_params_;
  tex_params.set_channel_count(GetChannelCountFromJob(job));
  return CreateDummyTexture(tex_params);
}

void NodeTraverser::SaveCachedTexture(const QByteArray &hash, TexturePtr texture)
{
  Q_UNUSED(hash)
  Q_UNUSED(texture)
}

TexturePtr NodeTraverser::GetCachedTexture(const QByteArray& hash)
{
  Q_UNUSED(hash)

  return nullptr;
}

QVector2D NodeTraverser::GenerateResolution() const
{
  return QVector2D(video_params_.square_pixel_width(), video_params_.height());
}

void NodeTraverser::PostProcessTable(const Node *node, const Node::ValueHint &hint, const TimeRange &range, NodeValueTable &output_params)
{
  bool got_cached_frame = false;
  QByteArray cached_node_hash;

  // Convert footage to image/sample buffers
  /*if (CanCacheFrames() && node->GetCacheTextures()) {
    // This node is set to cache the result, see if we can retrieved a previously cached version
    cached_node_hash = RenderManager::Hash(node, hint, GetCacheVideoParams(), range.in());

    TexturePtr cached_frame = GetCachedTexture(cached_node_hash);
    if (cached_frame) {
      output_params.Push(NodeValue::kTexture, QVariant::fromValue(cached_frame), node);

      // No more to do here
      got_cached_frame = true;
    }
  }*/

  // Strip out any jobs or footage
  QList<NodeValue> footage_jobs_to_run;
  QList<NodeValue> shader_jobs_to_run;
  QList<NodeValue> sample_jobs_to_run;
  QList<NodeValue> generate_jobs_to_run;

  for (int i=0; i<output_params.Count(); i++) {
    const NodeValue& v = output_params.at(i);
    QList<NodeValue>* take_this_value_list = nullptr;

    if (v.type() == NodeValue::kFootageJob) {
      take_this_value_list = &footage_jobs_to_run;
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
    foreach (const NodeValue& v, footage_jobs_to_run) {
      // Assume this is a VideoStream, we did a type check earlier in the function
      FootageJob job = v.data().value<FootageJob>();

      if (job.type() == Track::kVideo) {
        rational footage_time = Footage::AdjustTimeByLoopMode(range.in(), job.loop_mode(), job.length(), job.video_params().video_type(), job.video_params().frame_rate_as_time_base());

        if (footage_time.isNaN()) {
          // Push dummy texture
          output_params.Push(NodeValue::kTexture, QVariant::fromValue(CreateDummyTexture(job.video_params())), node, v.array(), v.tag());
        } else {
          output_params.Push(NodeValue::kTexture, QVariant::fromValue(ProcessVideoFootage(job, footage_time)), node, v.array(), v.tag());
        }
      }
    }

    // Run shaders
    foreach (const NodeValue& v, shader_jobs_to_run) {
      output_params.Push(NodeValue::kTexture, QVariant::fromValue(ProcessShader(node, range, v.data().value<ShaderJob>())), node, v.array(), v.tag());
    }

    // Run generate jobs
    foreach (const NodeValue& v, generate_jobs_to_run) {
      output_params.Push(NodeValue::kTexture, QVariant::fromValue(ProcessFrameGeneration(node, v.data().value<GenerateJob>())), node, v.array(), v.tag());
    }
  }

  // Retrieve audio samples
  foreach (const NodeValue& v, footage_jobs_to_run) {
    // Assume this is an AudioStream, we did a type check earlier in the function
    FootageJob job = v.data().value<FootageJob>();

    if (job.type() == Track::kAudio) {
      output_params.Push(NodeValue::kSamples, QVariant::fromValue(ProcessAudioFootage(job, range)), node, v.array(), v.tag());
    }
  }

  // Run any accelerated shader jobs
  foreach (const NodeValue& v, sample_jobs_to_run) {
    output_params.Push(NodeValue::kSamples, QVariant::fromValue(ProcessSamples(node, range, v.data().value<SampleJob>())), node, v.array(), v.tag());
  }

  if (CanCacheFrames() && node->GetCacheTextures() && !got_cached_frame) {
    // Save cached texture
    SaveCachedTexture(cached_node_hash, output_params.Get(NodeValue::kTexture).value<TexturePtr>());
  }
}

TexturePtr NodeTraverser::CreateDummyTexture(const VideoParams &p)
{
  return std::make_shared<Texture>(p);
}

}
