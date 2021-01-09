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

#ifndef RENDERPROCESSOR_H
#define RENDERPROCESSOR_H

#include "node/traverser.h"
#include "render/renderer.h"
#include "rendercache.h"
#include "stillimagecache.h"
#include "threading/threadticket.h"

namespace olive {

class RenderProcessor : public NodeTraverser
{
public:
  static void Process(RenderTicketPtr ticket, Renderer* render_ctx, StillImageCache* still_image_cache, DecoderCache* decoder_cache, ShaderCache* shader_cache, QVariant default_shader);

  struct RenderedWaveform {
    const Track* track;
    AudioVisualWaveform waveform;
    TimeRange range;
  };

protected:
  virtual NodeValueTable GenerateBlockTable(const Track *track, const TimeRange &range) override;

  virtual QVariant ProcessVideoFootage(VideoStream* video_stream, const rational &input_time) override;

  virtual QVariant ProcessAudioFootage(AudioStream* stream, const TimeRange &input_time) override;

  virtual QVariant ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual QVariant ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job) override;

  virtual QVariant ProcessFrameGeneration(const Node *node, const GenerateJob& job) override;

  virtual QVariant GetCachedFrame(const Node *node, const rational &time) override;

  virtual QVector2D GenerateResolution() const override;

private:
  RenderProcessor(RenderTicketPtr ticket, Renderer* render_ctx, StillImageCache* still_image_cache, DecoderCache* decoder_cache, ShaderCache* shader_cache, QVariant default_shader);

  void Run();

  DecoderPtr ResolveDecoderFromInput(Stream* stream);

  RenderTicketPtr ticket_;

  Renderer* render_ctx_;

  StillImageCache* still_image_cache_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

  QVariant default_shader_;

};

}

Q_DECLARE_METATYPE(olive::RenderProcessor::RenderedWaveform)

#endif // RENDERPROCESSOR_H
