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

#ifndef AUTOCACHER_H
#define AUTOCACHER_H

#include <QtConcurrent/QtConcurrent>

#include "config/config.h"
#include "node/color/colormanager/colormanager.h"
#include "node/group/group.h"
#include "node/node.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "render/projectcopier.h"
#include "render/renderjobtracker.h"
#include "render/renderticket.h"

namespace olive {

/**
 * @brief Manager for dynamically caching a sequence in the background
 *
 * Intended to be used with a Viewer to dynamically cache parts of a sequence based on the playhead.
 */
class PreviewAutoCacher : public QObject
{
  Q_OBJECT
public:
  PreviewAutoCacher(QObject *parent = nullptr);

  virtual ~PreviewAutoCacher() override;

  RenderTicketPtr GetSingleFrame(ViewerOutput *viewer, const rational& t, bool dry = false);
  RenderTicketPtr GetSingleFrame(Node *n, ViewerOutput *viewer, const rational& t, bool dry = false);

  RenderTicketPtr GetRangeOfAudio(ViewerOutput *viewer, TimeRange range);

  void ClearSingleFrameRenders();

  /**
   * @brief Set the viewer node to auto-cache
   */
  void SetProject(Project *project);

  /**
   * @brief Force a certain range to be cached
   *
   * Usually, PreviewAutoCacher caches a user-defined range around the playhead, however there are
   * times they may want certain non-playhead-related time ranges to be cached (i.e. entire sequence
   * or in/out range), so that can be set here.
   */
  void ForceCacheRange(ViewerOutput *context, const TimeRange& range);

  /**
   * @brief Updates the range of frames to auto-cache
   */
  void SetPlayhead(const rational& playhead);

  /**
   * @brief Call cancel on all currently running video tasks
   *
   * Signalling cancel to a video task indicates that we're no longer interested in its end result.
   * This does not end all video tasks immediately, the RenderManager will do what it can to speed
   * up finishing the task. The RenderManager will  also return "no result", which can be checked
   * with watcher->HasResult.
   */
  void CancelVideoTasks(bool and_wait_for_them_to_finish = false);
  void CancelAudioTasks(bool and_wait_for_them_to_finish = false);

  bool IsRenderingCustomRange() const;

  void SetRendersPaused(bool e);
  void SetThumbnailsPaused(bool e);

  void SetMulticamNode(MultiCamNode *n) { multicam_ = n; }

  void SetIgnoreCacheRequests(bool e) { ignore_cache_requests_ = e; }

public slots:
  void SetDisplayColorProcessor(ColorProcessorPtr processor)
  {
    display_color_processor_ = processor;
  }

signals:
  void StopCacheProxyTasks();

  void SignalCacheProxyTaskProgress(double d);

private:
  void TryRender();

  RenderTicketWatcher *RenderFrame(Node *node, ViewerOutput *context, const rational &time, PlaybackCache *cache, bool dry);

  RenderTicketPtr RenderAudio(Node *node, ViewerOutput *context, const TimeRange &range, PlaybackCache *cache);

  void ConnectToNodeCache(Node *node);
  void DisconnectFromNodeCache(Node *node);

  void CancelQueuedSingleFrameRender();

  void StartCachingRange(const TimeRange &range, TimeRangeList *range_list, RenderJobTracker *tracker);
  void StartCachingVideoRange(ViewerOutput *context, PlaybackCache *cache, const TimeRange &range);
  void StartCachingAudioRange(ViewerOutput *context, PlaybackCache *cache, const TimeRange &range);

  void VideoInvalidatedFromNode(ViewerOutput *context, PlaybackCache *cache, const olive::TimeRange &range);
  void AudioInvalidatedFromNode(ViewerOutput *context, PlaybackCache *cache, const olive::TimeRange &range);

  Project* project_;

  ProjectCopier *copier_;

  TimeRange cache_range_;

  bool use_custom_range_;
  TimeRange custom_autocache_range_;

  bool pause_renders_;
  bool pause_thumbnails_;

  RenderTicketPtr single_frame_render_;
  QMap<RenderTicketWatcher*, QVector<RenderTicketPtr> > video_immediate_passthroughs_;

  QTimer delayed_requeue_timer_;

  JobTime last_conform_task_;

  QVector<RenderTicketWatcher*> running_video_tasks_;
  QVector<RenderTicketWatcher*> running_audio_tasks_;

  ColorManager* copied_color_manager_;

  struct VideoJob {
    Node *node;
    ViewerOutput *context;
    PlaybackCache *cache;
    TimeRange range;
    TimeRangeListFrameIterator iterator;
  };

  struct VideoCacheData {
    RenderJobTracker job_tracker;
  };

  struct AudioJob {
    Node *node;
    ViewerOutput *context;
    PlaybackCache *cache;
    TimeRange range;
  };

  struct AudioCacheData {
    RenderJobTracker job_tracker;
    TimeRangeList needs_conform;
  };

  std::list<VideoJob> pending_video_jobs_;
  std::list<AudioJob> pending_audio_jobs_;

  QHash<PlaybackCache*, VideoCacheData> video_cache_data_;
  QHash<PlaybackCache*, AudioCacheData> audio_cache_data_;

  ColorProcessorPtr display_color_processor_;

  MultiCamNode *multicam_;

  bool ignore_cache_requests_;

private slots:
  /**
   * @brief Handler for when the NodeGraph reports a video change over a certain time range
   */
  void VideoInvalidatedFromCache(ViewerOutput *context, const olive::TimeRange &range);

  /**
   * @brief Handler for when the NodeGraph reports a audio change over a certain time range
   */
  void AudioInvalidatedFromCache(ViewerOutput *context, const olive::TimeRange &range);

  void CancelForCache();

  /**
   * @brief Handler for when the RenderManager has returned rendered audio
   */
  void AudioRendered();

  /**
   * @brief Handler for when the RenderManager has returned rendered video frames
   */
  void VideoRendered();

  /**
   * @brief Generic function called whenever the frames to render need to be (re)queued
   */
  //void RequeueFrames();

  void ConformFinished();

  void CacheProxyTaskCancelled();

};

}

#endif // AUTOCACHER_H
