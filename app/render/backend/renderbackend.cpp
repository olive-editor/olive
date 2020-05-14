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

#include "renderbackend.h"

#include <QDateTime>
#include <QThread>

#include "config/config.h"
#include "core.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  viewer_node_(nullptr),
  divider_(1),
  render_mode_(RenderMode::kOnline),
  pix_fmt_(PixelFormat::PIX_FMT_RGBA32F),
  sample_fmt_(SampleFormat::SAMPLE_FMT_FLT)
{
  // FIXME: Don't create in CLI mode
  cancel_dialog_ = new RenderCancelDialog(Core::instance()->main_window());
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ == viewer_node) {
    return;
  }

  if (viewer_node_) {
    // Clear queue and wait for any currently running actions to complete
    CancelQueue();

    // Delete all of our copied nodes
    video_copy_map_.Clear();
    audio_copy_map_.Clear();

    disconnect(viewer_node_,
               &ViewerOutput::GraphChangedFrom,
               this,
               &RenderBackend::NodeGraphChanged);

    disconnect(viewer_node_->audio_playback_cache(),
               &AudioPlaybackCache::Invalidated,
               this,
               &RenderBackend::AudioCallback);
  }

  // Set viewer node
  viewer_node_ = viewer_node;

  if (viewer_node_) {
    // Start copying viewer
    video_copy_map_.Init(viewer_node_);
    audio_copy_map_.Init(viewer_node_);

    video_copy_map_.Queue(viewer_node_->texture_input());
    audio_copy_map_.Queue(viewer_node_->samples_input());

    video_copy_map_.ProcessQueue();
    audio_copy_map_.ProcessQueue();

    connect(viewer_node_,
            &ViewerOutput::GraphChangedFrom,
            this,
            &RenderBackend::NodeGraphChanged);

    connect(viewer_node_->audio_playback_cache(),
            &AudioPlaybackCache::Invalidated,
            this,
            &RenderBackend::AudioCallback);
  }
}

void RenderBackend::CancelQueue()
{
  // FIXME: Implement something better than this...
  video_copy_map_.thread_pool()->waitForDone();
  audio_copy_map_.thread_pool()->waitForDone();
}

QByteArray HashInternal(Node *node,
                        const VideoRenderingParams &params,
                        const rational &time)
{
  QCryptographicHash hasher(QCryptographicHash::Sha1);

  // Embed video parameters into this hash
  hasher.addData(reinterpret_cast<const char*>(&params.effective_width()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&params.effective_height()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&params.format()), sizeof(PixelFormat::Format));
  hasher.addData(reinterpret_cast<const char*>(&params.mode()), sizeof(RenderMode::Mode));

  node->Hash(hasher, time);

  return hasher.result();
}

QFuture<QByteArray> RenderBackend::Hash(const rational &time)
{
  if (!viewer_node_) {
    return QFuture<QByteArray>();
  }

  return QtConcurrent::run(video_copy_map_.thread_pool(),
                           HashInternal,
                           viewer_node_,
                           video_params(),
                           time);
}

QFuture<FramePtr> RenderBackend::RenderFrame(const rational &time)
{
  if (!viewer_node_) {
    return QFuture<FramePtr>();
  }

  return QtConcurrent::run(video_copy_map_.thread_pool(),
                           this,
                           &RenderBackend::RenderFrameInternal,
                           time);
}

void RenderBackend::SetDivider(const int &divider)
{
  divider_ = divider;
}

void RenderBackend::SetMode(const RenderMode::Mode &mode)
{
  render_mode_ = mode;
}

void RenderBackend::SetPixelFormat(const PixelFormat::Format &pix_fmt)
{
  pix_fmt_ = pix_fmt;
}

void RenderBackend::SetSampleFormat(const SampleFormat::Format &sample_fmt)
{
  sample_fmt_ = sample_fmt;
}

void RenderBackend::NodeGraphChanged(NodeInput *from, NodeInput *source)
{
  if (from == viewer_node_->texture_input()) {
    video_copy_map_.Queue(source);
  } else if (from == viewer_node_->samples_input()) {
    audio_copy_map_.Queue(source);
  }
}

void RenderBackend::FootageProcessingEvent(StreamPtr stream, const TimeRange &input_time, NodeValueTable *table) const
{
  if (stream->type() == Stream::kVideo || stream->type() == Stream::kImage) {

    ImageStreamPtr video_stream = std::static_pointer_cast<ImageStream>(stream);
    rational time_match = (stream->type() == Stream::kImage) ? rational() : input_time.in();
    QString colorspace_match = video_stream->get_colorspace_match_string();

    NodeValue value;
    bool found_cache = false;

    if (still_image_cache_.Has(stream.get())) {
      CachedStill cs = still_image_cache_.Get(stream.get());

      if (cs.colorspace == colorspace_match
          && cs.alpha_is_associated == video_stream->premultiplied_alpha()
          && cs.divider == video_params_.divider()
          && cs.time == time_match) {
        value = cs.texture;
        found_cache = true;
      } else {
        still_image_cache_.Remove(stream.get());
      }
    }

    if (!found_cache) {

      value = GetDataFromStream(stream, input_time);

      still_image_cache_.Add(stream.get(), {value,
                                            colorspace_match,
                                            video_stream->premultiplied_alpha(),
                                            video_params_.divider(),
                                            time_match});

    }

    table->Push(value);

  } else if (stream->type() != Stream::kAudio) {

    table->Push(GetDataFromStream(stream, input_time));

  }
}

NodeValue RenderBackend::GetDataFromStream(StreamPtr stream, const TimeRange &input_time) const
{
  DecoderPtr decoder = ResolveDecoderFromInput(stream);

  if (decoder) {
    return FrameToTexture(decoder, stream, input_time);
  }

  return NodeValue();
}

NodeValueTable RenderBackend::GenerateBlockTable(const TrackOutput *track, const TimeRange &range) const
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
      NodeValueTable table = ProcessNode(NodeDependency(b, range_for_block));
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
        Block* src_block = static_cast<Block*>(copy_map_->key(b));
        QDir local_appdata_dir(Config::Current()["DiskCachePath"].toString());
        QDir waveform_loc = local_appdata_dir.filePath(QStringLiteral("waveform"));
        waveform_loc.mkpath(".");
        QString wave_fn(waveform_loc.filePath(QString::number(reinterpret_cast<quintptr>(src_block))));
        QFile wave_file(wave_fn);

        if (wave_file.open(QFile::ReadWrite)) {
          // We use S32 as a size-compatible substitute for SampleSummer::Sum which is 4 bytes in size
          AudioRenderingParams waveform_params(SampleSummer::kSumSampleRate, audio_params_.channel_layout(), SampleFormat::SAMPLE_FMT_S32);
          int chunk_size = (audio_params().sample_rate() / waveform_params.sample_rate());

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

FramePtr RenderBackend::RenderFrameInternal(const rational &time) const
{
  NodeValueTable table = GenerateTable(viewer_node_,
                                       TimeRange(time, time + viewer_node_->video_params().time_base()));

  QVariant texture = table.Get(NodeParam::kTexture);

  FramePtr frame = Frame::Create();
  frame->set_video_params(video_params());
  frame->allocate();

  if (texture.isNull()) {
    memset(frame->data(), 0, frame->allocated_size());
  } else {
    TextureToFrame(texture, frame);
  }

  return frame;
}

VideoRenderingParams RenderBackend::video_params() const
{
  return VideoRenderingParams(viewer_node_->video_params(),
                              pix_fmt_,
                              render_mode_,
                              divider_);
}

void RenderBackend::AudioCallback()
{
  qDebug() << "STUB";
  /*
  AudioPlaybackCache* pb_cache = viewer_node_->audio_playback_cache();
  QThreadPool* thread_pool = audio_copy_map_.thread_pool();

  while (!pb_cache->IsFullyValidated()
         && thread_pool->activeThreadCount() < thread_pool->maxThreadCount()) {
    // FIXME: Trigger a background audio thread
    TimeRange r = viewer_node_->audio_playback_cache()->GetInvalidatedRanges().first();

    qDebug() << "FIXME: Start rendering audio at:" << r;

    break;
  }
  */
}

bool RenderBackend::ConformWaitInfo::operator==(const RenderBackend::ConformWaitInfo &rhs) const
{
  return rhs.stream == stream
      && rhs.stream_time == stream_time
      && rhs.affected_range == affected_range;
}

RenderBackend::CopyMap::CopyMap() :
  original_viewer_(nullptr),
  copied_viewer_(nullptr)
{
}

void RenderBackend::CopyMap::Init(ViewerOutput *viewer)
{
  original_viewer_ = viewer;

  copied_viewer_ = static_cast<ViewerOutput*>(original_viewer_->copy());
  copy_map_.insert(original_viewer_, copied_viewer_);
}

void RenderBackend::CopyMap::Queue(NodeInput *input)
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

void RenderBackend::CopyMap::ProcessQueue()
{
  while (!queued_updates_.isEmpty()) {
    CopyNodeInputValue(queued_updates_.takeFirst());
  }
}

void RenderBackend::CopyMap::Clear()
{
  qDeleteAll(copy_map_);
  copy_map_.clear();

  original_viewer_ = nullptr;
  copied_viewer_ = nullptr;
}

void RenderBackend::CopyMap::CopyNodeInputValue(NodeInput *input)
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

    {
      // We start by removing all old dependencies from the map
      QList<Node*> old_deps = our_copy->GetExclusiveDependencies();

      foreach (Node* i, old_deps) {
        Node* n = copy_map_.take(copy_map_.key(i));
        delete n;
      }

      // And clear any other edges
      while (!our_copy->edges().isEmpty()) {
        NodeParam::DisconnectEdge(our_copy->edges().first());
      }
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

Node* RenderBackend::CopyMap::CopyNodeConnections(Node* src_node)
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

void RenderBackend::CopyMap::CopyNodeMakeConnection(NodeInput* src_input, NodeInput* dst_input)
{
  if (src_input->IsConnected()) {
    Node* dst_node = CopyNodeConnections(src_input->get_connected_node());

    NodeOutput* corresponding_output = dst_node->GetOutputWithID(src_input->get_connected_output()->id());

    NodeParam::ConnectEdge(corresponding_output,
                           dst_input);
  }
}

OLIVE_NAMESPACE_EXIT
