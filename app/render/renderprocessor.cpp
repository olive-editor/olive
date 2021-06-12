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

#include "renderprocessor.h"

#include <QOpenGLContext>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "node/project/project.h"
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

TexturePtr RenderProcessor::GenerateTexture(const rational &time, const rational &frame_length)
{
  ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"));

  NodeValueTable table;
  NodeOutput texture_output = viewer->GetConnectedTextureOutput();
  if (texture_output.IsValid()) {
    table = GenerateTable(texture_output.node(), texture_output.output(),
                          TimeRange(time, time + frame_length));
  }

  return table.Get(NodeValue::kTexture).value<TexturePtr>();
}

FramePtr RenderProcessor::GenerateFrame(TexturePtr texture, const rational& time)
{
  // Set up output frame parameters
  VideoParams frame_params = GetCacheVideoParams();

  QSize frame_size = ticket_->property("size").value<QSize>();
  if (!frame_size.isNull()) {
    frame_params.set_width(frame_size.width());
    frame_params.set_height(frame_size.height());
  }

  VideoParams::Format frame_format = static_cast<VideoParams::Format>(ticket_->property("format").toInt());
  if (frame_format != VideoParams::kFormatInvalid) {
    frame_params.set_format(frame_format);
  }

  frame_params.set_channel_count(texture ? texture->channel_count() : VideoParams::kRGBChannelCount);

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

  return frame;
}

void RenderProcessor::Run()
{
  // Depending on the render ticket type, start a job
  RenderManager::TicketType type = ticket_->property("type").value<RenderManager::TicketType>();

  switch (type) {
  case RenderManager::kTypeVideo:
  {
    SetCacheVideoParams(ticket_->property("vparam").value<VideoParams>());
    rational time = ticket_->property("time").value<rational>();

    rational frame_length = GetCacheVideoParams().frame_rate_as_time_base();
    if (GetCacheVideoParams().interlacing() != VideoParams::kInterlaceNone) {
      frame_length /= 2;
    }

    TexturePtr texture = GenerateTexture(time, frame_length);

    if (GetCacheVideoParams().interlacing() != VideoParams::kInterlaceNone) {
      // Get next between frame and interlace it
      TexturePtr top = texture;
      TexturePtr bottom = GenerateTexture(time + frame_length, frame_length);

      if (GetCacheVideoParams().interlacing() == VideoParams::kInterlacedBottomFirst) {
        std::swap(top, bottom);
      }

      texture = render_ctx_->InterlaceTexture(top, bottom, GetCacheVideoParams());
    }

    if (ticket_->property("textureonly").toBool()) {
      // Return GPU texture
      if (!texture) {
        texture = render_ctx_->CreateTexture(GetCacheVideoParams());
      }

      render_ctx_->Flush();

      ticket_->Finish(QVariant::fromValue(texture));
    } else {
      // Convert to CPU frame
      FramePtr frame = GenerateFrame(texture, time);

      ticket_->Finish(QVariant::fromValue(frame));
    }
    break;
  }
  case RenderManager::kTypeAudio:
  {
    ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket_->property("viewer"));
    TimeRange time = ticket_->property("time").value<TimeRange>();

    NodeValueTable table;
    NodeOutput texture_output = viewer->GetConnectedSampleOutput();
    if (texture_output.IsValid()) {
      table = GenerateTable(texture_output.node(), texture_output.output(), time);
    }

    QVariant sample_variant = table.Get(NodeValue::kSamples);
    SampleBufferPtr samples = sample_variant.value<SampleBufferPtr>();
    if (samples && ticket_->property("enablewaveforms").toBool()) {
      AudioVisualWaveform vis;
      vis.set_channel_count(samples->audio_params().channel_count());
      vis.OverwriteSamples(samples, samples->audio_params().sample_rate());
      ticket_->setProperty("waveform", QVariant::fromValue(vis));
    }

    ticket_->Finish(sample_variant);
    break;
  }
  case RenderManager::kTypeVideoDownload:
  {
    QString cache = ticket_->property("cache").toString();
    FramePtr frame = ticket_->property("frame").value<FramePtr>();
    QByteArray hash = ticket_->property("hash").toByteArray();

    ticket_->Finish(FrameHashCache::SaveCacheFrame(cache, hash, frame));
    break;
  }
  default:
    // Fail
    ticket_->Finish();
  }
}

DecoderPtr RenderProcessor::ResolveDecoderFromInput(const QString& decoder_id, const Decoder::CodecStream &stream)
{
  if (!stream.IsValid()) {
    qWarning() << "Attempted to resolve the decoder of a null stream";
    return nullptr;
  }

  QMutexLocker locker(decoder_cache_->mutex());

  DecoderPair decoder = decoder_cache_->value(stream);

  qint64 file_last_modified = QFileInfo(stream.filename()).lastModified().toMSecsSinceEpoch();

  if (!decoder.decoder || decoder.last_modified != file_last_modified) {
    // No decoder
    decoder.decoder = Decoder::CreateFromID(decoder_id);
    decoder.last_modified = file_last_modified;

    if (decoder.decoder->Open(stream)) {
      decoder_cache_->insert(stream, decoder);
    } else {
      qWarning() << "Failed to open decoder for" << stream.filename()
                 << "::" << stream.stream();
      return nullptr;
    }
  }

  return decoder.decoder;
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

      double speed_value = b->GetStandardValue(Block::kSpeedInput).toDouble();

      if (qIsNull(speed_value)) {
        // Just silence, don't think there's any other practical application of 0 speed audio
        samples_from_this_block->fill(0);
      } else if (!qFuzzyCompare(speed_value, 1.0)) {
        // Multiply time
        samples_from_this_block->speed(speed_value);
      }

      if (b->GetStandardValue(Block::kReverseInput).toBool()) {
        samples_from_this_block->reverse();
      }

      int copy_length = qMin(max_dest_sz, samples_from_this_block->sample_count());

      // Copy samples into destination buffer
      for (int i=0; i<samples_from_this_block->audio_params().channel_count(); i++) {
        block_range_buffer->set(i, samples_from_this_block->data(i), destination_offset, copy_length);
      }

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

QVariant RenderProcessor::ProcessVideoFootage(const FootageJob &stream, const rational &input_time)
{
  if (ticket_->property("type").value<RenderManager::TicketType>() != RenderManager::kTypeVideo) {
    // Video cannot contribute to audio, so we do nothing here
    return QVariant();
  }

  TexturePtr value = nullptr;

  // Check the still frame cache. On large frames such as high resolution still images, uploading
  // and color managing them for every frame is a waste of time, so we implement a small cache here
  // to optimize such a situation
  const VideoParams& render_params = GetCacheVideoParams();
  VideoParams stream_data = stream.video_params();

  ColorManager* color_manager = Node::ValueToPtr<ColorManager>(ticket_->property("colormanager"));

  // See if we can make this divider larger (i.e. if the fooage is smaller)
  int footage_divider = render_params.divider();
  while (footage_divider > 1
         && VideoParams::GetScaledDimension(stream_data.width(), footage_divider-1) < render_params.effective_width()
         && VideoParams::GetScaledDimension(stream_data.height(), footage_divider-1) < render_params.effective_height()) {
    footage_divider--;
  }

  QString using_colorspace = stream_data.colorspace();

  if (using_colorspace.isEmpty()) {
    // FIXME:
    qWarning() << "HAVEN'T GOTTEN DEFAULT INPUT COLORSPACE";
  }

  Decoder::CodecStream default_codec_stream(stream.filename(), stream_data.stream_index());

  StillImageCache::EntryPtr want_entry = std::make_shared<StillImageCache::Entry>(
        nullptr,
        default_codec_stream,
        ColorProcessor::GenerateID(color_manager, using_colorspace, color_manager->GetReferenceColorSpace()),
        stream_data.premultiplied_alpha(),
        footage_divider,
        (stream_data.video_type() == VideoParams::kVideoTypeStill) ? 0 : input_time,
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

    QString decoder_id = stream.decoder();

    DecoderPtr decoder = nullptr;

    if (stream_data.video_type() == VideoParams::kVideoTypeVideo) {
      decoder = ResolveDecoderFromInput(decoder_id, default_codec_stream);
    } else {
      // Since image sequences involve multiple files, we don't engage the decoder cache
      decoder = Decoder::CreateFromID(decoder_id);

      QString frame_filename;

      if (stream_data.video_type() == VideoParams::kVideoTypeImageSequence) {
        int64_t frame_number = stream_data.get_time_in_timebase_units(input_time);
        frame_filename = Decoder::TransformImageSequenceFileName(stream.filename(), frame_number);
      } else {
        frame_filename = stream.filename();
      }

      // Decoder will close automatically since it's a stream_ptr
      decoder->Open(Decoder::CodecStream(frame_filename, stream_data.stream_index()));
    }

    if (decoder) {
      Decoder::RetrieveVideoParams p;
      p.divider = footage_divider;
      p.src_interlacing = stream_data.interlacing();
      p.dst_interlacing = GetCacheVideoParams().interlacing();

      FramePtr frame = decoder->RetrieveVideo((stream_data.video_type() == VideoParams::kVideoTypeVideo) ? input_time : Decoder::kAnyTimecode, p);

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
                                                             using_colorspace,
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

QVariant RenderProcessor::ProcessAudioFootage(const FootageJob &stream, const TimeRange &input_time)
{
  QVariant value;

  DecoderPtr decoder = ResolveDecoderFromInput(stream.decoder(), Decoder::CodecStream(stream.filename(), stream.audio_params().stream_index()));

  if (decoder) {
    const AudioParams& audio_params = ticket_->property("aparam").value<AudioParams>();

    Decoder::RetrieveAudioData status = decoder->RetrieveAudio(input_time, audio_params,
                                                               stream.cache_path(),
                                                               stream.loop_mode(),
                                                               static_cast<RenderMode::Mode>(ticket_->property("mode").toInt()));

    if (status.status == Decoder::kOK && status.samples) {
      value = QVariant::fromValue(status.samples);
    } else if (status.status == Decoder::kWaitingForConform) {
      ticket_->setProperty("incomplete", true);
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

  VideoParams tex_params = GetCacheVideoParams();

  tex_params.set_channel_count(GetChannelCountFromJob(job));

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

  VideoParams frame_params = GetCacheVideoParams();
  frame_params.set_channel_count(GetChannelCountFromJob(job));
  frame->set_video_params(frame_params);
  frame->allocate();

  node->GenerateFrame(frame, job);

  TexturePtr texture = render_ctx_->CreateTexture(frame->video_params(),
                                                  frame->data(),
                                                  frame->linesize_pixels());

  return QVariant::fromValue(texture);
}

bool RenderProcessor::CanCacheFrames()
{
  return ticket_->property("type").value<RenderManager::TicketType>() == RenderManager::kTypeVideo;
}

QVariant RenderProcessor::GetCachedTexture(const QByteArray& hash)
{
  QString cache_dir = ticket_->property("cache").toString();
  if (cache_dir.isEmpty()) {
    return QVariant();
  }

  FramePtr f = FrameHashCache::LoadCacheFrame(cache_dir, hash);

  if (f) {
    TexturePtr texture = render_ctx_->CreateTexture(f->video_params(), f->data(), f->linesize_pixels());
    return QVariant::fromValue(texture);
  }

  return QVariant();
}

void RenderProcessor::SaveCachedTexture(const QByteArray &hash, const QVariant &tex_var)
{
  // FIXME: Temporarily disabled because I don't know how to ensure that the frame saved here is
  //        not the main frame. If it is, it'll be saved twice which will waste a lot of cycles.
  //        At least disabled, the frame will still save, and if nothing else alters the hash, it
  //        will pick up automatically from GetCachedTexture.
  /*if (!tex_var.isNull()) {
    QString cache_dir = ticket_->property("cache").toString();

    if (!cache_dir.isEmpty()) {
      TexturePtr texture = tex_var.value<TexturePtr>();
      FramePtr frame = Frame::Create();
      frame->set_video_params(texture->params());
      frame->allocate();
      render_ctx_->DownloadFromTexture(texture.get(), frame->data(), frame->linesize_pixels());
      FrameHashCache::SaveCacheFrame(cache_dir, hash, frame);
      qDebug() << "Saved mid-render frame to cache";
    }
  }*/
}

}
