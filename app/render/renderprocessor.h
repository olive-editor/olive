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

#ifndef RENDERPROCESSOR_H
#define RENDERPROCESSOR_H

#include "node/traverser.h"
#include "render/renderer.h"
#include "rendercache.h"
#include "stillimagecache.h"
#include "threading/threadticket.h"

OLIVE_NAMESPACE_ENTER

class RenderProcessor : public NodeTraverser
{
public:
  static void Process(RenderTicketPtr ticket, Renderer* render_ctx, StillImageCache* still_image_cache, DecoderCache* decoder_cache, ShaderCache* shader_cache, QVariant default_shader);

  struct RenderedWaveform {
    const TrackOutput* track;
    AudioVisualWaveform waveform;
    TimeRange range;
  };

protected:
  virtual NodeValueTable GenerateBlockTable(const TrackOutput *track, const TimeRange &range) override;

  virtual QVariant ProcessVideoFootage(StreamPtr stream, const rational &input_time) override;

  virtual QVariant ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time) override;

  virtual QVariant ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual QVariant ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job) override;

  virtual QVariant ProcessFrameGeneration(const Node *node, const GenerateJob& job) override;

  virtual QVariant GetCachedFrame(const Node *node, const rational &time) override;

private:
  RenderProcessor(RenderTicketPtr ticket, Renderer* render_ctx, StillImageCache* still_image_cache, DecoderCache* decoder_cache, ShaderCache* shader_cache, QVariant default_shader);

  void Run();

  DecoderPtr ResolveDecoderFromInput(StreamPtr stream);

  RenderTicketPtr ticket_;

  Renderer* render_ctx_;

  StillImageCache* still_image_cache_;

  DecoderCache* decoder_cache_;

  ShaderCache* shader_cache_;

  QVariant default_shader_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::RenderProcessor::RenderedWaveform)

#endif // RENDERPROCESSOR_H
