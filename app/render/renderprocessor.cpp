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

#include "renderprocessor.h"

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "rendermanager.h"

OLIVE_NAMESPACE_ENTER

RenderProcessor::RenderProcessor(RenderTicketPtr ticket, Renderer *render_ctx) :
  ticket_(ticket),
  render_ctx_(render_ctx)
{
}

void RenderProcessor::Run()
{
  // Depending on the render ticket type, start a job
  RenderManager::TicketType type = ticket_->property("type").value<RenderManager::TicketType>();

  ticket_->Start();

  switch (type) {
  case RenderManager::kTypeVideo:
  {
    ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"));
    rational time = ticket_->property("time").value<rational>();

    NodeValueTable table = ProcessInput(viewer->texture_input(),
                                        TimeRange(time, time + viewer->video_params().time_base()));

    Renderer::TexturePtr texture = table.Get(NodeParam::kTexture).value<Renderer::TexturePtr>();

    QSize frame_size = ticket_->property("size").value<QSize>();
    if (frame_size.isNull()) {
      frame_size = QSize(viewer->video_params().effective_width(),
                         viewer->video_params().effective_height());
    }

    FramePtr frame = Frame::Create();
    frame->set_timestamp(time);
    frame->set_video_params(VideoParams(frame_size.width(),
                                        frame_size.height(),
                                        viewer->video_params().time_base(),
                                        viewer->video_params().format(),
                                        viewer->video_params().pixel_aspect_ratio(),
                                        viewer->video_params().interlacing(),
                                        viewer->video_params().divider()));
    frame->allocate();

    if (!texture) {
      // Blank frame out
      memset(frame->data(), 0, frame->allocated_size());
    } else {
      // Dump texture contents to frame
      const VideoParams& tex_params = texture->params();

      if (tex_params.width() != frame->width() || tex_params.height() != frame->height()) {
        // FIXME: Blit this shit
      }

      render_ctx_->DownloadFromTexture(texture.get(), frame->data(), frame->linesize_pixels());
    }

    ticket_->Finish(QVariant::fromValue(frame), IsCancelled());
    break;
  }
  case RenderManager::kTypeAudio:
  {
    ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"));
    TimeRange time = ticket_->property("time").value<TimeRange>();

    NodeValueTable table = ProcessInput(viewer->samples_input(), time);

    ticket_->Finish(table.Get(NodeParam::kSamples), IsCancelled());
    break;
  }
  case RenderManager::kTypeVideoDownload:
  {
    FrameHashCache* cache = Node::ValueToPtr<FrameHashCache>(ticket_->property("cache"));
    FramePtr frame = ticket_->property("frame").value<FramePtr>();
    QByteArray hash = ticket_->property("hash").toByteArray();

    ticket_->Finish(cache->SaveCacheFrame(hash, frame), false);
    break;
  }
  default:
    // Fail
    ticket_->Cancel();
  }

  this->deleteLater();
}

void RenderProcessor::Process(RenderTicketPtr ticket, Renderer *render_ctx)
{
  RenderProcessor p(ticket, render_ctx);
  p.Run();
}

NodeValueTable RenderProcessor::GenerateBlockTable(const TrackOutput *track, const TimeRange &range)
{
  if (track->track_type() == Timeline::kTrackTypeAudio) {

    const AudioParams& audio_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->audio_params();

    QList<Block*> active_blocks = track->BlocksAtTimeRange(range);

    // All these blocks will need to output to a buffer so we create one here
    SampleBufferPtr block_range_buffer = SampleBuffer::CreateAllocated(audio_params,
                                                                       audio_params.time_to_samples(range.length()));
    block_range_buffer->fill(0);

    NodeValueTable merged_table;

    // Loop through active blocks retrieving their audio
    foreach (Block* b, active_blocks) {
      TimeRange range_for_block(qMax(b->in(), range.in()),
                                qMin(b->out(), range.out()));

      int destination_offset = audio_params.time_to_samples(range_for_block.in() - range.in());
      int max_dest_sz = audio_params.time_to_samples(range_for_block.length());

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

    if (ticket_->property("waveforms").toBool()) {
      // Generate a visual waveform and send it back to the main thread
      AudioVisualWaveform visual_waveform;
      visual_waveform.set_channel_count(audio_params.channel_count());
      visual_waveform.OverwriteSamples(block_range_buffer, audio_params.sample_rate());

      RenderedWaveform waveform_info = {track, visual_waveform, range};
      QVector<RenderedWaveform> waveform_list = ticket_->property("waveforms").value< QVector<RenderedWaveform> >();
      waveform_list.append(waveform_info);
      ticket_->setProperty("waveforms", QVariant::fromValue(waveform_list));
    }

    merged_table.Push(NodeParam::kSamples, QVariant::fromValue(block_range_buffer), track);

    return merged_table;

  } else {
    return NodeTraverser::GenerateBlockTable(track, range);
  }
}

QVariant RenderProcessor::ProcessVideoFootage(StreamPtr stream, const rational &input_time)
{
  Renderer::TexturePtr value = nullptr;

  // Check the still frame cache. On large frames such as high resolution still images, uploading
  // and color managing them for every frame is a waste of time, so we implement a small cache here
  // to optimize such a situation
  VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(stream);
  const VideoParams& video_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->video_params();
  StillImageCache::Entry want_entry = {nullptr,
                                       stream,
                                       video_stream->get_colorspace_match_string(),
                                       video_stream->premultiplied_alpha(),
                                       video_params.divider(),
                                       (video_stream->video_type() == VideoStream::kVideoTypeStill) ? 0 : input_time};

  still_image_cache_->mutex()->lock();

  foreach (const StillImageCache::Entry& e, still_image_cache_->entries()) {
    if (StillImageCache::CompareEntryMetadata(want_entry, e)) {
      // Found an exact match of the texture we want in the cache, use it instead of reading it
      // ourselves
      value = e.texture;
      break;
    }
  }

  if (!value) {
    // Failed to find the texture, let's see if it's being generated by another processor
    foreach (const StillImageCache::Entry& e, still_image_cache_->pending()) {
      if (StillImageCache::CompareEntryMetadata(want_entry, e)) {
        // An exact match of this texture is pending, let's wait for it
        while (!value) {
          // FIXME: Hacky way of waiting for other threads
          still_image_cache_->mutex()->unlock();
          QThread::msleep(1);
          still_image_cache_->mutex()->lock();

          value = e.texture;
        }
        break;
      }
    }
  }

  if (value) {
    // Found the texture, we can release the cache now
    still_image_cache_->mutex()->unlock();
  } else {
    // Wasn't in still image cache, so we'll have to retrieve it from the decoder

    // Let other processors know we're getting this texture
    still_image_cache_->PushPending(want_entry);

    still_image_cache_->mutex()->unlock();

    DecoderPtr decoder = ResolveDecoderFromInput(stream);

    if (decoder) {
      FramePtr frame = decoder->RetrieveVideo(input_time,
                                              video_params.divider());

      if (frame) {
        // Return a texture from the derived class
        Renderer::TexturePtr unmanaged_texture = render_ctx_->CreateTexture(frame->video_params(), frame->data(), frame->linesize_pixels());

        Renderer::TexturePtr managed_texture = render_ctx_->TransformColor(unmanaged_texture, )

        value = FootageFrameToTexture(stream, frame);

        still_image_cache_->mutex()->lock();

        still_image_cache_->RemovePending(want_entry);

        // Put this into the image cache instead
        want_entry.texture = value;
        still_image_cache_->PushEntry(want_entry);

        still_image_cache_->mutex()->unlock();
      }
    }
  }

  return QVariant::fromValue(value);
}

QVariant RenderProcessor::ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time)
{
  QVariant value;

  DecoderPtr decoder = ResolveDecoderFromInput(stream);

  if (decoder) {
    const AudioParams& audio_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->audio_params();

    // See if we have a conformed version of this audio
    if (!decoder->HasConformedVersion(audio_params)) {

      // If not, the audio needs to be conformed
      // For online rendering/export, it's a waste of time to render the audio until we have
      // all we need, so we try to handle the conform ourselves
      AudioStreamPtr as = std::static_pointer_cast<AudioStream>(stream);

      // Check if any other threads are conforming this audio
      if (as->try_start_conforming(audio_params)) {

        // If not, conform it ourselves
        decoder->ConformAudio(&IsCancelled(), audio_params);

      } else {

        // If another thread is conforming already, hackily try to wait until it's done.
        do {
          QThread::msleep(1000);
        } while (!as->has_conformed_version(audio_params) && !IsCancelled());

      }

    }

    if (decoder->HasConformedVersion(audio_params)) {
      SampleBufferPtr frame = decoder->RetrieveAudio(input_time.in(), input_time.length(),
                                                     audio_params);

      if (frame) {
        value = QVariant::fromValue(frame);
      }
    }
  }

  return value;
}

QVariant RenderProcessor::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  const VideoParams& video_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->video_params();

  render_ctx_->ProcessShader(node, range, job, video_params);
}

QVariant RenderProcessor::ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job)
{
  if (!job.samples() || !job.samples()->is_allocated()) {
    return QVariant();
  }

  SampleBufferPtr output_buffer = SampleBuffer::CreateAllocated(job.samples()->audio_params(), job.samples()->sample_count());
  NodeValueDatabase value_db;

  const AudioParams& audio_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->audio_params();

  for (int i=0;i<job.samples()->sample_count();i++) {
    // Calculate the exact rational time at this sample
    double sample_to_second = static_cast<double>(i) / static_cast<double>(audio_params.sample_rate());

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

QVariant RenderProcessor::ProcessFrameGeneration(const Node *node, const GenerateJob &job)
{
  FramePtr frame = Frame::Create();

  const VideoParams& video_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->video_params();

  PixelFormat::Format output_fmt;
  if (job.GetAlphaChannelRequired()) {
    output_fmt = PixelFormat::GetFormatWithAlphaChannel(video_params.format());
  } else {
    output_fmt = PixelFormat::GetFormatWithoutAlphaChannel(video_params.format());
  }

  frame->set_video_params(VideoParams(video_params.width(),
                                      video_params.height(),
                                      video_params.time_base(),
                                      output_fmt,
                                      video_params.pixel_aspect_ratio(),
                                      video_params.interlacing(),
                                      video_params.divider()));
  frame->allocate();

  node->GenerateFrame(frame, job);

  Renderer::TexturePtr texture = render_ctx_->CreateTexture(frame->video_params(), frame->data(), frame->linesize_pixels());

  return QVariant::fromValue(texture);
}

QVariant RenderProcessor::GetCachedFrame(const Node *node, const rational &time)
{
  if (ticket_->property("mode").value<RenderMode::Mode>() == RenderMode::kOffline
      && !cache_path_.isEmpty()
      && node->id() == QStringLiteral("org.olivevideoeditor.Olive.videoinput")) {
    const VideoParams& video_params = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"))->video_params();

    QByteArray hash = RenderManager::Hash(node, video_params, time);

    FramePtr f = FrameHashCache::LoadCacheFrame(cache_path_, hash);

    if (f) {
      // The cached frame won't load with the correct divider by default, so we enforce it here
      f->set_video_params(VideoParams(f->width() * video_params.divider(),
                                      f->height() * video_params.divider(),
                                      f->video_params().time_base(),
                                      f->video_params().format(),
                                      f->video_params().pixel_aspect_ratio(),
                                      f->video_params().interlacing(),
                                      video_params.divider()));

      Renderer::TexturePtr texture = render_ctx_->CreateTexture(f->video_params(), f->data(), f->linesize_pixels());
      return QVariant::fromValue(texture);
    }
  }

  return QVariant();
}

OLIVE_NAMESPACE_EXIT
