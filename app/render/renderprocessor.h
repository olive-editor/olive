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

#ifndef RENDERPROCESSOR_H
#define RENDERPROCESSOR_H

#include "node/block/clip/clip.h"
#include "node/traverser.h"
#include "render/renderer.h"
#include "rendercache.h"
#include "threading/threadticket.h"

namespace olive {

class RenderProcessor : public NodeTraverser
{
public:
  static void Process(RenderTicketPtr ticket, Renderer* render_ctx, DecoderCache* decoder_cache, ShaderCache* shader_cache, QVariant default_shader);

  struct RenderedWaveform {
    const ClipBlock* block;
    AudioVisualWaveform waveform;
    TimeRange range;
    bool silence;
  };

protected:
  virtual NodeValueTable GenerateBlockTable(const Track *track, const TimeRange &range) override;

  virtual void ProcessVideoFootage(TexturePtr destination, const FootageJob &stream, const rational &input_time) override;

  virtual void ProcessAudioFootage(SampleBufferPtr destination, const FootageJob &stream, const TimeRange &input_time) override;

  virtual void ProcessShader(TexturePtr destination, const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual void ProcessSamples(SampleBufferPtr destination, const Node *node, const TimeRange &range, const SampleJob &job) override;

  virtual void ProcessColorTransform(TexturePtr destination, const Node *node, const ColorTransformJob& job) override;

  virtual void ProcessFrameGeneration(TexturePtr destination, const Node *node, const GenerateJob& job) override;

  virtual bool CanCacheFrames() override;

  virtual TexturePtr CreateTexture(const VideoParams &p) override
  {
    return render_ctx_->CreateTexture(p);
  }

  virtual SampleBufferPtr CreateSampleBuffer(const AudioParams &params, int sample_count) override
  {
    return SampleBuffer::CreateAllocated(params, sample_count);
  }

private:
  RenderProcessor(RenderTicketPtr ticket, Renderer* render_ctx, DecoderCache* decoder_cache, ShaderCache* shader_cache, QVariant default_shader);

  TexturePtr GenerateTexture(const rational& time, const rational& frame_length);

  FramePtr GenerateFrame(TexturePtr texture, const rational &time);

  void Run();

  DecoderPtr ResolveDecoderFromInput(const QString &decoder_id, const Decoder::CodecStream& stream);

  RenderTicketPtr ticket_;

  Renderer* render_ctx_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

  QVariant default_shader_;

};

}

Q_DECLARE_METATYPE(olive::RenderProcessor::RenderedWaveform)

#endif // RENDERPROCESSOR_H
