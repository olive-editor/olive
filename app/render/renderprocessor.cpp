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
#include "node/block/clip/clip.h"
#include "node/block/transition/transition.h"
#include "node/project/project.h"
#include "rendermanager.h"

namespace olive {

#define super NodeTraverser

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

  NodeValueTable table;
  if (Node* node = Node::ValueToPtr<Node>(ticket_->property("node"))) {
    table = GenerateTable(node, range);
  }

  NodeValue tex_val = table.Get(NodeValue::kTexture);

  ResolveJobs(tex_val, range);

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

  VideoParams::Format frame_format = static_cast<VideoParams::Format>(ticket_->property("format").toInt());
  if (frame_format != VideoParams::kFormatInvalid) {
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

    if (tex_params.effective_width() != frame_params.effective_width()
        || tex_params.effective_height() != frame_params.effective_height()
        || tex_params.format() != frame_params.format()
        || output_color_transform) {
      TexturePtr blit_tex = render_ctx_->CreateTexture(frame_params);

      QMatrix4x4 matrix = ticket_->property("matrix").value<QMatrix4x4>();

      if (output_color_transform) {
        // Yes color transform, blit color managed
        ColorTransformJob job;

        job.SetColorProcessor(output_color_transform);
        job.SetInputTexture(texture);
        job.SetInputAlphaAssociation(OLIVE_CONFIG("ReassocLinToNonLin").toBool() ? kAlphaAssociated : kAlphaNone);
        job.SetTransformMatrix(matrix);

        render_ctx_->BlitColorManaged(job, blit_tex.get());
      } else {
        // No color transform, just blit
        ShaderJob job;
        job.Insert(QStringLiteral("ove_maintex"), NodeValue(NodeValue::kTexture, QVariant::fromValue(texture)));
        job.Insert(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, matrix));

        render_ctx_->BlitToTexture(render_ctx_->GetDefaultShader(), job, blit_tex.get());
      }

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
        RenderManager::ReturnType return_type = RenderManager::ReturnType(ticket_->property("return").toInt());

        FramePtr frame;
        QString cache = ticket_->property("cache").toString();

        if (return_type == RenderManager::kFrame || !cache.isEmpty()) {
          // Convert to CPU frame
          frame = GenerateFrame(texture, time);

          // Save to cache if requested
          if (!cache.isEmpty()) {
            rational timebase = ticket_->property("cachetimebase").value<rational>();
            QUuid uuid = ticket_->property("cacheuuid").value<QUuid>();
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

    NodeValueTable table;
    if (Node* node = Node::ValueToPtr<Node>(ticket_->property("node"))) {
      table = GenerateTable(node, time);
    }

    NodeValue sample_val = table.Get(NodeValue::kSamples);

    ResolveJobs(sample_val, time);

    SampleBuffer samples = sample_val.toSamples();
    if (samples.is_allocated()) {
      samples.clamp();

      if (ticket_->property("enablewaveforms").toBool()) {
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

  if (!decoder.decoder || decoder.last_modified != file_last_modified) {
    // No decoder
    decoder.decoder = Decoder::CreateFromID(decoder_id);
    decoder.last_modified = file_last_modified;
    decoder_cache_->insert(stream, decoder);
    locker.unlock();

    if (!decoder.decoder->Open(stream)) {
      qWarning() << "Failed to open decoder for" << stream.filename()
                 << "::" << stream.stream();
      return nullptr;
    }

    if (!render_ctx_) {
      // Assume dry run and increment access time
      decoder.decoder->IncrementAccessTime(RenderManager::kDryRunInterval.toDouble() * 1000);
    }
  }

  return decoder.decoder;
}

void RenderProcessor::Process(RenderTicketPtr ticket, Renderer *render_ctx, DecoderCache *decoder_cache, ShaderCache *shader_cache)
{
  RenderProcessor p(ticket, render_ctx, decoder_cache, shader_cache);
  p.Run();
}

NodeValueTable RenderProcessor::GenerateBlockTable(const Track *track, const TimeRange &range)
{
  if (track->type() == Track::kAudio) {

    const AudioParams& audio_params = GetCacheAudioParams();

    QVector<Block*> active_blocks = track->BlocksAtTimeRange(range);

    // All these blocks will need to output to a buffer so we create one here
    SampleBuffer block_range_buffer(audio_params, range.length());
    block_range_buffer.silence();

    NodeValueTable merged_table;

    // Loop through active blocks retrieving their audio
    foreach (Block* b, active_blocks) {
      if (dynamic_cast<ClipBlock*>(b) || dynamic_cast<TransitionBlock*>(b)) {
        TimeRange range_for_block(qMax(b->in(), range.in()),
                                  qMin(b->out(), range.out()));

        int destination_offset = audio_params.time_to_samples(range_for_block.in() - range.in());
        int max_dest_sz = audio_params.time_to_samples(range_for_block.length());

        // Destination buffer
        NodeValueTable table = GenerateTable(b, Track::TransformRangeForBlock(b, range_for_block));
        SampleBuffer samples_from_this_block = table.Take(NodeValue::kSamples).toSamples();
        ClipBlock *clip_cast = dynamic_cast<ClipBlock*>(b);

        if (samples_from_this_block.is_allocated()) {
          // If this is a clip, we might have extra speed/reverse information
          if (clip_cast) {
            double speed_value = clip_cast->speed();
            bool reversed = clip_cast->reverse();

            if (qIsNull(speed_value)) {
              // Just silence, don't think there's any other practical application of 0 speed audio
              samples_from_this_block.silence();
            } else if (!qFuzzyCompare(speed_value, 1.0)) {
              if (clip_cast->maintain_audio_pitch()) {
                AudioProcessor processor;

                if (processor.Open(samples_from_this_block.audio_params(), samples_from_this_block.audio_params(), speed_value)) {
                  AudioProcessor::Buffer out;

                  // FIXME: This is not the best way to do this, the TempoProcessor works best
                  //        when it's given a continuous stream of audio, which is challenging
                  //        in our current "modular" audio system. This should still work reasonably
                  //        well on export (assuming audio is all generated at once on export), but
                  //        users may hear clicks and pops in the audio during preview due to this
                  //        approach.
                  int r = processor.Convert(samples_from_this_block.to_raw_ptrs().data(), samples_from_this_block.sample_count(), nullptr);

                  if (r < 0) {
                    qCritical() << "Failed to change tempo of audio:" << r;
                  } else {
                    processor.Flush();

                    processor.Convert(nullptr, 0, &out);

                    if (!out.empty()) {
                      int nb_samples = out.front().size() * samples_from_this_block.audio_params().bytes_per_sample_per_channel();

                      if (nb_samples) {
                        SampleBuffer new_samples(samples_from_this_block.audio_params(), nb_samples);

                        for (int i=0; i<out.size(); i++) {
                          memcpy(new_samples.data(i), out[i].data(), out[i].size());
                        }

                        samples_from_this_block = new_samples;
                      }
                    }
                  }
                }
              } else {
                // Multiply time
                samples_from_this_block.speed(speed_value);
              }
            }

            if (reversed) {
              samples_from_this_block.reverse();
            }
          }

          int copy_length = qMin(max_dest_sz, samples_from_this_block.sample_count());

          // Copy samples into destination buffer
          for (int i=0; i<samples_from_this_block.audio_params().channel_count(); i++) {
            block_range_buffer.set(i, samples_from_this_block.data(i), destination_offset, copy_length);
          }

          NodeValueTable::Merge({merged_table, table});
        }

        // Create block waveforms if requested
        if (ticket_->property("enablewaveforms").toBool() && clip_cast) {
          // Format information for use in the main thread
          RenderedWaveform waveform_info;
          waveform_info.block = clip_cast;
          waveform_info.range = range_for_block - b->in();

          if (!(waveform_info.silence = !samples_from_this_block.is_allocated())) {
            // Generate a visual waveform from the samples acquired from this block
            AudioVisualWaveform visual_waveform;
            visual_waveform.set_channel_count(audio_params.channel_count());
            visual_waveform.OverwriteSamples(samples_from_this_block, audio_params.sample_rate());
            waveform_info.waveform = visual_waveform;
          }

          QVector<RenderedWaveform> waveform_list = ticket_->property("waveforms").value< QVector<RenderedWaveform> >();
          waveform_list.append(waveform_info);
          ticket_->setProperty("waveforms", QVariant::fromValue(waveform_list));
        }
      }
    }

    merged_table.Push(NodeValue::kSamples, QVariant::fromValue(block_range_buffer), track);

    return merged_table;

  } else {
    return super::GenerateBlockTable(track, range);
  }
}

void RenderProcessor::ProcessVideoFootage(TexturePtr destination, const FootageJob &stream, const rational &input_time)
{
  if (ticket_->property("type").value<RenderManager::TicketType>() != RenderManager::kTypeVideo) {
    // Video cannot contribute to audio, so we do nothing here
    return;
  }

  // Check the still frame cache. On large frames such as high resolution still images, uploading
  // and color managing them for every frame is a waste of time, so we implement a small cache here
  // to optimize such a situation
  VideoParams stream_data = stream.video_params();

  ColorManager* color_manager = Node::ValueToPtr<ColorManager>(ticket_->property("colormanager"));

  QString using_colorspace = stream_data.colorspace();

  if (using_colorspace.isEmpty()) {
    // FIXME:
    qWarning() << "HAVEN'T GOTTEN DEFAULT INPUT COLORSPACE";
  }

  Decoder::CodecStream default_codec_stream(stream.filename(), stream_data.stream_index(), GetCurrentBlock());

  QString decoder_id = stream.decoder();

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
      frame_filename = Decoder::TransformImageSequenceFileName(stream.filename(), frame_number);

      // Decoder will close automatically since it's a stream_ptr
      decoder->Open(Decoder::CodecStream(frame_filename, stream_data.stream_index(), GetCurrentBlock()));
    }
    break;
  }
  }

  if (decoder && render_ctx_) {
    Decoder::RetrieveVideoParams p;
    p.divider = stream.video_params().divider();
    p.maximum_format = destination->format();

    if (!IsCancelled()) {
      VideoParams tex_params = stream.video_params();

      if (tex_params.is_valid()) {
        TexturePtr unmanaged_texture = decoder->RetrieveVideo(render_ctx_, (stream_data.video_type() == VideoParams::kVideoTypeVideo) ? input_time : Decoder::kAnyTimecode, p, GetCancelPointer());

        if (unmanaged_texture) {
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

void RenderProcessor::ProcessAudioFootage(SampleBuffer &destination, const FootageJob &stream, const TimeRange &input_time)
{
  DecoderPtr decoder = ResolveDecoderFromInput(stream.decoder(), Decoder::CodecStream(stream.filename(), stream.audio_params().stream_index(), nullptr));

  if (decoder) {
    const AudioParams& audio_params = GetCacheAudioParams();

    Decoder::RetrieveAudioStatus status = decoder->RetrieveAudio(destination,
                                                                 input_time, audio_params,
                                                                 stream.cache_path(),
                                                                 loop_mode(),
                                                                 static_cast<RenderMode::Mode>(ticket_->property("mode").toInt()));

    if (status == Decoder::kWaitingForConform) {
      ticket_->setProperty("incomplete", true);
    }
  }
}

void RenderProcessor::ProcessShader(TexturePtr destination, const Node *node, const TimeRange &range, const ShaderJob &job)
{
  if (!render_ctx_) {
    return;
  }

  QString full_shader_id = QStringLiteral("%1:%2").arg(node->id(), job.GetShaderID());

  QMutexLocker locker(shader_cache_->mutex());

  QVariant shader = shader_cache_->value(full_shader_id);

  if (shader.isNull()) {
    // Since we have shader code, compile it now
    shader = render_ctx_->CreateNativeShader(node->GetShaderCode(job.GetShaderID()));

    if (shader.isNull()) {
      // Couldn't find or build the shader required
      return;
    }
  }

  // Run shader
  render_ctx_->BlitToTexture(shader, job, destination.get());
}

void RenderProcessor::ProcessSamples(SampleBuffer &destination, const Node *node, const TimeRange &range, const SampleJob &job)
{
  if (!job.samples().is_allocated()) {
    return;
  }

  NodeValueRow value_db;

  const AudioParams& audio_params = GetCacheAudioParams();

  for (int i=0;i<job.samples().sample_count();i++) {
    // Calculate the exact rational time at this sample
    double sample_to_second = static_cast<double>(i) / static_cast<double>(audio_params.sample_rate());

    rational this_sample_time = rational::fromDouble(range.in().toDouble() + sample_to_second);

    // Update all non-sample and non-footage inputs
    for (auto j=job.GetValues().constBegin(); j!=job.GetValues().constEnd(); j++) {
      NodeValueTable value = ProcessInput(node, j.key(), TimeRange(this_sample_time, this_sample_time));

      value_db.insert(j.key(), GenerateRowValue(node, j.key(), &value));
    }

    node->ProcessSamples(value_db,
                         job.samples(),
                         destination,
                         i);
  }
}

void RenderProcessor::ProcessColorTransform(TexturePtr destination, const Node *node, const ColorTransformJob &job)
{
  if (!render_ctx_) {
    return;
  }

  render_ctx_->BlitColorManaged(job, destination.get());
}

void RenderProcessor::ProcessFrameGeneration(TexturePtr destination, const Node *node, const GenerateJob &job)
{
  if (!render_ctx_) {
    return;
  }

  FramePtr frame = Frame::Create();

  frame->set_video_params(destination->params());
  frame->allocate();

  node->GenerateFrame(frame, job);

  destination->Upload(frame->data(), frame->linesize_pixels());
}

bool RenderProcessor::CanCacheFrames()
{
  return ticket_->property("type").value<RenderManager::TicketType>() == RenderManager::kTypeVideo;
}

TexturePtr RenderProcessor::CreateTexture(const VideoParams &p)
{
  if (render_ctx_) {
    return render_ctx_->CreateTexture(p);
  } else {
    return super::CreateTexture(p);
  }
}

void RenderProcessor::ConvertToReferenceSpace(TexturePtr destination, TexturePtr source, const QString &input_cs)
{
  if (!render_ctx_) {
    return;
  }

  ColorManager* color_manager = Node::ValueToPtr<ColorManager>(ticket_->property("colormanager"));
  ColorProcessorPtr cp = ColorProcessor::Create(color_manager, input_cs, color_manager->GetReferenceColorSpace());

  ColorTransformJob ctj;

  ctj.SetColorProcessor(cp);
  ctj.SetInputTexture(source);
  ctj.SetInputAlphaAssociation(kAlphaAssociated);

  render_ctx_->BlitColorManaged(ctj, destination.get());
}

}
