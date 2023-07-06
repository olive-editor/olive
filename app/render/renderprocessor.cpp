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

#include "renderprocessor.h"

#include <QOpenGLContext>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "audio/audioprocessor.h"
#include "common/qtutils.h"
#include "node/block/clip/clip.h"
#include "node/block/transition/transition.h"
#include "node/project.h"
#include "rendermanager.h"

namespace olive {

RenderProcessor::RenderProcessor(RenderTicketPtr ticket, Renderer *render_ctx, DecoderCache* decoder_cache, ShaderCache *shader_cache) :
  ticket_(ticket),
  render_ctx_(render_ctx),
  decoder_cache_(decoder_cache),
  shader_cache_(shader_cache)
{
}

TexturePtr RenderProcessor::GenerateTexture(const rational &time, const rational &frame_length)
{
  TimeRange range = TimeRange(time, time + frame_length);

  value_t tex_val;
  if (Node* node = QtUtils::ValueToPtr<Node>(ticket_->property("node"))) {
    ValueParams::Cache cache;
    ValueParams vp(GetCacheVideoParams(), GetCacheAudioParams(), range, ticket_->property("output").toString(), LoopMode::kLoopModeOff, GetCancelPointer(), &cache);
    tex_val = node->Value(vp);
  }

  ResolveJobs(tex_val);

  return tex_val.toTexture();
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

  PixelFormat frame_format = static_cast<PixelFormat::Format>(ticket_->property("format").toInt());
  if (frame_format != PixelFormat::INVALID) {
    frame_params.set_format(frame_format);
  }

  int force_channel_count = ticket_->property("channelcount").toInt();
  if (force_channel_count != 0) {
    frame_params.set_channel_count(force_channel_count);
  } else {
    frame_params.set_channel_count(texture ? texture->channel_count() : VideoParams::kRGBAChannelCount);
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

    if (output_color_transform) {
      TexturePtr transform_tex = render_ctx_->CreateTexture(tex_params);
      ColorTransformJob job;

      job.SetColorProcessor(output_color_transform);
      job.SetInputTexture(texture);
      job.SetInputAlphaAssociation(OLIVE_CONFIG("ReassocLinToNonLin").toBool() ? kAlphaAssociated : kAlphaNone);

      render_ctx_->BlitColorManaged(job, transform_tex.get());

      texture = transform_tex;
    }

    if (tex_params.effective_width() != frame_params.effective_width()
        || tex_params.effective_height() != frame_params.effective_height()
        || tex_params.format() != frame_params.format()) {
      TexturePtr blit_tex = render_ctx_->CreateTexture(frame_params);

      QMatrix4x4 matrix = ticket_->property("matrix").value<QMatrix4x4>();

      // No color transform, just blit
      ShaderJob job;
      job.Insert(QStringLiteral("ove_maintex"), texture);
      job.Insert(QStringLiteral("ove_mvpmat"), matrix);

      render_ctx_->BlitToTexture(render_ctx_->GetDefaultShader(), job, blit_tex.get());

      // Replace texture that we're going to download in the next step
      texture = blit_tex;
    }

    render_ctx_->Flush();

    render_ctx_->DownloadFromTexture(texture->id(), texture->params(), frame->data(), frame->linesize_pixels());
  }

  return frame;
}

void RenderProcessor::Run()
{
  // Depending on the render ticket type, start a job
  RenderManager::TicketType type = ticket_->property("type").value<RenderManager::TicketType>();

  SetCancelPointer(ticket_->GetCancelAtom());

  SetCacheVideoParams(ticket_->property("vparam").value<VideoParams>());
  SetCacheAudioParams(ticket_->property("aparam").value<AudioParams>());

  if (IsCancelled()) {
    ticket_->Finish();
    return;
  }

  switch (type) {
  case RenderManager::kTypeVideo:
  {
    rational time = ticket_->property("time").value<rational>();

    rational frame_length = GetCacheVideoParams().frame_rate_as_time_base();
    if (GetCacheVideoParams().interlacing() != VideoParams::kInterlaceNone) {
      frame_length /= 2;
    }

    TexturePtr texture = GenerateTexture(time, frame_length);

    if (!render_ctx_) {
      ticket_->Finish();
    } else {
      if (GetCacheVideoParams().interlacing() != VideoParams::kInterlaceNone) {
        // Get next between frame and interlace it
        TexturePtr top = texture;
        TexturePtr bottom = GenerateTexture(time + frame_length, frame_length);

        if (GetCacheVideoParams().interlacing() == VideoParams::kInterlacedBottomFirst) {
          std::swap(top, bottom);
        }

        texture = render_ctx_->InterlaceTexture(top, bottom, GetCacheVideoParams());
      }

      if (HeardCancel()) {
        // Finish cancelled ticket with nothing since we can't guarantee the frame we generated
        // is actually "complete
        ticket_->Finish();
      } else {
        FramePtr frame;
        QString cache = ticket_->property("cache").toString();
        RenderManager::ReturnType return_type = RenderManager::ReturnType(ticket_->property("return").toInt());

        if (return_type == RenderManager::kFrame || !cache.isEmpty()) {
          // Convert to CPU frame
          frame = GenerateFrame(texture, time);

          // Save to cache if requested
          if (!cache.isEmpty()) {
            rational timebase = ticket_->property("cachetimebase").value<rational>();
            QUuid uuid = ticket_->property("cacheid").value<QUuid>();
            bool cache_result = FrameHashCache::SaveCacheFrame(cache, uuid, time, timebase, frame);
            ticket_->setProperty("cached", cache_result);
          }
        }

        if (return_type == RenderManager::kTexture) {
          // Return GPU texture
          if (!texture) {
            texture = render_ctx_->CreateTexture(GetCacheVideoParams());
            render_ctx_->ClearDestination(texture.get());
          }

          render_ctx_->Flush();

          ticket_->Finish(QVariant::fromValue(texture));
        } else {
          ticket_->Finish(QVariant::fromValue(frame));
        }
      }
    }
    break;
  }
  case RenderManager::kTypeAudio:
  {
    TimeRange time = ticket_->property("time").value<TimeRange>();

    value_t sample_val;
    if (Node* node = QtUtils::ValueToPtr<Node>(ticket_->property("node"))) {
      ValueParams::Cache cache;
      ValueParams vp(GetCacheVideoParams(), GetCacheAudioParams(), time, ticket_->property("output").toString(), LoopMode::kLoopModeOff, GetCancelPointer(), &cache);
      sample_val = node->Value(vp);
    }

    ResolveJobs(sample_val);

    SampleBuffer samples = sample_val.toSamples();
    if (samples.is_allocated()) {
      if (ticket_->property("clamp").toBool() && !IsCancelled()) {
        samples.clamp();
      }

      if (ticket_->property("enablewaveforms").toBool() && !IsCancelled()) {
        AudioVisualWaveform vis;
        vis.set_channel_count(samples.audio_params().channel_count());
        vis.OverwriteSamples(samples, samples.audio_params().sample_rate());
        ticket_->setProperty("waveform", QVariant::fromValue(vis));
      }
    }

    if (HeardCancel()) {
      ticket_->Finish();
    } else {
      ticket_->Finish(QVariant::fromValue(samples));
    }
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

  DecoderPtr dec = nullptr;

  if (decoder.decoder && decoder.last_modified == file_last_modified) {
    dec = decoder.decoder;
  } else {
    // No decoder
    decoder.decoder = dec = Decoder::CreateFromID(decoder_id);
    decoder.last_modified = file_last_modified;
    decoder_cache_->insert(stream, decoder);
    locker.unlock();

    if (!dec->Open(stream)) {
      qWarning() << "Failed to open decoder for" << stream.filename()
                 << "::" << stream.stream();
      return nullptr;
    }

    if (!render_ctx_) {
      // Assume dry run and increment access time
      decoder.decoder->IncrementAccessTime(RenderManager::kDryRunInterval.toDouble() * 1000);
    }
  }

  return dec;
}

void RenderProcessor::Process(RenderTicketPtr ticket, Renderer *render_ctx, DecoderCache *decoder_cache, ShaderCache *shader_cache)
{
  RenderProcessor p(ticket, render_ctx, decoder_cache, shader_cache);
  p.Run();
}

void RenderProcessor::ProcessVideoFootage(TexturePtr destination, const FootageJob *stream, const rational &input_time)
{
  if (ticket_->property("type").value<RenderManager::TicketType>() != RenderManager::kTypeVideo) {
    // Video cannot contribute to audio, so we do nothing here
    return;
  }

  // Check the still frame cache. On large frames such as high resolution still images, uploading
  // and color managing them for every frame is a waste of time, so we implement a small cache here
  // to optimize such a situation
  VideoParams stream_data = stream->video_params();

  ColorManager* color_manager = QtUtils::ValueToPtr<ColorManager>(ticket_->property("colormanager"));

  QString using_colorspace = stream_data.colorspace();

  if (using_colorspace.isEmpty()) {
    // FIXME:
    qWarning() << "HAVEN'T GOTTEN DEFAULT INPUT COLORSPACE";
  }

  Decoder::CodecStream default_codec_stream(stream->filename(), stream_data.stream_index(), nullptr);

  QString decoder_id = stream->decoder();

  DecoderPtr decoder = nullptr;

  switch (stream_data.video_type()) {
  case VideoParams::kVideoTypeVideo:
  case VideoParams::kVideoTypeStill:
    decoder = ResolveDecoderFromInput(decoder_id, default_codec_stream);
    break;
  case VideoParams::kVideoTypeImageSequence:
  {
    if (render_ctx_) {
      // Since image sequences involve multiple files, we don't engage the decoder cache
      decoder = Decoder::CreateFromID(decoder_id);

      QString frame_filename;

      int64_t frame_number = stream_data.get_time_in_timebase_units(input_time);
      frame_filename = Decoder::TransformImageSequenceFileName(stream->filename(), frame_number);

      // Decoder will close automatically since it's a stream_ptr
      decoder->Open(Decoder::CodecStream(frame_filename, stream_data.stream_index(), GetCurrentBlock()));
    }
    break;
  }
  }

  if (decoder && render_ctx_) {
    Decoder::RetrieveVideoParams p;
    p.divider = stream->video_params().divider();
    p.maximum_format = destination->format();

    if (!IsCancelled()) {
      VideoParams tex_params = stream->video_params();

      if (tex_params.is_valid()) {
        TexturePtr unmanaged_texture;

        p.renderer = render_ctx_;
        p.time = (stream_data.video_type() == VideoParams::kVideoTypeVideo) ? input_time : Decoder::kAnyTimecode;
        p.cancelled = GetCancelPointer();
        p.force_range = stream_data.color_range();
        p.src_interlacing = stream_data.interlacing();

        unmanaged_texture = decoder->RetrieveVideo(p);

        if (!IsCancelled() && unmanaged_texture) {
          // We convert to our rendering pixel format, since that will always be float-based which
          // is necessary for correct color conversion
          ColorProcessorPtr processor = ColorProcessor::Create(color_manager,
                                                               using_colorspace,
                                                               color_manager->GetReferenceColorSpace());

          ColorTransformJob job;

          job.SetColorProcessor(processor);
          job.SetInputTexture(unmanaged_texture);

          if (stream_data.channel_count() != VideoParams::kRGBAChannelCount
              || stream_data.colorspace() == color_manager->GetReferenceColorSpace()) {
            job.SetInputAlphaAssociation(kAlphaNone);
          } else if (stream_data.premultiplied_alpha()) {
            job.SetInputAlphaAssociation(kAlphaAssociated);
          } else {
            job.SetInputAlphaAssociation(kAlphaUnassociated);
          }

          render_ctx_->BlitColorManaged(job, destination.get());
        }
      }
    }
  }
}

void RenderProcessor::ProcessAudioFootage(SampleBuffer &destination, const FootageJob *stream, const TimeRange &input_time)
{
  DecoderPtr decoder = ResolveDecoderFromInput(stream->decoder(), Decoder::CodecStream(stream->filename(), stream->audio_params().stream_index(), GetCurrentBlock()));

  if (decoder) {
    Decoder::RetrieveAudioStatus status = decoder->RetrieveAudio(destination,
                                                                 input_time.in(),
                                                                 stream->cache_path(),
                                                                 stream->loop_mode(),
                                                                 static_cast<RenderMode::Mode>(ticket_->property("mode").toInt()));

    if (status == Decoder::kWaitingForConform) {
      ticket_->setProperty("incomplete", true);
    }
  }
}

void RenderProcessor::ProcessShader(TexturePtr destination, const ShaderJob *job)
{
  if (!render_ctx_) {
    return;
  }

  QString full_shader_id = QStringLiteral("%1:%2").arg(QString::number(reinterpret_cast<quintptr>(job->function())), job->GetShaderID());

  QMutexLocker locker(shader_cache_->mutex());

  QVariant shader = shader_cache_->value(full_shader_id);

  if (shader.isNull()) {
    // Since we have shader code, compile it now
    shader = render_ctx_->CreateNativeShader(job->do_function());

    if (shader.isNull()) {
      // Couldn't find or build the shader required
      return;
    }

    shader_cache_->insert(full_shader_id, shader);
  }

  locker.unlock();

  // Run shader
  render_ctx_->BlitToTexture(shader, *job, destination.get());
}

void RenderProcessor::ProcessSamples(SampleBuffer &destination, const SampleJob &job)
{
  job.do_function(destination);
}

void RenderProcessor::ProcessColorTransform(TexturePtr destination, const ColorTransformJob *job)
{
  if (!render_ctx_) {
    return;
  }

  render_ctx_->BlitColorManaged(*job, destination.get());
}

void RenderProcessor::ProcessFrameGeneration(TexturePtr destination, const GenerateJob *job)
{
  if (!render_ctx_) {
    return;
  }

  FramePtr frame = Frame::Create();

  frame->set_video_params(destination->params());
  frame->allocate();

  job->do_function(frame);

  destination->Upload(frame->data(), frame->linesize_pixels());
}

TexturePtr RenderProcessor::ProcessVideoCacheJob(const CacheJob *val)
{
  FramePtr frame = FrameHashCache::LoadCacheFrame(val->GetFilename());
  if (frame) {
    TexturePtr tex = CreateTexture(frame->video_params());
    if (tex) {
      tex->Upload(frame->data(), frame->linesize_pixels());
      return tex;
    }
  } else {
    QStringList s = ticket_->property("badcache").toStringList();
    s.append(val->GetFilename());
    ticket_->setProperty("badcache", s);
  }

  return nullptr;
}

TexturePtr RenderProcessor::CreateTexture(const VideoParams &p)
{
  if (render_ctx_) {
    // Create real texture with render context
    return render_ctx_->CreateTexture(p);
  } else {
    // Create dummy texture
    return CreateDummyTexture(p);
  }
}

TexturePtr RenderProcessor::CreateDummyTexture(const VideoParams &p)
{
  return std::make_shared<Texture>(p);
}

void RenderProcessor::ConvertToReferenceSpace(TexturePtr destination, TexturePtr source, const QString &input_cs)
{
  if (!render_ctx_) {
    return;
  }

  ColorManager* color_manager = QtUtils::ValueToPtr<ColorManager>(ticket_->property("colormanager"));
  ColorProcessorPtr cp = ColorProcessor::Create(color_manager, input_cs, color_manager->GetReferenceColorSpace());

  ColorTransformJob ctj;

  ctj.SetColorProcessor(cp);
  ctj.SetInputTexture(source);
  ctj.SetInputAlphaAssociation(kAlphaAssociated);

  render_ctx_->BlitColorManaged(ctj, destination.get());
}

bool RenderProcessor::UseCache() const
{
  return static_cast<RenderMode::Mode>(ticket_->property("mode").toInt()) == RenderMode::kOffline;
}

void RenderProcessor::ResolveJobs(value_t &val)
{
  if (val.type() == TYPE_TEXTURE) {

    if (TexturePtr job_tex = val.toTexture()) {
      if (AcceleratedJob *base_job = job_tex->job()) {

        if (resolved_texture_cache_.contains(job_tex.get())) {
          val = resolved_texture_cache_.value(job_tex.get());
        } else {
          // Resolve any sub-jobs
          for (auto it=base_job->GetValues().begin(); it!=base_job->GetValues().end(); it++) {
            // Jobs will almost always be submitted with one of these types
            value_t &subval = it.value();
            ResolveJobs(subval);
          }

          if (CacheJob *cj = dynamic_cast<CacheJob*>(base_job)) {
            TexturePtr tex = ProcessVideoCacheJob(cj);
            if (tex) {
              val = tex;
            } else {
              val = cj->GetFallback();
            }

          } else if (ColorTransformJob *ctj = dynamic_cast<ColorTransformJob*>(base_job)) {

            VideoParams ctj_params = job_tex->params();

            ctj_params.set_format(GetCacheVideoParams().format());

            TexturePtr dest = CreateTexture(ctj_params);

            // Resolve input texture
            value_t v = ctj->GetInputTexture();
            ResolveJobs(v);
            ctj->SetInputTexture(v);

            ProcessColorTransform(dest, ctj);

            val = dest;

          } else if (ShaderJob *sj = dynamic_cast<ShaderJob*>(base_job)) {

            VideoParams tex_params = job_tex->params();

            TexturePtr tex = CreateTexture(tex_params);

            ProcessShader(tex, sj);

            val = tex;

          } else if (GenerateJob *gj = dynamic_cast<GenerateJob*>(base_job)) {

            VideoParams tex_params = job_tex->params();

            TexturePtr tex = CreateTexture(tex_params);

            ProcessFrameGeneration(tex, gj);

            // Convert to reference space
            const QString &colorspace = tex_params.colorspace();
            if (!colorspace.isEmpty()) {
              // Set format to primary format
              tex_params.set_format(GetCacheVideoParams().format());

              TexturePtr dest = CreateTexture(tex_params);

              ConvertToReferenceSpace(dest, tex, colorspace);

              tex = dest;
            }

            val = tex;

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

            val = tex;

          }

          // Cache resolved value
          resolved_texture_cache_.insert(job_tex.get(), val.toTexture());
        }
      }
    }

  } else if (val.type() == TYPE_SAMPLES) {

    if (AudioJobPtr job = val.toAudioJob()) {
      if (resolved_sample_cache_.contains(job.get())) {

        val = resolved_sample_cache_.value(job.get());

      } else {

        if (SampleJob *sjob = dynamic_cast<SampleJob *>(job->job())) {

          for (auto it=sjob->GetValues().begin(); it!=sjob->GetValues().end(); it++) {
            // Jobs will almost always be submitted with one of these types
            value_t &subval = it.value();
            ResolveJobs(subval);
          }

          SampleBuffer output_buffer = CreateSampleBuffer(sjob->audio_params(), sjob->sample_count());
          ProcessSamples(output_buffer, *sjob);
          val = output_buffer;

        } else if (FootageJob *fjob = dynamic_cast<FootageJob *>(job->job())) {

          SampleBuffer buffer = CreateSampleBuffer(GetCacheAudioParams(), fjob->time().length());
          ProcessAudioFootage(buffer, fjob, fjob->time());
          val = buffer;

        }

        resolved_sample_cache_.insert(job.get(), val);

      }
    }
  }
}

}
