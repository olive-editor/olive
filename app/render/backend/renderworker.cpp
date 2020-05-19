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

#include "renderworker.h"

#include <QDir>

#include "audio/sumsamples.h"
#include "config/config.h"
#include "node/block/clip/clip.h"
#include "renderbackend.h"

OLIVE_NAMESPACE_ENTER

RenderWorker::RenderWorker(RenderBackend* parent) :
  parent_(parent),
  viewer_(nullptr),
  available_(true)
{
}

RenderWorker::~RenderWorker()
{
  Close();
}

QByteArray RenderWorker::Hash(const rational &time, bool block_for_update)
{
  if (!viewer_) {
    return QByteArray();
  }

  QMutexLocker locker(&lock_);

  UpdateData(block_for_update);

  QCryptographicHash hasher(QCryptographicHash::Sha1);

  // Embed video parameters into this hash
  hasher.addData(reinterpret_cast<const char*>(&video_params_.effective_width()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&video_params_.effective_height()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&video_params_.format()), sizeof(PixelFormat::Format));
  hasher.addData(reinterpret_cast<const char*>(&video_params_.mode()), sizeof(RenderMode::Mode));

  viewer_->Hash(hasher, time);

  emit FinishedJob();

  return hasher.result();
}

FramePtr RenderWorker::RenderFrame(const rational &time, bool block_for_update)
{
  if (!viewer_) {
    return nullptr;
  }

  QMutexLocker locker(&lock_);

  UpdateData(block_for_update);

  NodeValueTable table = ProcessInput(viewer_->texture_input(),
                                      TimeRange(time, time + video_params_.time_base()));

  QVariant texture = table.Get(NodeParam::kTexture);

  FramePtr frame = Frame::Create();
  frame->set_video_params(video_params_);
  frame->set_timestamp(time);
  frame->allocate();

  if (texture.isNull()) {
    // Blank frame out
    memset(frame->data(), 0, frame->allocated_size());
  } else {
    // Dump texture contents to frame
    TextureToFrame(texture, frame, video_download_matrix_);
  }

  emit FinishedJob();

  return frame;
}

SampleBufferPtr RenderWorker::RenderAudio(const TimeRange &range, bool block_for_update)
{
  if (!viewer_) {
    return nullptr;
  }

  QMutexLocker locker(&lock_);

  parent_->WorkerStartedRenderingAudio(range);

  UpdateData(block_for_update);

  audio_render_time_ = range;

  NodeValueTable table = ProcessInput(viewer_->samples_input(), range);

  QVariant samples = table.Get(NodeParam::kSamples);

  return samples.value<SampleBufferPtr>();
}

void RenderWorker::UpdateData(bool block_for_update)
{
  // FIXME: This is pretty trashy. It works, but it's not good. Should probably be changed at some
  //        point.
  if (block_for_update) {
    QMetaObject::invokeMethod(parent_,
                              "UpdateInstance",
                              Qt::BlockingQueuedConnection,
                              OLIVE_NS_ARG(RenderWorker*, this));
  } else {
    parent_->UpdateInstance(this);
  }
}

NodeValueTable RenderWorker::GenerateBlockTable(const TrackOutput *track, const TimeRange &range)
{
  if (track->track_type() == Timeline::kTrackTypeAudio) {

    QList<Block*> active_blocks = track->BlocksAtTimeRange(range);

    // All these blocks will need to output to a buffer so we create one here
    SampleBufferPtr block_range_buffer = SampleBuffer::CreateAllocated(audio_params_,
                                                                       audio_params_.time_to_samples(range.length()));
    block_range_buffer->fill(0);

    NodeValueTable merged_table;

    // Loop through active blocks retrieving their audio
    foreach (Block* b, active_blocks) {
      TimeRange range_for_block(qMax(b->in(), range.in()),
                                qMin(b->out(), range.out()));

      int destination_offset = audio_params_.time_to_samples(range_for_block.in() - range.in());
      int max_dest_sz = audio_params_.time_to_samples(range_for_block.length());

      // Destination buffer
      NodeValueTable table = GenerateTable(b, range_for_block);
      QVariant sample_val = table.Take(NodeParam::kSamples);
      SampleBufferPtr samples_from_this_block;

      if (sample_val.isNull()
          || !(samples_from_this_block = sample_val.value<SampleBufferPtr>())) {
        // If we retrieved no samples from this block, do nothing
        continue;
      }

      // Stretch samples here
      rational abs_speed = qAbs(b->speed());

      if (abs_speed != 1) {
        samples_from_this_block->speed(abs_speed.toDouble());
      }

      if (b->is_reversed()) {
        // Reverse the audio buffer
        samples_from_this_block->reverse();
      }

      int copy_length = qMin(max_dest_sz, samples_from_this_block->sample_count_per_channel());

      // Copy samples into destination buffer
      block_range_buffer->set(samples_from_this_block->const_data(), destination_offset, copy_length);

      {
        // Save waveform to file
        Block* src_block = static_cast<Block*>(copy_map_.key(b));
        QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());
        QDir waveform_loc = local_appdata_dir.filePath(QStringLiteral("waveform"));
        waveform_loc.mkpath(".");
        QString wave_fn(waveform_loc.filePath(QString::number(reinterpret_cast<quintptr>(src_block))));
        QFile wave_file(wave_fn);

        if (wave_file.open(QFile::ReadWrite)) {
          // We use S32 as a size-compatible substitute for SampleSummer::Sum which is 4 bytes in
          // size
          AudioRenderingParams waveform_params(SampleSummer::kSumSampleRate,
                                               audio_params_.channel_layout(),
                                               SampleFormat::SAMPLE_FMT_S32);

          int chunk_size = (audio_params_.sample_rate() / waveform_params.sample_rate());

          {
            // Write metadata header
            SampleSummer::Info info;
            info.channels = audio_params_.channel_count();
            wave_file.write(reinterpret_cast<char*>(&info), sizeof(SampleSummer::Info));
          }

          qint64 start_offset = sizeof(SampleSummer::Info) + waveform_params.time_to_bytes(range_for_block.in() - b->in());
          qint64 length_offset = waveform_params.time_to_bytes(range_for_block.length());
          qint64 end_offset = start_offset + length_offset;

          if (wave_file.size() < end_offset) {
            wave_file.resize(end_offset);
          }

          wave_file.seek(start_offset);

          for (int i=0;i<samples_from_this_block->sample_count_per_channel();i+=chunk_size) {
            QVector<SampleSummer::Sum> summary = SampleSummer::SumSamples(samples_from_this_block,
                                                                          i,
                                                                          qMin(chunk_size, samples_from_this_block->sample_count_per_channel() - i));

            wave_file.write(reinterpret_cast<const char*>(summary.constData()),
                            summary.size() * sizeof(SampleSummer::Sum));
          }

          wave_file.close();

          if (src_block->type() == Block::kClip) {
            emit static_cast<ClipBlock*>(src_block)->PreviewUpdated();
          }
        }
      }

      NodeValueTable::Merge({merged_table, table});
    }

    merged_table.Push(NodeParam::kSamples, QVariant::fromValue(block_range_buffer));

    return merged_table;

  } else {
    return NodeTraverser::GenerateBlockTable(track, range);
  }
}

void RenderWorker::ProcessNodeEvent(const Node *node, const TimeRange &range, NodeValueDatabase &input_params_in, NodeValueTable &output_params)
{
  // Check if node processes samples
  if (!(node->GetCapabilities(input_params_in) & Node::kSampleProcessor)) {
    return;
  }

  // Copy database so we can make some temporary modifications to it
  NodeValueDatabase input_params = input_params_in;
  NodeInput* sample_input = node->ProcessesSamplesFrom(input_params);

  // Try to find the sample buffer in the table
  QVariant samples_var = input_params[sample_input].Get(NodeParam::kSamples);

  // If there isn't one, there's nothing to do
  if (samples_var.isNull()) {
    return;
  }

  SampleBufferPtr input_buffer = samples_var.value<SampleBufferPtr>();

  if (!input_buffer) {
    return;
  }

  SampleBufferPtr output_buffer = SampleBuffer::CreateAllocated(input_buffer->audio_params(), input_buffer->sample_count_per_channel());

  int sample_count = input_buffer->sample_count_per_channel();

  // FIXME: Hardcoded float sample format
  for (int i=0;i<sample_count;i++) {
    // Calculate the exact rational time at this sample
    int sample_out_of_channel = i / audio_params_.channel_count();
    double sample_to_second = static_cast<double>(sample_out_of_channel) / static_cast<double>(audio_params_.sample_rate());

    rational this_sample_time = rational::fromDouble(range.in().toDouble() + sample_to_second);

    // Update all non-sample and non-footage inputs
    foreach (NodeParam* param, node->parameters()) {
      if (param->type() == NodeParam::kInput
          && param != sample_input) {
        NodeInput* input = static_cast<NodeInput*>(param);

        // If the input isn't keyframing, we don't need to update it unless it's connected, in which case it may change
        if (input->IsConnected() || input->is_keyframing()) {
          input_params.Insert(input, ProcessInput(input, TimeRange(this_sample_time, this_sample_time)));
        }
      }
    }

    node->ProcessSamples(input_params,
                         audio_params_,
                         input_buffer,
                         output_buffer,
                         i);
  }

  output_params.Push(NodeParam::kSamples, QVariant::fromValue(output_buffer));
}

void RenderWorker::FootageProcessingEvent(StreamPtr stream, const TimeRange &input_time, NodeValueTable *table)
{
  if (stream->type() == Stream::kVideo || stream->type() == Stream::kImage) {

    ImageStreamPtr video_stream = std::static_pointer_cast<ImageStream>(stream);
    rational time_match = (stream->type() == Stream::kImage) ? rational() : input_time.in();
    QString colorspace_match = video_stream->get_colorspace_match_string();

    NodeValue value;
    bool found_cache = false;

    if (still_image_cache_.contains(stream.get())) {
      const CachedStill& cs = still_image_cache_[stream.get()];

      if (cs.colorspace == colorspace_match
          && cs.alpha_is_associated == video_stream->premultiplied_alpha()
          && cs.divider == video_params_.divider()
          && cs.time == time_match) {
        value = cs.texture;
        found_cache = true;
      } else {
        still_image_cache_.remove(stream.get());
      }
    }

    if (!found_cache) {

      value = GetDataFromStream(stream, input_time);

      still_image_cache_.insert(stream.get(), {value,
                                               colorspace_match,
                                               video_stream->premultiplied_alpha(),
                                               video_params_.divider(),
                                               time_match});

    }

    table->Push(value);

  } else if (stream->type() == Stream::kAudio) {

    table->Push(GetDataFromStream(stream, input_time));

  }
}

NodeValue RenderWorker::GetDataFromStream(StreamPtr stream, const TimeRange &input_time)
{
  DecoderPtr decoder = ResolveDecoderFromInput(stream);

  if (decoder) {
    if (stream->type() == Stream::kVideo || stream->type() == Stream::kImage) {
      return FrameToTexture(decoder, stream, input_time);
    } else if (stream->type() == Stream::kAudio) {
      if (decoder->HasConformedVersion(audio_params())) {
        SampleBufferPtr frame = decoder->RetrieveAudio(input_time.in(), input_time.length(),
                                                       audio_params());

        if (frame) {
          return NodeValue(NodeParam::kSamples, QVariant::fromValue(frame));
        }
      } else {
        emit AudioConformUnavailable(decoder->stream(), audio_render_time_,
                                     input_time.out(), audio_params());
      }
    }
  }

  return NodeValue();
}

DecoderPtr RenderWorker::ResolveDecoderFromInput(StreamPtr stream)
{
  // Access a map of Node inputs and decoder instances and retrieve a frame!

  DecoderPtr decoder = decoder_cache_.value(stream.get());

  if (!decoder && stream) {
    // Create a new Decoder here
    decoder = Decoder::CreateFromID(stream->footage()->decoder());
    decoder->set_stream(stream);

    if (decoder->Open()) {
      decoder_cache_.insert(stream.get(), decoder);
    } else {
      decoder = nullptr;
      qWarning() << "Failed to open decoder for" << stream->footage()->filename()
                 << "::" << stream->index();
    }
  }

  return decoder;
}

void RenderWorker::Queue(NodeInput *input)
{
  if (!queued_updates_.isEmpty()) {
    // Remove any inputs that are dependents of this input since they may have been removed since
    // it was queued
    QList<Node*> deps = input->GetDependencies();

    for (int i=0;i<queued_updates_.size();i++) {
      if (deps.contains(queued_updates_.at(i)->parentNode())) {
        // We don't need to queue this value since this input supersedes it
        queued_updates_.removeAt(i);
        i--;
      }
    }
  }

  queued_updates_.append(input);
}

void RenderWorker::ProcessQueue()
{
  while (!queued_updates_.isEmpty()) {
    CopyNodeInputValue(queued_updates_.takeFirst());
  }
}

void RenderWorker::CopyNodeInputValue(NodeInput *input)
{
  // Find our copy of this parameter
  Node* our_copy_node = copy_map_.value(input->parentNode());
  NodeInput* our_copy = our_copy_node->GetInputWithID(input->id());

  // Copy the standard/keyframe values between these two inputs
  NodeInput::CopyValues(input,
                        our_copy,
                        false);

  // Handle connections
  if (input->IsConnected() || our_copy->IsConnected()) {
    // If one of the inputs is connected, it's likely this change came from connecting or
    // disconnecting whatever was connected to it

    // We start by removing all old dependencies from the map
    QList<Node*> old_deps = our_copy->GetExclusiveDependencies();
    foreach (Node* i, old_deps) {
      delete copy_map_.take(copy_map_.key(i));
    }

    // And clear any other edges
    while (!our_copy->edges().isEmpty()) {
      NodeParam::DisconnectEdge(our_copy->edges().first());
    }

    // Then we copy all node dependencies and connections (if there are any)
    CopyNodeMakeConnection(input, our_copy);
  }

  // Call on sub-elements too
  if (input->IsArray()) {
    foreach (NodeInput* i, static_cast<NodeInputArray*>(input)->sub_params()) {
      CopyNodeInputValue(i);
    }
  }
}

Node* RenderWorker::CopyNodeConnections(Node* src_node)
{
  // Check if this node is already in the map
  Node* dst_node = copy_map_.value(src_node);

  // If not, create it now
  if (!dst_node) {
    dst_node = src_node->copy();
    copy_map_.insert(src_node, dst_node);
  }

  // Make sure its values are copied
  Node::CopyInputs(src_node, dst_node, false);

  // Copy all connections
  QList<NodeInput*> src_node_inputs = src_node->GetInputsIncludingArrays();
  QList<NodeInput*> dst_node_inputs = dst_node->GetInputsIncludingArrays();

  for (int i=0;i<src_node_inputs.size();i++) {
    NodeInput* src_input = src_node_inputs.at(i);

    CopyNodeMakeConnection(src_input, dst_node_inputs.at(i));
  }

  return dst_node;
}

void RenderWorker::CopyNodeMakeConnection(NodeInput* src_input, NodeInput* dst_input)
{
  if (src_input->IsConnected()) {
    Node* dst_node = CopyNodeConnections(src_input->get_connected_node());

    NodeOutput* corresponding_output = dst_node->GetOutputWithID(src_input->get_connected_output()->id());

    NodeParam::ConnectEdge(corresponding_output,
                           dst_input);
  }
}

void RenderWorker::Init(ViewerOutput* viewer)
{
  viewer_ = static_cast<ViewerOutput*>(viewer->copy());
  copy_map_.insert(viewer, viewer_);

  Queue(viewer->texture_input());
  Queue(viewer->samples_input());
  Queue(viewer->track_input(Timeline::kTrackTypeAudio));
  ProcessQueue();
}

void RenderWorker::Close()
{
  // Delete all the nodes
  qDeleteAll(copy_map_);
  copy_map_.clear();

  viewer_ = nullptr;
}

OLIVE_NAMESPACE_EXIT
