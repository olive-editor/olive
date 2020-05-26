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
#include "common/functiontimer.h"
#include "config/config.h"
#include "node/block/clip/clip.h"
#include "task/conform/conform.h"
#include "renderbackend.h"

OLIVE_NAMESPACE_ENTER

RenderWorker::RenderWorker(RenderBackend* parent) :
  parent_(parent),
  available_(true),
  audio_mode_is_preview_(false)
{
}

void RenderWorker::RenderFrame(RenderTicketPtr ticket, ViewerOutput* viewer, const rational &time)
{
  NodeValueTable table = ProcessInput(viewer->texture_input(),
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

  ticket->Finish(QVariant::fromValue(frame));

  FinishedJob();
}

void RenderWorker::RenderAudio(RenderTicketPtr ticket, ViewerOutput* viewer, const TimeRange &range)
{
  NodeValueTable table = ProcessInput(viewer->samples_input(), range);

  QVariant samples = table.Get(NodeParam::kSamples);

  ticket->Finish(samples);

  FinishedJob();
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
        Block* src_block = static_cast<Block*>(copy_map_->key(b));
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

      // Return a texture from the derived class
      return FrameToTexture(decoder, stream, input_time);

    } else if (stream->type() == Stream::kAudio) {

      // See if we have a conformed version of this audio
      if (!decoder->HasConformedVersion(audio_params())) {

        // If not, check what audio mode we're in
        if (audio_mode_is_preview_) {

          // For preview, we report the conform is missing and finish the render without it
          // temporarily. The backend that picks up this signal will recache this section once the
          // conform is available.
          emit AudioConformUnavailable(decoder->stream(),
                                       audio_render_time_,
                                       input_time.out(),
                                       audio_params());

        } else {

          // For online rendering/export, it's a waste of time to render the audio until we have
          // all we need, so we try to handle the conform ourselves
          AudioStreamPtr as = std::static_pointer_cast<AudioStream>(stream);

          // Check if any other threads are conforming this audio
          if (as->try_start_conforming(audio_params())) {

            // If not, conform it ourselves
            decoder->ConformAudio(&IsCancelled(), audio_params());

          } else {

            // If another thread is conforming already, hackily try to wait until it's done.
            do {
              QThread::msleep(1000);
            } while (!as->has_conformed_version(audio_params()) && !IsCancelled());

          }
        }

      }


      if (decoder->HasConformedVersion(audio_params())) {
        SampleBufferPtr frame = decoder->RetrieveAudio(input_time.in(), input_time.length(),
                                                       audio_params());

        if (frame) {
          return NodeValue(NodeParam::kSamples, QVariant::fromValue(frame));
        }
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

OLIVE_NAMESPACE_EXIT
