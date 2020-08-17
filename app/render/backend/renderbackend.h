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

#include "config/config.h"
#include "dialog/rendercancel/rendercancel.h"
#include "decodercache.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "render/backend/colorprocessorcache.h"
#include "renderticket.h"
#include "renderticketwatcher.h"
#include "renderworker.h"

OLIVE_NAMESPACE_ENTER

class RenderBackend : public QObject
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);

  virtual ~RenderBackend() override;

  void Close();

  ViewerOutput* GetViewerNode() const
  {
    return viewer_node_;
  }

  void SetViewerNode(ViewerOutput* viewer_node);

  void SetAutoCacheEnabled(bool e)
  {
    autocache_enabled_ = e;
  }

  bool IsAutoCachePaused() const
  {
    return autocache_paused_;
  }

  void SetAutoCachePaused(bool paused)
  {
    autocache_paused_ = paused;

    if (autocache_paused_) {
      // Pause the autocache
      ClearVideoQueue();
    } else {
      // Unpause the cache
      AutoCacheRequeueFrames();
    }
  }

  void AutoCacheRange(const TimeRange& range);

  void AutoCacheRequeueFrames();

  void SetAutoCachePlayhead(const rational& playhead)
  {
    autocache_range_ = TimeRange(playhead - Config::Current()["DiskCacheBehind"].value<rational>(),
                                 playhead + Config::Current()["DiskCacheAhead"].value<rational>());

    autocache_has_changed_ = true;
    use_custom_autocache_range_ = false;

    AutoCacheRequeueFrames();
  }

  void SetRenderMode(RenderMode::Mode e)
  {
    render_mode_ = e;
  }

  void SetPreviewGenerationEnabled(bool e)
  {
    generate_audio_previews_ = e;
  }

  void ProcessUpdateQueue();

  /**
   * @brief Asynchronously generate a hash at a given time
   */
  RenderTicketPtr Hash(const QVector<rational> &times, bool prioritize = false);

  /**
   * @brief Asynchronously generate a frame at a given time
   */
  RenderTicketPtr RenderFrame(const rational& time, bool prioritize = false, const QByteArray& hash = QByteArray());

  /**
   * @brief Asynchronously generate a chunk of audio
   */
  RenderTicketPtr RenderAudio(const TimeRange& r, bool prioritize = false);

  const VideoParams& GetVideoParams() const
  {
    return video_params_;
  }

  const AudioParams& GetAudioParams() const
  {
    return audio_params_;
  }

  void SetVideoParams(const VideoParams& params);

  void SetAudioParams(const AudioParams& params);

  void SetVideoDownloadMatrix(const QMatrix4x4& mat);

  static std::list<TimeRange> SplitRangeIntoChunks(const TimeRange& r);

public slots:
  void NodeGraphChanged(NodeInput *source);

  void ClearVideoQueue();

  void ClearAudioQueue();

  void ClearQueue();

signals:

protected:
  virtual RenderWorker* CreateNewWorker() = 0;

private:
  void CopyNodeInputValue(NodeInput* input);
  Node *CopyNodeConnections(Node *src_node);
  void CopyNodeMakeConnection(NodeInput *src_input, NodeInput *dst_input);

  void ClearQueueOfType(RenderTicket::Type type);

  void SetHashes(FrameHashCache* cache, const QVector<rational>& times, const QVector<QByteArray>& hashes, qint64 job_time);

  ViewerOutput* viewer_node_;

  // VIDEO MEMBERS
  VideoParams video_params_;
  QMatrix4x4 video_download_matrix_;

  // AUDIO MEMBERS
  AudioParams audio_params_;

  QList<NodeInput*> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  ViewerOutput* copied_viewer_node_;

  std::list<RenderTicketPtr> render_queue_;

  std::list<RenderTicketPtr> running_tickets_;

  struct WorkerData {
    RenderWorker* worker;
    bool busy;
  };

  QVector<WorkerData> workers_;

  bool autocache_enabled_;
  bool autocache_paused_;

  bool generate_audio_previews_;

  RenderMode::Mode render_mode_;

  TimeRange autocache_range_;

  bool autocache_has_changed_;

  bool use_custom_autocache_range_;
  TimeRange custom_autocache_range_;

  static QVector<RenderBackend*> instances_;
  static QMutex instance_lock_;
  static RenderBackend* active_instance_;
  static QThreadPool thread_pool_;
  void SetActiveInstance();

  QMap<RenderTicketWatcher*, QVector<rational> > autocache_hash_tasks_;

  QList<QFutureWatcher<void>*> autocache_hash_process_tasks_;

  QMap<RenderTicketWatcher*, TimeRange> autocache_audio_tasks_;

  QMap<RenderTicketWatcher*, QByteArray> autocache_video_tasks_;

  QMap<QFutureWatcher<bool>*, QByteArray> autocache_video_download_tasks_;

  QVector<QByteArray> autocache_currently_caching_hashes_;

private slots:
  void WorkerFinished();

  void RunNextJob();

  void TicketFinished();

  void WorkerGeneratedWaveform(OLIVE_NAMESPACE::RenderTicketPtr ticket, OLIVE_NAMESPACE::TrackOutput* track, OLIVE_NAMESPACE::AudioVisualWaveform samples, OLIVE_NAMESPACE::TimeRange range);

  void AutoCacheVideoInvalidated(const OLIVE_NAMESPACE::TimeRange &range);

  void AutoCacheAudioInvalidated(const OLIVE_NAMESPACE::TimeRange &range);

  void AutoCacheHashesGenerated();

  void AutoCacheHashesProcessed();

  void AutoCacheAudioRendered();

  void AutoCacheVideoRendered();

  void AutoCacheVideoDownloaded();

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERBACKEND_H
