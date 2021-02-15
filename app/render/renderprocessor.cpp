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

#include "renderprocessor.h"

#include <QOpenGLContext>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "project/project.h"
#include "rendermanager.h"

namespace olive {

RenderProcessor::RenderProcessor(RenderTicketPtr ticket, Renderer *render_ctx, StillImageCache* still_image_cache, DecoderCache* decoder_cache, ShaderCache *shader_cache, QVariant default_shader) :
  ticket_(ticket),
  render_ctx_(render_ctx),
  still_image_cache_(still_image_cache),
  decoder_cache_(decoder_cache),
  shader_cache_(shader_cache),
  default_shader_(default_shader)
{
}

void RenderProcessor::Run()
{
  // Depending on the render ticket type, start a job
  RenderManager::TicketType type = ticket_->property("type").value<RenderManager::TicketType>();

  ticket_->Start();

  if (ticket_->WasCancelled()) {
    return;
  }

  switch (type) {
  case RenderManager::kTypeVideo:
  {
    Sequence* viewer = Node::ValueToPtr<Sequence>(ticket_->property("viewer"));
    const VideoParams& video_params = ticket_->property("vparam").value<VideoParams>();
    rational time = ticket_->property("time").value<rational>();

    NodeValueTable table = ProcessInput(viewer, Sequence::kTextureInput,
                                        TimeRange(time, time + video_params.time_base()));

    TexturePtr texture = table.Get(NodeValue::kTexture).value<TexturePtr>();

    // Set up output frame parameters
    VideoParams frame_params = ticket_->property("vparam").value<VideoParams>();

    QSize frame_size = ticket_->property("size").value<QSize>();
    if (!frame_size.isNull()) {
      frame_params.set_width(frame_size.width());
      frame_params.set_height(frame_size.height());
    }

    VideoParams::Format frame_format = static_cast<VideoParams::Format>(ticket_->property("format").toInt());
    if (frame_format != VideoParams::kFormatInvalid) {
      frame_params.set_format(frame_format);
    }

    if (RenderManager::instance()->backend() == RenderManager::kOpenGL
        && QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES) {
      // HACK: From what I can tell, ANGLE only supports texture reading to RGBA
      frame_params.set_channel_count(VideoParams::kRGBAChannelCount);
    } else if (texture) {
      frame_params.set_channel_count(texture->channel_count());
    }

    FramePtr frame = Frame::Create();
    frame->set_timestamp(time);
    frame->set_video_params(frame_params);
    frame->allocate();

    if (!texture) {
      // Blank frame out
      memset(frame->data(), 0, frame->allocated_size());
    } else {
      // Dump texture contents to frame
      ColorProcessorPtr output_color_transform = ticket_->property("coloroutput").value<ColorProcessorPtr>();
      const VideoParams& tex_params = texture->params();

      if (tex_params.effective_width() != frame_params.effective_width()
          || tex_params.effective_height() != frame_params.effective_height()
          || tex_params.format() != frame_params.format()
          || output_color_transform) {
        TexturePtr blit_tex = render_ctx_->CreateTexture(frame_params);

        QMatrix4x4 matrix = ticket_->property("matrix").value<QMatrix4x4>();

        if (output_color_transform) {
          // Yes color transform, blit color managed
          render_ctx_->BlitColorManaged(output_color_transform, texture, true, blit_tex.get(), true, matrix);
        } else {
          // No color transform, just blit
          ShaderJob job;
          job.InsertValue(QStringLiteral("ove_maintex"), NodeValue(NodeValue::kTexture, QVariant::fromValue(texture)));
          job.InsertValue(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, matrix));

          render_ctx_->BlitToTexture(default_shader_, job, blit_tex.get());
        }

        // Replace texture that we're going to download in the next step
        texture = blit_tex;
      }

      render_ctx_->DownloadFromTexture(texture.get(), frame->data(), frame->linesize_pixels());
    }

    ticket_->Finish(QVariant::fromValue(frame), IsCancelled());
    break;
  }
  case RenderManager::kTypeAudio:
  {
    Sequence* viewer = Node::ValueToPtr<Sequence>(ticket_->property("viewer"));
    TimeRange time = ticket_->property("time").value<TimeRange>();

    NodeValueTable table = ProcessInput(viewer, Sequence::kSamplesInput, time);

    ticket_->Finish(table.Get(NodeValue::kSamples), IsCancelled());
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
}

DecoderPtr RenderProcessor::ResolveDecoderFromInput(const QString& decoder_id, const Decoder::CodecStream &stream)
{
  if (!stream.IsValid()) {
    qWarning() << "Attempted to resolve the decoder of a null stream";
    return nullptr;
  }

  QMutexLocker locker(decoder_cache_->mutex());

  DecoderPtr decoder = decoder_cache_->value(stream);

  if (!decoder) {
    // No decoder
    decoder = Decoder::CreateFromID(decoder_id);

    if (decoder->Open(stream)) {
      decoder_cache_->insert(stream, decoder);
    } else {
      qWarning() << "Failed to open decoder for" << stream.filename()
                 << "::" << stream.stream();
      return nullptr;
    }
  }

  return decoder;
}

void RenderProcessor::Process(RenderTicketPtr ticket, Renderer *render_ctx, StillImageCache *still_image_cache, DecoderCache *decoder_cache, ShaderCache *shader_cache, QVariant default_shader)
{
  RenderProcessor p(ticket, render_ctx, still_image_cache, decoder_cache, shader_cache, default_shader);
  p.Run();
}

NodeValueTable RenderProcessor::GenerateBlockTable(const Track *track, const TimeRange &range)
{
  if (track->type() == Track::kAudio) {

    const AudioParams& audio_params = ticket_->property("aparam").value<AudioParams>();

    QVector<Block*> active_blocks = track->BlocksAtTimeRange(range);

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
      NodeValueTable table = GenerateTable(b, Track::TransformRangeForBlock(b, range_for_block));
      SampleBufferPtr samples_from_this_block = table.Take(NodeValue::kSamples).value<SampleBufferPtr>();

      if (!samples_from_this_block) {
        // If we retrieved no samples from this block, do nothing
        continue;
      }

      // FIXME: Doesn't handle reversing
      if (b->IsInputKeyframing(Block::kSpeedInput) || b->IsInputConnected(Block::kSpeedInput)) {
        // FIXME: We'll need to calculate the speed hoo boy
      } else {
        double speed_value = b->GetStandardValue(Block::kSpeedInput).toDouble();

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

    if (ticket_->property("enablewaveforms").toBool()) {
      // Generate a visual waveform and send it back to the main thread
      AudioVisualWaveform visual_waveform;
      visual_waveform.set_channel_count(audio_params.channel_count());
      visual_waveform.OverwriteSamples(block_range_buffer, audio_params.sample_rate());

      RenderedWaveform waveform_info = {track, visual_waveform, range};
      QVector<RenderedWaveform> waveform_list = ticket_->property("waveforms").value< QVector<RenderedWaveform> >();
      waveform_list.append(waveform_info);
      ticket_->setProperty("waveforms", QVariant::fromValue(waveform_list));
    }

    merged_table.Push(NodeValue::kSamples, QVariant::fromValue(block_range_buffer), track);

    return merged_table;

  } else {
    return NodeTraverser::GenerateBlockTable(track, range);
  }
}

QVariant RenderProcessor::ProcessVideoFootage(const Footage::StreamReference &stream, const rational &input_time)
{
  TexturePtr value = nullptr;

  // Check the still frame cache. On large frames such as high resolution still images, uploading
  // and color managing them for every frame is a waste of time, so we implement a small cache here
  // to optimize such a situation
  const VideoParams& render_params = ticket_->property("vparam").value<VideoParams>();
  VideoParams stream_params = stream.video_params();

  ColorManager* color_manager = Node::ValueToPtr<ColorManager>(ticket_->property("colormanager"));

  // See if we can make this divider larger (i.e. if the fooage is smaller)
  int footage_divider = render_params.divider();
  while (footage_divider > 1
         && VideoParams::GetScaledDimension(stream_params.width(), footage_divider-1) < render_params.effective_width()
         && VideoParams::GetScaledDimension(stream_params.height(), footage_divider-1) < render_params.effective_height()) {
    footage_divider--;
  }

  Stream stream_data = stream.GetStream();

  StillImageCache::EntryPtr want_entry = std::make_shared<StillImageCache::Entry>(
        nullptr,
        stream,
        ColorProcessor::GenerateID(color_manager, stream.video_colorspace(), color_manager->GetReferenceColorSpace()),
        stream_data.premultiplied_alpha(),
        footage_divider,
        (stream_data.video_type() == Stream::kVideoTypeStill) ? 0 : input_time,
        true);

  bool found_existing = false;

  still_image_cache_->mutex()->lock();

  foreach (StillImageCache::EntryPtr e, still_image_cache_->entries()) {
    if (StillImageCache::CompareEntryMetadata(want_entry, e)) {
      // Found an exact match of the texture we want in the cache. See if it's working or if it's
      // ready.
      want_entry = e;
      found_existing = true;

      while (want_entry->working) {
        still_image_cache_->wait_cond()->wait(still_image_cache_->mutex());
      }

      value = want_entry->texture;
      break;
    }
  }

  if (value) {
    // Found the texture, we can release the cache now
    still_image_cache_->mutex()->unlock();
  } else {
    // Wasn't in still image cache, so we'll have to retrieve it from the decoder

    // Let other processors know we're getting this texture (want_entry's `working` field is
    // already set to true in the initializer above)
    if (!found_existing) {
      still_image_cache_->PushEntry(want_entry);
    }

    still_image_cache_->mutex()->unlock();

    QString decoder_id = stream.footage()->decoder();

    DecoderPtr decoder = nullptr;

    if (stream_data.video_type() == Stream::kVideoTypeVideo) {
      decoder = ResolveDecoderFromInput(decoder_id, Decoder::GetCodecStreamFromStreamReference(stream));
    } else {
      // Since image sequences involve multiple files, we don't engage the decoder cache
      decoder = Decoder::CreateFromID(decoder_id);

      QString frame_filename;

      if (stream_data.video_type() == Stream::kVideoTypeImageSequence) {
        int64_t frame_number = stream.GetTimeInTimebaseUnits(input_time);
        frame_filename = Decoder::TransformImageSequenceFileName(stream.filename(), frame_number);
      } else {
        frame_filename = stream.filename();
      }

      // Decoder will close automatically since it's a stream_ptr
      decoder->Open(Decoder::CodecStream(frame_filename, stream.index()));
    }

    if (decoder) {
      FramePtr frame = decoder->RetrieveVideo((stream_data.video_type() == Stream::kVideoTypeVideo) ? input_time : Decoder::kAnyTimecode, footage_divider);

      if (frame) {
        // Return a texture from the derived class
        TexturePtr unmanaged_texture = render_ctx_->CreateTexture(frame->video_params(),
                                                                  frame->data(),
                                                                  frame->linesize_pixels());

        // We convert to our rendering pixel format, since that will always be float-based which
        // is necessary for correct color conversion
        VideoParams managed_params = frame->video_params();
        managed_params.set_format(render_params.format());
        managed_params.set_pixel_aspect_ratio(stream_data.pixel_aspect_ratio());
        managed_params.set_interlacing(stream_data.interlacing());
        value = render_ctx_->CreateTexture(managed_params);

        ColorProcessorPtr processor = ColorProcessor::Create(color_manager,
                                                             stream.video_colorspace(),
                                                             color_manager->GetReferenceColorSpace());

        render_ctx_->BlitColorManaged(processor, unmanaged_texture,
                                      stream_data.premultiplied_alpha(),
                                      value.get());

        still_image_cache_->mutex()->lock();

        // Put this into the image cache instead
        want_entry->texture = value;
        want_entry->working = false;

        still_image_cache_->wait_cond()->wakeAll();

        still_image_cache_->mutex()->unlock();
      }
    }
  }

  return QVariant::fromValue(value);
}

QVariant RenderProcessor::ProcessAudioFootage(const Footage::StreamReference &stream, const TimeRange &input_time)
{
  QVariant value;

  DecoderPtr decoder = ResolveDecoderFromInput(stream.footage()->decoder(), Decoder::GetCodecStreamFromStreamReference(stream));

  if (decoder) {
    const AudioParams& audio_params = ticket_->property("aparam").value<AudioParams>();

    SampleBufferPtr frame = decoder->RetrieveAudio(input_time, audio_params,
                                                   stream.footage()->project()->cache_path(),
                                                   &IsCancelled());

    if (frame) {
      value = QVariant::fromValue(frame);
    }
  }

  return value;
}

QVariant RenderProcessor::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job)
{
  Q_UNUSED(range)

  QString full_shader_id = QStringLiteral("%1:%2").arg(node->id(), job.GetShaderID());

  QMutexLocker locker(shader_cache_->mutex());

  QVariant shader = shader_cache_->value(full_shader_id);

  if (shader.isNull()) {
    // Since we have shader code, compile it now
    shader = render_ctx_->CreateNativeShader(node->GetShaderCode(job.GetShaderID()));

    if (shader.isNull()) {
      // Couldn't find or build the shader required
      return QVariant();
    }
  }

  VideoParams tex_params = ticket_->property("vparam").value<VideoParams>();

  bool input_textures_have_alpha = false;
  for (auto it=job.GetValues().cbegin(); it!=job.GetValues().cend(); it++) {
    if (it.value().type() == NodeValue::kTexture) {
      TexturePtr tex = it.value().data().value<TexturePtr>();
      if (tex && tex->channel_count() == VideoParams::kRGBAChannelCount) {
        input_textures_have_alpha = true;
        break;
      }
    }
  }

  if (input_textures_have_alpha || job.GetAlphaChannelRequired()) {
    tex_params.set_channel_count(VideoParams::kRGBAChannelCount);
  } else {
    tex_params.set_channel_count(VideoParams::kRGBChannelCount);
  }

  TexturePtr destination = render_ctx_->CreateTexture(tex_params);

  // Run shader
  render_ctx_->BlitToTexture(shader, job, destination.get());

  return QVariant::fromValue(destination);
}

QVariant RenderProcessor::ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job)
{
  if (!job.samples() || !job.samples()->is_allocated()) {
    return QVariant();
  }

  SampleBufferPtr output_buffer = SampleBuffer::CreateAllocated(job.samples()->audio_params(), job.samples()->sample_count());
  NodeValueDatabase value_db;

  const AudioParams& audio_params = ticket_->property("aparam").value<AudioParams>();

  for (int i=0;i<job.samples()->sample_count();i++) {
    // Calculate the exact rational time at this sample
    double sample_to_second = static_cast<double>(i) / static_cast<double>(audio_params.sample_rate());

    rational this_sample_time = rational::fromDouble(range.in().toDouble() + sample_to_second);

    // Update all non-sample and non-footage inputs
    for (auto j=job.GetValues().constBegin(); j!=job.GetValues().constEnd(); j++) {
      NodeValueTable value = ProcessInput(node, j.key(), TimeRange(this_sample_time, this_sample_time));

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

  VideoParams frame_params = ticket_->property("vparam").value<VideoParams>();
  if (job.GetAlphaChannelRequired()) {
    frame_params.set_channel_count(VideoParams::kRGBAChannelCount);
  } else {
    frame_params.set_channel_count(VideoParams::kRGBChannelCount);
  }

  frame->set_video_params(frame_params);
  frame->allocate();

  node->GenerateFrame(frame, job);

  TexturePtr texture = render_ctx_->CreateTexture(frame->video_params(),
                                                  frame->data(),
                                                  frame->linesize_pixels());

  return QVariant::fromValue(texture);
}

QVariant RenderProcessor::GetCachedFrame(const Node *node, const rational &time)
{
  if (!ticket_->property("cache").toString().isEmpty()
      && node->id() == QStringLiteral("org.olivevideoeditor.Olive.videoinput")) {
    const VideoParams& video_params = ticket_->property("vparam").value<VideoParams>();

    QByteArray hash = RenderManager::Hash(node, video_params, time);

    FramePtr f = FrameHashCache::LoadCacheFrame(ticket_->property("cache").toString(), hash);

    if (f) {
      // The cached frame won't load with the correct divider by default, so we enforce it here
      VideoParams p = f->video_params();

      p.set_width(f->width() * video_params.divider());
      p.set_height(f->height() * video_params.divider());
      p.set_divider(video_params.divider());

      f->set_video_params(p);

      TexturePtr texture = render_ctx_->CreateTexture(f->video_params(), f->data(), f->linesize_pixels());
      return QVariant::fromValue(texture);
    }
  }

  return QVariant();
}

QVector2D RenderProcessor::GenerateResolution() const
{
  // Set resolution to the destination to the "logical" resolution of the destination
  const VideoParams& video_params = ticket_->property("vparam").value<VideoParams>();
  return QVector2D(video_params.width() * video_params.pixel_aspect_ratio().toDouble(),
                   video_params.height());
}

}
