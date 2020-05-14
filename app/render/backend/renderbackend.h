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

#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include <QtConcurrent/QtConcurrent>

#include "dialog/rendercancel/rendercancel.h"
#include "decodercache.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "node/traverser.h"
#include "render/backend/colorprocessorcache.h"

OLIVE_NAMESPACE_ENTER

class RenderBackend : public QObject, public NodeTraverser
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);

  void SetViewerNode(ViewerOutput* viewer_node);

  void CancelQueue();

  /**
   * @brief Asynchronously generate a hash at a given time
   */
  QFuture<QByteArray> Hash(const rational& time);

  /**
   * @brief Asynchronously generate a frame at a given time
   */
  QFuture<FramePtr> RenderFrame(const rational& time);

  void SetDivider(const int& divider);

  void SetMode(const RenderMode::Mode& mode);

  void SetPixelFormat(const PixelFormat::Format& pix_fmt);

  void SetSampleFormat(const SampleFormat::Format& sample_fmt);

public slots:
  void NodeGraphChanged(NodeInput *from, NodeInput *source);

protected:
  virtual void TextureToFrame(const QVariant& texture, FramePtr frame) const = 0;

  virtual NodeValue FrameToTexture(FramePtr frame) const = 0;

  virtual void FootageProcessingEvent(StreamPtr stream, const TimeRange &input_time, NodeValueTable* table) const override;

  virtual NodeValueTable GenerateBlockTable(const TrackOutput *track, const TimeRange &range) const override;

private:
  FramePtr RenderFrameInternal(const rational& time) const;

  /**
   * @brief Internal reference to attached viewer node
   */
  ViewerOutput* viewer_node_;

  RenderCancelDialog* cancel_dialog_;

  VideoRenderingParams video_params() const;

  NodeValue GetDataFromStream(StreamPtr stream, const TimeRange& input_time) const;

  DecoderPtr ResolveDecoderFromInput(StreamPtr stream) const;

  class CopyMap {
  public:
    CopyMap();

    void Init(ViewerOutput* viewer);

    void Queue(NodeInput* input);

    void ProcessQueue();

    void Clear();

    QThreadPool* thread_pool() {
      return &thread_pool_;
    }

  private:
    void CopyNodeInputValue(NodeInput* input);
    Node *CopyNodeConnections(Node *src_node);
    void CopyNodeMakeConnection(NodeInput *src_input, NodeInput *dst_input);

    ViewerOutput* original_viewer_;
    ViewerOutput* copied_viewer_;
    QList<NodeInput*> queued_updates_;
    QHash<Node*, Node*> copy_map_;
    QThreadPool thread_pool_;

  };

  // VIDEO MEMBERS
  CopyMap video_copy_map_;
  int divider_;
  RenderMode::Mode render_mode_;
  PixelFormat::Format pix_fmt_;

  ColorProcessorCache color_cache_;

  struct CachedStill {
    NodeValue texture;
    QString colorspace;
    bool alpha_is_associated;
    int divider;
    rational time;
  };

  RenderCache<Stream*, CachedStill> still_image_cache_;

  // AUDIO MEMBERS
  CopyMap audio_copy_map_;
  SampleFormat::Format sample_fmt_;

  struct ConformWaitInfo {
    StreamPtr stream;
    TimeRange affected_range;
    rational stream_time;

    bool operator==(const ConformWaitInfo& rhs) const;
  };

  QList<ConformWaitInfo> footage_wait_info_;

private slots:
  void AudioCallback();

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERBACKEND_H
