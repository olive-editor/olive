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

#ifndef RENDERWORKER_H
#define RENDERWORKER_H

#include <QMatrix4x4>

#include "decodercache.h"
#include "node/traverser.h"
#include "node/output/viewer/viewer.h"
#include "renderticket.h"

OLIVE_NAMESPACE_ENTER

class RenderBackend;

class RenderWorker : public QObject, public NodeTraverser
{
  Q_OBJECT
public:
  RenderWorker(RenderBackend* parent);

  virtual ~RenderWorker() override;

  bool IsAvailable() const
  {
    return available_;
  }

  void SetAvailable(bool a)
  {
    available_ = a;
  }

  void SetViewerNode(ViewerOutput* viewer)
  {
    viewer_ = viewer;
  }

  void SetVideoParams(const VideoParams& params)
  {
    video_params_ = params;
  }

  void SetAudioParams(const AudioParams& params)
  {
    audio_params_ = params;
  }

  void SetVideoDownloadMatrix(const QMatrix4x4& mat)
  {
    video_download_matrix_ = mat;
  }

  void SetCopyMap(QHash<Node*, Node*>* copy_map)
  {
    copy_map_ = copy_map;
  }

  void SetRenderMode(const RenderMode::Mode& mode)
  {
    render_mode_ = mode;
  }

  void SetPreviewGenerationEnabled(bool e)
  {
    generate_audio_previews_ = e;
  }

  void Hash(RenderTicketPtr ticket, ViewerOutput* viewer, const QVector<rational>& times);

  /**
   * @brief Render the frame at this time
   *
   * Produces a fully rendered frame from the connected viewer at this time.
   *
   * @return
   *
   * A frame corresponding to the set video parameters. If no nodes are active at the time, this
   * function will still return a blank frame with the same parameters. If no viewer node is set,
   * nullptr is returned.
   */
  void RenderFrame(RenderTicketPtr ticket, ViewerOutput* viewer, const rational &time);

  void RenderAudio(RenderTicketPtr ticket, ViewerOutput* viewer, const TimeRange& range);

protected:
  virtual void TextureToFrame(const QVariant& texture, FramePtr frame, const QMatrix4x4 &mat) const = 0;

  virtual QVariant FootageFrameToTexture(StreamPtr stream, FramePtr frame) const = 0;

  virtual QVariant CachedFrameToTexture(FramePtr frame) const = 0;

  virtual NodeValueTable GenerateBlockTable(const TrackOutput *track, const TimeRange &range) override;

  virtual QVariant ProcessVideoFootage(StreamPtr stream, const rational &input_time) override;

  virtual QVariant ProcessAudioFootage(StreamPtr stream, const TimeRange &input_time) override;

  virtual QVariant ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job) override;

  virtual QVariant ProcessFrameGeneration(const Node *node, const GenerateJob& job) override;

  virtual QVariant GetCachedFrame(const Node *node, const rational &time) override;

  virtual bool TextureHasAlpha(const QVariant& v) const = 0;

  const VideoParams& video_params() const
  {
    return video_params_;
  }

  const AudioParams& audio_params() const
  {
    return audio_params_;
  }

  const RenderMode::Mode& render_mode() const
  {
    return render_mode_;
  }

signals:
  void AudioConformUnavailable(StreamPtr stream, TimeRange range,
                               rational stream_time, AudioParams params);

  void FinishedJob();

  void WaveformGenerated(OLIVE_NAMESPACE::RenderTicketPtr ticket, OLIVE_NAMESPACE::TrackOutput* track, OLIVE_NAMESPACE::AudioVisualWaveform samples, OLIVE_NAMESPACE::TimeRange range);

private:
  DecoderPtr ResolveDecoderFromInput(StreamPtr stream);

  static QByteArray HashNode(const Node* n, const VideoParams& params, const rational& time);

  RenderBackend* parent_;

  RenderTicketPtr ticket_;

  VideoParams video_params_;

  AudioParams audio_params_;

  struct CachedStill {
    QVariant texture;
    QString colorspace;
    bool alpha_is_associated;
    int divider;
    rational time;
  };

  QHash<Stream*, CachedStill> still_image_cache_;

  QMatrix4x4 video_download_matrix_;

  QMutex decoder_lock_;
  DecoderCache decoder_cache_;
  QHash<Stream*, qint64> decoder_age_;

  TimeRange audio_render_time_;
  bool available_;

  bool generate_audio_previews_;

  ViewerOutput* viewer_;

  QHash<Node*, Node*>* copy_map_;

  RenderMode::Mode render_mode_;

  QTimer* cleanup_timer_;

  static const int kMaxDecoderLife;

private slots:
  void ClearOldDecoders();

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERWORKER_H
