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

  virtual TexturePtr ProcessVideoFootage(const FootageJob &stream, const rational &input_time) override;

  virtual SampleBufferPtr ProcessAudioFootage(const FootageJob &stream, const TimeRange &input_time) override;

  virtual TexturePtr ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual SampleBufferPtr ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job) override;

  virtual TexturePtr ProcessFrameGeneration(const Node *node, const GenerateJob& job) override;

  virtual bool CanCacheFrames() override;

  virtual TexturePtr GetCachedTexture(const QByteArray &hash) override;

  virtual void SaveCachedTexture(const QByteArray& hash, TexturePtr texture) override;

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
