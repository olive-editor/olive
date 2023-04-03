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

#ifndef RENDERPROCESSOR_H
#define RENDERPROCESSOR_H

#include "node/block/clip/clip.h"
#include "render/job/cachejob.h"
#include "render/job/colortransformjob.h"
#include "render/job/footagejob.h"
#include "render/job/samplejob.h"
#include "render/job/shaderjob.h"
#include "render/renderer.h"
#include "rendercache.h"
#include "renderticket.h"

namespace olive {

class RenderProcessor
{
public:
  static void Process(RenderTicketPtr ticket, Renderer* render_ctx, DecoderCache* decoder_cache, ShaderCache* shader_cache);

  struct RenderedWaveform {
    const ClipBlock* block;
    AudioVisualWaveform waveform;
    TimeRange range;
    bool silence;
  };

protected:
  void ProcessVideoFootage(TexturePtr destination, const FootageJob *stream, const rational &input_time);

  void ProcessAudioFootage(SampleBuffer &destination, const FootageJob *stream, const TimeRange &input_time);

  void ProcessShader(TexturePtr destination, const ShaderJob *job);

  void ProcessSamples(SampleBuffer &destination, const SampleJob &job);

  void ProcessColorTransform(TexturePtr destination, const ColorTransformJob *job);

  void ProcessFrameGeneration(TexturePtr destination, const GenerateJob *job);

  TexturePtr ProcessVideoCacheJob(const CacheJob *val);

  TexturePtr CreateTexture(const VideoParams &p);
  TexturePtr CreateDummyTexture(const VideoParams &p);

  SampleBuffer CreateSampleBuffer(const AudioParams &params, int sample_count)
  {
    return SampleBuffer(params, sample_count);
  }

  SampleBuffer CreateSampleBuffer(const AudioParams &params, const rational &length)
  {
    if (params.is_valid()) {
      return CreateSampleBuffer(params, params.time_to_samples(length));
    } else {
      return SampleBuffer();
    }
  }

  void ConvertToReferenceSpace(TexturePtr destination, TexturePtr source, const QString &input_cs);

  bool UseCache() const;

private:
  RenderProcessor(RenderTicketPtr ticket, Renderer* render_ctx, DecoderCache* decoder_cache, ShaderCache* shader_cache);

  TexturePtr GenerateTexture(const rational& time, const rational& frame_length);

  FramePtr GenerateFrame(TexturePtr texture, const rational &time);

  void Run();

  DecoderPtr ResolveDecoderFromInput(const QString &decoder_id, const Decoder::CodecStream& stream);

  void ResolveJobs(value_t &value);

  const VideoParams &GetCacheVideoParams() const { return vparam_; }
  const AudioParams &GetCacheAudioParams() const { return aparam_; }
  void SetCacheVideoParams(const VideoParams &vparam) { vparam_ = vparam; }
  void SetCacheAudioParams(const AudioParams &aparam) { aparam_ = aparam; }
  bool IsCancelled() const { return cancel_atom_ && cancel_atom_->IsCancelled(); }
  bool HeardCancel() const { return cancel_atom_ && cancel_atom_->HeardCancel(); }
  CancelAtom *GetCancelPointer() const { return cancel_atom_; }
  void SetCancelPointer(CancelAtom *p) { cancel_atom_ = p; }
  Block *GetCurrentBlock() const { return nullptr; }

  RenderTicketPtr ticket_;

  Renderer* render_ctx_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

  VideoParams vparam_;
  AudioParams aparam_;
  CancelAtom *cancel_atom_;

  QHash<Texture*, TexturePtr> resolved_texture_cache_;

};

}

Q_DECLARE_METATYPE(olive::RenderProcessor::RenderedWaveform)

#endif // RENDERPROCESSOR_H
