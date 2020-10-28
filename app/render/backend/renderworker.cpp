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
#include <QThread>
#include <QTimer>

#include "audio/audiovisualwaveform.h"
#include "common/functiontimer.h"
#include "config/config.h"
#include "node/block/clip/clip.h"
#include "task/conform/conform.h"

OLIVE_NAMESPACE_ENTER

// FIXME: Hardcoded value. It seems to work fine, but is there a possibility we should make
//        this a dynamic value somehow or a configurable value?
const int RenderWorker::kMaxDecoderLife = 6000;

RenderWorker::RenderWorker(RenderBackend* parent) :
  parent_(parent),
  video_force_download_resolution_(false),
  available_(true),
  generate_audio_previews_(false),
  render_mode_(RenderMode::kOnline)
{
  cleanup_timer_ = new QTimer();
  cleanup_timer_->setInterval(kMaxDecoderLife);
  connect(cleanup_timer_, &QTimer::timeout, this, &RenderWorker::ClearOldDecoders, Qt::DirectConnection);
  cleanup_timer_->moveToThread(qApp->thread());
  QMetaObject::invokeMethod(cleanup_timer_, "start", Qt::QueuedConnection);
}

RenderWorker::~RenderWorker()
{
  QMetaObject::invokeMethod(cleanup_timer_, "stop", Qt::QueuedConnection);
  cleanup_timer_->deleteLater();
}

void RenderWorker::Hash(RenderTicketPtr ticket, ViewerOutput *viewer, const QVector<rational> &times)
{
  ticket_ = ticket;

  QVector<QByteArray> hashes(times.size());

  for (int i=0;i<hashes.size();i++) {
    hashes[i] = HashNode(viewer->texture_input()->get_connected_node(),
                         video_params_,
                         times.at(i));
  }

  ticket->Finish(QVariant::fromValue(hashes));

  emit FinishedJob();
}

QByteArray RenderWorker::HashNode(const Node *n, const VideoParams &params, const rational &time)
{
  QCryptographicHash hasher(QCryptographicHash::Sha1);

  // Embed video parameters into this hash
  hasher.addData(reinterpret_cast<const char*>(&params.effective_width()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&params.effective_height()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&params.format()), sizeof(PixelFormat::Format));

  if (n) {
    n->Hash(hasher, time);
  }

  return hasher.result();
}

void RenderWorker::ClearOldDecoders()
{
  QMutexLocker locker(&decoder_lock_);

  QHash<Stream*, qint64>::iterator i = decoder_age_.begin();

  while (i != decoder_age_.end()) {
    if (i.value() < QDateTime::currentMSecsSinceEpoch() - kMaxDecoderLife) {
      // This decoder is old, remove it
      decoder_cache_.remove(i.key());

      i = decoder_age_.erase(i);
    } else {
      i++;
    }
  }
}

void RenderWorker::RenderFrame(RenderTicketPtr ticket, ViewerOutput* viewer, const rational &time)
{
  ticket_ = ticket;

  NodeValueTable table = ProcessInput(viewer->texture_input(),
                                      TimeRange(time, time + video_params_.time_base()));

  QVariant texture = table.Get(NodeParam::kTexture);

  PixelFormat::Format output_format;
  if (!texture.isNull() && TextureHasAlpha(texture)) {
    output_format = PixelFormat::GetFormatWithAlphaChannel(video_params_.format());
  } else {
    output_format = PixelFormat::GetFormatWithoutAlphaChannel(video_params_.format());
  }

  FramePtr frame = Frame::Create();
  frame->set_timestamp(time);

  if (video_force_download_resolution_ || texture.isNull()) {
    // If we're setting the resolution ourselves or we're zeroing it out, allocate the frame now
    frame->set_video_params(VideoParams(video_params_.width(),
                                        video_params_.height(),
                                        video_params_.time_base(),
                                        output_format,
                                        video_params_.pixel_aspect_ratio(),
                                        video_params_.interlacing(),
                                        video_params_.divider()));
    frame->allocate();
  }

  if (texture.isNull()) {
    // Blank frame out
    memset(frame->data(), 0, frame->allocated_size());
  } else {
    // Dump texture contents to frame
    TextureToFrame(texture, frame, video_download_matrix_);
  }

  ticket->Finish(QVariant::fromValue(frame));

  emit FinishedJob();
}

void RenderWorker::RenderAudio(RenderTicketPtr ticket, ViewerOutput* viewer, const TimeRange &range)
{
  ticket_ = ticket;

  NodeValueTable table = ProcessInput(viewer->samples_input(), range);

  QVariant samples = table.Get(NodeParam::kSamples);

  ticket->Finish(samples);

  emit FinishedJob();
}

void RenderWorker::ClearDecoders()
{
  decoder_cache_.clear();
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
      SampleBufferPtr samples_from_this_block = table.Take(NodeParam::kSamples).value<SampleBufferPtr>();

      if (!samples_from_this_block) {
        // If we retrieved no samples from this block, do nothing
        continue;
      }

      // FIXME: Doesn't handle reversing
      if (b->speed_input()->is_keyframing() || b->speed_input()->is_connected()) {
        // FIXME: We'll need to calculate the speed hoo boy
      } else {
        double speed_value = b->speed_input()->get_standard_value().toDouble();

        if (qIsNull(speed_value)) {
          // Just silence, don't think there's any other practical application of 0 speed audio
          samples_from_this_block->fill(0);
        } else if (!qFuzzyCompare(speed_value, 1.0)) {
          // Multiply time
          samples_from_this_block->speed(speed_value);
        }
      }

      int copy_length = qMin(max_dest_sz, samples_from_this_block->sample_count());

      // Copy samples into destination buffer
      block_range_buffer->set(samples_from_this_block->const_data(), destination_offset, copy_length);

      NodeValueTable::Merge({merged_table, table});
    }

    if (generate_audio_previews_) {
      // Find original track object
      TrackOutput* original_track = nullptr;

      // Have to do a manual loop since our track is const and QHash won't take it
      QHash<Node*, Node*>::const_iterator i;
      for (i=copy_map_->constBegin(); i!=copy_map_->constEnd(); i++) {
        if (i.value() == track) {
          original_track = static_cast<TrackOutput*>(i.key());
          break;
        }
      }

      if (original_track) {
        // Generate a visual waveform and send it back to the main thread
        AudioVisualWaveform visual_waveform;
        visual_waveform.set_channel_count(audio_params_.channel_count());
        visual_waveform.OverwriteSamples(block_range_buffer, audio_params_.sample_rate());

        emit WaveformGenerated(ticket_, original_track, visual_waveform, range);
      }
    }

    merged_table.Push(NodeParam::kSamples, QVariant::fromValue(block_range_buffer), track);

    return merged_table;

  } else {
    return NodeTraverser::GenerateBlockTable(track, range);
  }
}

QVariant RenderWorker::ProcessSamples(const Node *node, const TimeRange &range, const SampleJob& job)
{
  if (!job.samples() || !job.samples()->is_allocated()) {
    return QVariant();
  }

  SampleBufferPtr output_buffer = SampleBuffer::CreateAllocated(job.samples()->audio_params(), job.samples()->sample_count());
  NodeValueDatabase value_db;

  for (int i=0;i<job.samples()->sample_count();i++) {
    // Calculate the exact rational time at this sample
    double sample_to_second = static_cast<double>(i) / static_cast<double>(audio_params_.sample_rate());

    rational this_sample_time = rational::fromDouble(range.in().toDouble() + sample_to_second);

    // Update all non-sample and non-footage inputs
    NodeValueMap::const_iterator j;
    for (j=job.GetValues().constBegin(); j!=job.GetValues().constEnd(); j++) {
      NodeValueTable value;
      NodeInput* corresponding_input = node->GetInputWithID(j.key());

      if (corresponding_input) {
        value = ProcessInput(corresponding_input, TimeRange(this_sample_time, this_sample_time));
      } else {
        value.Push(j.value());
      }

      value_db.Insert(j.key(), value);
    }

    AddGlobalsToDatabase(value_db, TimeRange(this_sample_time, this_sample_time));

    node->ProcessSamples(value_db,
                         job.samples(),
                         output_buffer,
                         i);
  }

  return QVariant::fromValue(output_buffer);
}

QVariant RenderWorker::ProcessFrameGeneration(const Node* node, const GenerateJob &job)
{
  FramePtr frame = Frame::Create();

  PixelFormat::Format output_fmt;
  if (job.GetAlphaChannelRequired()) {
    output_fmt = PixelFormat::GetFormatWithAlphaChannel(video_params_.format());
  } else {
    output_fmt = PixelFormat::GetFormatWithoutAlphaChannel(video_params_.format());
  }

  frame->set_video_params(VideoParams(video_params_.width(),
                                      video_params_.height(),
                                      video_params_.time_base(),
                                      output_fmt,
                                      video_params_.pixel_aspect_ratio(),
                                      video_params_.interlacing(),
                                      video_params_.divider()));
  frame->allocate();

  node->GenerateFrame(frame, job);

  return CachedFrameToTexture(frame);
}

QVariant RenderWorker::GetCachedFrame(const Node* node, const rational& time)
{
  if (render_mode_ == RenderMode::kOffline
      && !cache_path_.isEmpty()
      && node->id() == QStringLiteral("org.olivevideoeditor.Olive.videoinput")) {
    QByteArray hash = HashNode(node, video_params(), time);

    FramePtr f = FrameHashCache::LoadCacheFrame(cache_path_, hash);

    if (f) {
      // The cached frame won't load with the correct divider by default, so we enforce it here
      f->set_video_params(VideoParams(f->width() * video_params_.divider(),
                                      f->height() * video_params_.divider(),
                                      f->video_params().time_base(),
                                      f->video_params().format(),
                                      f->video_params().pixel_aspect_ratio(),
                                      f->video_params().interlacing(),
                                      video_params_.divider()));

      return CachedFrameToTexture(f);
    }
  }

  return QVariant();
}

DecoderPtr RenderWorker::ResolveDecoderFromInput(StreamPtr stream)
{
  // Access a map of Node inputs and decoder instances and retrieve a frame!
  QMutexLocker locker(&decoder_lock_);

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

  decoder_age_.insert(stream.get(), QDateTime::currentMSecsSinceEpoch());

  return decoder;
}

QVariant RenderWorker::ProcessVideoFootage(StreamPtr stream, const rational &input_time)
{
  VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream);
  rational time_match = (video_stream->video_type() == VideoStream::kVideoTypeStill) ? 0 : input_time;
  QString colorspace_match = video_stream->get_colorspace_match_string();

  QVariant value;
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

    DecoderPtr decoder = ResolveDecoderFromInput(stream);

    if (decoder) {
      FramePtr frame = decoder->RetrieveVideo(input_time,
                                              video_params().divider());

      if (frame) {
        // Return a texture from the derived class
        value = FootageFrameToTexture(stream, frame);

        if (!value.isNull()) {
          // Put this into the image cache instead
          still_image_cache_.insert(stream.get(), {value,
                                                   colorspace_match,
                                                   video_stream->premultiplied_alpha(),
                                                   video_params_.divider(),
                                                   time_match});
        }
      }
    }

  }

  return value;
}

QVariant RenderWorker::ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time)
{
  QVariant value;

  DecoderPtr decoder = ResolveDecoderFromInput(stream);

  if (decoder) {
    // See if we have a conformed version of this audio
    if (!decoder->HasConformedVersion(audio_params())) {

      // If not, the audio needs to be conformed
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

    if (decoder->HasConformedVersion(audio_params())) {
      SampleBufferPtr frame = decoder->RetrieveAudio(input_time.in(), input_time.length(),
                                                     audio_params());

      if (frame) {
        value = QVariant::fromValue(frame);
      }
    }
  }

  return value;
}

OLIVE_NAMESPACE_EXIT
