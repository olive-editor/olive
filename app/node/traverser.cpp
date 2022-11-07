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

#include "traverser.h"

#include "node.h"
#include "node/block/clip/clip.h"
#include "render/job/footagejob.h"
#include "render/rendermanager.h"

namespace olive {

NodeValueDatabase NodeTraverser::GenerateDatabase(const Node* node, const TimeRange &range)
{
  NodeValueDatabase database;

  // HACK: Pick up loop mode from clips
  LoopMode old_loop_mode = loop_mode_;
  if (const ClipBlock *clip = dynamic_cast<const ClipBlock*>(node)) {
    loop_mode_ = clip->loop_mode();
  }

  // We need to insert tables into the database for each input
  auto ignore = node->IgnoreInputsForRendering();
  foreach (const QString& input, node->inputs()) {
    if (IsCancelled()) {
      return NodeValueDatabase();
    }

    if (ignore.contains(input)) {
      continue;
    }

    database.Insert(input, ProcessInput(node, input, range));
  }

  loop_mode_ = old_loop_mode;

  return database;
}

NodeValueRow NodeTraverser::GenerateRow(NodeValueDatabase *database, const Node *node, const TimeRange &range)
{
  // Generate row
  NodeValueRow row;
  for (auto it=database->begin(); it!=database->end(); it++) {
    // Get hint for which value should be pulled
    NodeValue value = GenerateRowValue(node, it.key(), &it.value(), range);
    row.insert(it.key(), value);
  }

  // TEMP: Audio needs to be refactored to work with new job system. But refactoring hasn't been
  //       done yet, so we emulate old behavior here JUST FOR AUDIO.
  for (auto it=row.begin(); it!=row.end(); it++) {
    NodeValue &val = it.value();
    if (val.type() == NodeValue::kSamples) {
      ResolveJobs(val);
    }
  }
  // END TEMP

  return row;
}

NodeValueRow NodeTraverser::GenerateRow(const Node *node, const TimeRange &range)
{
  // Generate database of input values of node
  NodeValueDatabase database = GenerateDatabase(node, range);

  return GenerateRow(&database, node, range);
}

NodeValue NodeTraverser::GenerateRowValue(const Node *node, const QString &input, NodeValueTable *table, const TimeRange &time)
{
  NodeValue value = GenerateRowValueElement(node, input, -1, table, time);

  if (value.array()) {
    // Resolve each element of array
    NodeValueTableArray tables = value.value<NodeValueTableArray>();
    NodeValueArray output;

    for (auto it=tables.begin(); it!=tables.end(); it++) {
      output[it->first] = GenerateRowValueElement(node, input, it->first, &it->second, time);
    }

    value = NodeValue(value.type(), QVariant::fromValue(output), value.source(), value.array(), value.tag());
  }

  return value;
}

NodeValue NodeTraverser::GenerateRowValueElement(const Node *node, const QString &input, int element, NodeValueTable *table, const TimeRange &time)
{
  int value_index = GenerateRowValueElementIndex(node->GetValueHintForInput(input, element), node->GetInputDataType(input), table);

  if (value_index == -1) {
    // If value was -1, try getting the last value
    value_index = table->Count() - 1;
  }

  if (value_index == -1) {
    // If value is still -1, assume the table is empty and return nothing
    return NodeValue();
  }

  NodeValue value = table->TakeAt(value_index);

  if (value.type() == NodeValue::kTexture && UseCache()) {
    if (TexturePtr tex = value.toTexture()) {
      QMutexLocker locker(node->video_frame_cache()->mutex());

      node->video_frame_cache()->LoadState();

      QString cache = node->video_frame_cache()->GetValidCacheFilename(time.in());
      if (!cache.isEmpty()) {
        value.set_value(tex->toJob(CacheJob(cache, value)));
      }
    }
  }

  return value;
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

void NodeTraverser::Transform(QTransform *transform, const Node *start, const Node *end, const TimeRange &range)
{
  transform_ = transform;
  transform_start_ = start;
  transform_now_ = nullptr;

  GenerateTable(end, range);

  transform_ = nullptr;
}

NodeValueTable NodeTraverser::ProcessInput(const Node* node, const QString& input, const TimeRange& range)
{
  // If input is connected, retrieve value directly
  if (node->IsInputConnectedForRender(input)) {

    TimeRange adjusted_range = node->InputTimeAdjustment(input, -1, range, true);

    // Value will equal something from the connected node, follow it
    Node *output = node->GetConnectedRenderOutput(input);
    NodeValueTable table = GenerateTable(output, adjusted_range, node);
    return table;

  } else {

    // Store node
    QVariant return_val;
    bool is_array = node->InputIsArray(input);

    if (is_array) {

      // Value is an array, we will return a list of NodeValueTables
      NodeValueTableArray array_tbl;

      Node::ActiveElements a = node->GetActiveElementsAtTime(input, range);
      if (a.mode() == Node::ActiveElements::kAllElements) {
        int sz = node->InputArraySize(input);
        for (int i=0; i<sz; i++) {
          ProcessInputElement(array_tbl, node, input, i, range);
        }
      } else if (a.mode() == Node::ActiveElements::kSpecified) {
        for (int ele : a.elements()) {
          ProcessInputElement(array_tbl, node, input, ele, range);
        }
      }

      return_val = QVariant::fromValue(array_tbl);

    } else {

      // Not connected or an array, just pull the immediate
      TimeRange adjusted_range = node->InputTimeAdjustment(input, -1, range, true);

      return_val = node->GetValueAtTime(input, adjusted_range.in());

    }

    NodeValueTable return_table;
    return_table.Push(node->GetInputDataType(input), return_val, node, is_array);
    return return_table;

  }
}

void NodeTraverser::ProcessInputElement(NodeValueTableArray &array_tbl, const Node *node, const QString &input, int element, const TimeRange &range)
{
  NodeValueTable& sub_tbl = array_tbl[element];
  TimeRange adjusted_range = node->InputTimeAdjustment(input, element, range, true);

  if (node->IsInputConnectedForRender(input, element)) {
    Node *output = node->GetConnectedRenderOutput(input, element);
    sub_tbl = GenerateTable(output, adjusted_range, node);
  } else {
    QVariant input_value = node->GetValueAtTime(input, adjusted_range.in(), element);
    sub_tbl.Push(node->GetInputDataType(input), input_value, node);
  }
}

NodeTraverser::NodeTraverser() :
  cancel_(nullptr),
  transform_(nullptr),
  loop_mode_(LoopMode::kLoopModeOff)
{
}

class GTTTime
{
public:
  GTTTime(const Node *n) { t = QDateTime::currentMSecsSinceEpoch(); node = n; }

  ~GTTTime() { qDebug() << "GT for" << node << "took" << (QDateTime::currentMSecsSinceEpoch() - t); }

  qint64 t;
  const Node *node;

};

NodeValueTable NodeTraverser::GenerateTable(const Node *n, const TimeRange& range, const Node *next_node)
{
  // NOTE: Times how long a node takes to process, useful for profiling.
  //GTTTime gtt(n);Q_UNUSED(gtt);

  // Use table cache to skip processing where available
  if (value_cache_.contains(n)) {
    QHash<TimeRange, NodeValueTable> &node_value_map = value_cache_[n];
    if (node_value_map.contains(range)) {
      return node_value_map.value(range);
    }
  }

  // Generate row for node
  NodeValueDatabase database = GenerateDatabase(n, range);

  // Check for bypass
  bool is_enabled;
  if (!database[Node::kEnabledInput].Has(NodeValue::kBoolean)) {
    // Fallback if we couldn't find a bool value
    is_enabled = true;
  } else {
    is_enabled = database[Node::kEnabledInput].Get(NodeValue::kBoolean).toBool();
  }

  NodeValueTable table;

  if (is_enabled) {
    NodeValueRow row = GenerateRow(&database, n, range);

    // Generate output table
    table = database.Merge();

    // By this point, the node should have all the inputs it needs to render correctly
    NodeGlobals globals(video_params_, audio_params_, range, loop_mode_);
    n->Value(row, globals, &table);

    // `transform_now_` is the next node in the path that needs to be traversed. It only ever goes
    // "down" the graph so that any traversing going back up doesn't unnecessarily transform
    // from unrelated nodes or the same node twice
    if (transform_) {
      if (transform_now_ == n || transform_start_ == n) {
        if (transform_now_ == n) {
          QTransform t = n->GizmoTransformation(row, globals);
          if (!t.isIdentity()) {
            (*transform_) *= t;
          }
        }

        transform_now_ = next_node;
      }
    }
  } else {
    // If this node has an effect input, ensure that is pushed last
    NodeValueTable primary;
    if (!n->GetEffectInputID().isEmpty()) {
      primary = database.Take(n->GetEffectInputID());
    }

    table = database.Merge();
    table.Push(primary);
  }

  value_cache_[n][range] = table;

  return table;
}

TexturePtr NodeTraverser::ProcessVideoCacheJob(const CacheJob *val)
{
  return nullptr;
}

QVector2D NodeTraverser::GenerateResolution() const
{
  return QVector2D(video_params_.square_pixel_width(), video_params_.height());
}

void NodeTraverser::ResolveJobs(NodeValue &val)
{
  if (val.type() == NodeValue::kTexture) {

    if (TexturePtr job_tex = val.toTexture()) {
      if (AcceleratedJob *base_job = job_tex->job()) {

        if (resolved_texture_cache_.contains(job_tex.get())) {
          val.set_value(resolved_texture_cache_.value(job_tex.get()));
        } else {
          // Resolve any sub-jobs
          for (auto it=base_job->GetValues().begin(); it!=base_job->GetValues().end(); it++) {
            // Jobs will almost always be submitted with one of these types
            NodeValue &subval = it.value();
            ResolveJobs(subval);
          }

          if (CacheJob *cj = dynamic_cast<CacheJob*>(base_job)) {
            TexturePtr tex = ProcessVideoCacheJob(cj);
            if (tex) {
              val.set_value(tex);
            } else {
              val.set_value(cj->GetFallback());
            }

          } else if (ColorTransformJob *ctj = dynamic_cast<ColorTransformJob*>(base_job)) {

            VideoParams ctj_params = job_tex->params();

            ctj_params.set_format(GetCacheVideoParams().format());

            TexturePtr dest = CreateTexture(ctj_params);

            // Resolve input texture
            NodeValue v = ctj->GetInputTexture();
            ResolveJobs(v);
            ctj->SetInputTexture(v);

            ProcessColorTransform(dest, val.source(), ctj);

            val.set_value(dest);

          } else if (ShaderJob *sj = dynamic_cast<ShaderJob*>(base_job)) {

            VideoParams tex_params = job_tex->params();

            TexturePtr tex = CreateTexture(tex_params);

            ProcessShader(tex, val.source(), sj);

            val.set_value(tex);

          } else if (GenerateJob *gj = dynamic_cast<GenerateJob*>(base_job)) {

            VideoParams tex_params = job_tex->params();

            TexturePtr tex = CreateTexture(tex_params);

            ProcessFrameGeneration(tex, val.source(), gj);

            // Convert to reference space
            const QString &colorspace = tex_params.colorspace();
            if (!colorspace.isEmpty()) {
              // Set format to primary format
              tex_params.set_format(GetCacheVideoParams().format());

              TexturePtr dest = CreateTexture(tex_params);

              ConvertToReferenceSpace(dest, tex, colorspace);

              tex = dest;
            }

            val.set_value(tex);

          } else if (FootageJob *fj = dynamic_cast<FootageJob*>(base_job)) {

            rational footage_time = Footage::AdjustTimeByLoopMode(fj->time().in(), fj->loop_mode(), fj->length(), fj->video_params().video_type(), fj->video_params().frame_rate_as_time_base());

            TexturePtr tex;

            if (footage_time.isNaN()) {
              // Push dummy texture
              tex = CreateDummyTexture(fj->video_params());
            } else {
              VideoParams managed_params = fj->video_params();
              managed_params.set_format(GetCacheVideoParams().format());

              tex = CreateTexture(managed_params);
              ProcessVideoFootage(tex, fj, footage_time);
            }

            val.set_value(tex);

          }

          // Cache resolved value
          resolved_texture_cache_.insert(job_tex.get(), val.toTexture());
        }
      }
    }

  } else if (val.type() == NodeValue::kSamples) {

    if (val.canConvert<SampleJob>()) {

      SampleJob job = val.value<SampleJob>();
      SampleBuffer output_buffer = CreateSampleBuffer(job.samples().audio_params(), job.samples().sample_count());
      ProcessSamples(output_buffer, val.source(), job.time(), job);
      val.set_value(QVariant::fromValue(output_buffer));

    } else if (val.canConvert<FootageJob>()) {

      FootageJob job = val.value<FootageJob>();
      SampleBuffer buffer = CreateSampleBuffer(GetCacheAudioParams(), job.time().length());
      ProcessAudioFootage(buffer, &job, job.time());
      val.set_value(buffer);

    }

  }
}

TexturePtr NodeTraverser::CreateDummyTexture(const VideoParams &p)
{
  return std::make_shared<Texture>(p);
}

}
