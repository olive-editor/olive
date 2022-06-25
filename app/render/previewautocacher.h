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
#include "node/graph.h"
#include "node/group/group.h"
#include "node/node.h"
#include "node/output/viewer/viewer.h"
#include "node/project/project.h"
#include "render/audioparams.h"
#include "render/renderjobtracker.h"
#include "render/rendermanager.h"

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
  PreviewAutoCacher();

  virtual ~PreviewAutoCacher() override;

  RenderTicketPtr GetSingleFrame(const rational& t);

  RenderTicketPtr GetRangeOfAudio(TimeRange range);

  /**
   * @brief Set the viewer node to auto-cache
   */
  void SetViewerNode(ViewerOutput *viewer_node);

  /**
   * @brief Force a certain range to be cached
   *
   * Usually, PreviewAutoCacher caches a user-defined range around the playhead, however there are
   * times they may want certain non-playhead-related time ranges to be cached (i.e. entire sequence
   * or in/out range), so that can be set here.
   */
  void ForceCacheRange(const TimeRange& range);

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

  bool IsRenderingCustomRange() const
  {
    return queued_frame_iterator_.IsCustomRange() && queued_frame_iterator_.HasNext();
  }

  void SetAudioPaused(bool e);

signals:
  void StopCacheProxyTasks();

  void SignalCacheProxyTaskProgress(double d);

private:
  void TryRender();

  RenderTicketWatcher *RenderFrame(Node *node, const rational &time, FrameHashCache *cache);
  RenderTicketWatcher *RenderFrame(const rational &time, FrameHashCache *cache)
  {
    return RenderFrame(copied_viewer_node_->GetConnectedTextureOutput(), time, cache);
  }

  RenderTicketPtr RenderAudio(Node *node, const TimeRange &range, bool generate_waveforms);
  RenderTicketPtr RenderAudio(const TimeRange &range, bool generate_waveforms)
  {
    return RenderAudio(copied_viewer_node_->GetConnectedSampleOutput(), range, generate_waveforms);
  }

  /**
   * @brief Process all changes to internal NodeGraph copy
   *
   * PreviewAutoCacher staggers updates to its internal NodeGraph copy, only applying them when the
   * RenderManager is not reading from it. This function is called when such an opportunity arises.
   */
  void ProcessUpdateQueue();

  void AddNode(Node* node);
  void RemoveNode(Node* node);
  void AddEdge(Node *output, const NodeInput& input);
  void RemoveEdge(Node *output, const NodeInput& input);
  void CopyValue(const NodeInput& input);
  void CopyValueHint(const NodeInput& input);

  void InsertIntoCopyMap(Node* node, Node* copy);

  void ConnectToNodeCache(Node *node);
  void DisconnectFromNodeCache(Node *node);

  void UpdateGraphChangeValue();
  void UpdateLastSyncedValue();

  void CancelQueuedSingleFrameRender();

  void VideoInvalidatedList(const TimeRangeList &list);
  void AudioInvalidatedList(const TimeRangeList &list);

  void StartCachingRange(const TimeRange &range, TimeRangeList *range_list, RenderJobTracker *tracker);
  void StartCachingVideoRange(const TimeRange &range);
  void StartCachingAudioRange(const TimeRange &range);

  class QueuedJob {
  public:
    enum Type {
      kNodeAdded,
      kNodeRemoved,
      kEdgeAdded,
      kEdgeRemoved,
      kValueChanged,
      kValueHintChanged
    };

    Type type;
    Node* node;
    NodeInput input;
    Node *output;
  };

  ViewerOutput* viewer_node_;

  Project copied_project_;

  QVector<QueuedJob> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  QHash<NodeGraph*, NodeGraph*> graph_map_;
  ViewerOutput* copied_viewer_node_;
  ColorManager* copied_color_manager_;
  QVector<Node*> created_nodes_;

  TimeRange cache_range_;

  bool use_custom_range_;
  TimeRange custom_autocache_range_;

  TimeRangeList invalidated_video_;
  TimeRangeList invalidated_audio_;

  bool pause_audio_;

  RenderTicketPtr single_frame_render_;

  QMap<RenderTicketWatcher*, TimeRange> audio_tasks_;
  QMap<RenderTicketWatcher*, rational> video_tasks_;
  QMap<RenderTicketWatcher*, QVector<RenderTicketPtr> > video_immediate_passthroughs_;

  JobTime graph_changed_time_;
  JobTime last_update_time_;

  QTimer delayed_requeue_timer_;

  TimeRangeList audio_needing_conform_;

  JobTime last_conform_task_;

  RenderJobTracker video_job_tracker_;
  RenderJobTracker audio_job_tracker_;

  TimeRangeListFrameIterator queued_frame_iterator_;
  TimeRangeList audio_iterator_;

  static const bool kRealTimeWaveformsEnabled;

private slots:
  /**
   * @brief Handler for when the NodeGraph reports a video change over a certain time range
   */
  void VideoInvalidated(const olive::TimeRange &range);

  /**
   * @brief Handler for when the NodeGraph reports a audio change over a certain time range
   */
  void AudioInvalidated(const olive::TimeRange &range);

  /**
   * @brief Handler for when the RenderManager has returned rendered audio
   */
  void AudioRendered();

  /**
   * @brief Handler for when the RenderManager has returned rendered video frames
   */
  void VideoRendered();

  void NodeAdded(Node* node);

  void NodeRemoved(Node* node);

  void EdgeAdded(Node *output, const NodeInput& input);

  void EdgeRemoved(Node *output, const NodeInput& input);

  void ValueChanged(const NodeInput& input);

  void ValueHintChanged(const NodeInput &input);

  /**
   * @brief Generic function called whenever the frames to render need to be (re)queued
   */
  void RequeueFrames();

  void ConformFinished();

  void VideoAutoCacheEnableChanged(bool e);

  void AudioAutoCacheEnableChanged(bool e);

  void CacheProxyTaskCancelled();

};

}

#endif // AUTOCACHER_H
