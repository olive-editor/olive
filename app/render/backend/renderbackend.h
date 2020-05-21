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
#include "render/backend/colorprocessorcache.h"
#include "renderworker.h"

OLIVE_NAMESPACE_ENTER

class RenderBackend : public QObject
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);

  virtual ~RenderBackend() override;

  void SetViewerNode(ViewerOutput* viewer_node);

  void CancelQueue();

  /**
   * @brief Asynchronously generate a hash at a given time
   */
  QFuture<QByteArray> Hash(const rational& time, bool block_for_update);

  /**
   * @brief Asynchronously generate a frame at a given time
   */
  QFuture<FramePtr> RenderFrame(const rational& time, bool clear_queue, bool block_for_update);

  /**
   * @brief Asynchronously generate a chunk of audio
   */
  QFuture<SampleBufferPtr> RenderAudio(const TimeRange& r, bool block_for_update);

  void SetVideoParams(const VideoRenderingParams& params);

  void SetAudioParams(const AudioRenderingParams& params);

  void SetVideoDownloadMatrix(const QMatrix4x4& mat);

  void SetAutomaticAudio(bool e);

  void WorkerStartedRenderingAudio(const TimeRange& r);

  static QList<TimeRange> SplitRangeIntoChunks(const TimeRange& r);

public slots:
  void NodeGraphChanged(NodeInput *source);

  void UpdateInstance(OLIVE_NAMESPACE::RenderWorker* instance);

protected:
  virtual RenderWorker* CreateNewWorker() = 0;

  void Close();

private:
  struct RenderPool {
    RenderPool();

    QVector<RenderWorker*> instances;
    int queuer;
    QThreadPool threads;

    void Init(ViewerOutput *v);
    void Queue(NodeInput* input);
    void Close();
    void Destroy();
  };

  RenderWorker *GetInstanceFromPool(RenderPool &pool);

  ViewerOutput* viewer_node_;

  //RenderCancelDialog* cancel_dialog_;

  RenderPool video_pool_;
  RenderPool audio_pool_;
  RenderPool hash_pool_;

  QMutex queued_audio_lock_;
  TimeRangeList queued_audio_;

  bool auto_audio_;

  // VIDEO MEMBERS
  VideoRenderingParams video_params_;
  QMatrix4x4 video_download_matrix_;

  // AUDIO MEMBERS
  AudioRenderingParams audio_params_;
  QHash< QFutureWatcher<SampleBufferPtr>*, TimeRange > audio_jobs_;

  struct ConformWaitInfo {
    StreamPtr stream;
    AudioRenderingParams params;
    TimeRange affected_range;
    rational stream_time;

    bool operator==(const ConformWaitInfo& rhs) const;
  };

  QList<ConformWaitInfo> conform_wait_info_;

  void ListenForConformSignal(AudioStreamPtr s);

  void StopListeningForConformSignal(AudioStream *s);

  bool ic_from_conform_;

private slots:
  void AudioConformUnavailable(StreamPtr stream, TimeRange range,
                               rational stream_time, AudioRenderingParams params);

  void AudioConformUpdated(OLIVE_NAMESPACE::AudioRenderingParams params);

  void AudioInvalidated(const TimeRange &r);

  void AudioRendered();

  void WorkerFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERBACKEND_H
