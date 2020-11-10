#ifndef AUTOCACHER_H
#define AUTOCACHER_H

#include <QtConcurrent/QtConcurrent>

#include "config/config.h"
#include "node/node.h"
#include "node/output/viewer/viewer.h"
#include "render/colormanager.h"
#include "threading/threadticketwatcher.h"

OLIVE_NAMESPACE_ENTER

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

  RenderTicketPtr GetSingleFrame(const rational& t);

  /**
   * @brief Set the viewer node to auto-cache
   */
  void SetViewerNode(ViewerOutput *viewer_node);

  /**
   * @brief If the mouse is held during the next cache invalidation, cache anyway
   *
   * By default, PreviewAutoCacher ignores invalidations that occur while the mouse is held down,
   * assuming that if the mouse is held, the user is dragging something. If you know the mouse will
   * be held during a certain action and want PreviewAutoCacher to cache anyway, call this before
   * the cache invalidates.
   */
  void IgnoreNextMouseButton();

  /**
   * @brief Returns whether the auto-cache is currently paused or not
   */
  bool IsPaused() const
  {
    return paused_;
  }

  /**
   * @brief Sets whether the auto-cache is currently paused or not
   * @param paused
   *
   * If TRUE, the cache queue is cleared (any frames currently being rendered will be processed as
   * normal however). If FALSE, any uncached frames in the range will automatically be queued.
   */
  void SetPaused(bool paused);

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
   * @brief Clears queue of running jobs
   *
   * Any jobs that haven't run yet are cancelled and will never run. Any jobs that are currently
   * running are cancelled, but may not be finished by the time this function returns. If the
   * jobs must be finished by the time this function returns, set `wait` to TRUE.
   */
  void ClearQueue(bool wait = false);

  void ClearHashQueue(bool wait = false);
  void ClearVideoQueue(bool wait = false);
  void ClearAudioQueue(bool wait = false);
  void ClearVideoDownloadQueue(bool wait = false);

  void SetColorManager(ColorManager* manager)
  {
    color_manager_ = manager;
  }

public slots:
  /**
   * @brief Main handler for when the NodeGraph changes
   */
  void NodeGraphChanged(NodeInput *source);

private:
  static void GenerateHashes(ViewerOutput* viewer, FrameHashCache *cache, const QVector<rational>& times, qint64 job_time);

  void CopyNodeInputValue(NodeInput* input);
  Node *CopyNodeConnections(Node *src_node);
  void CopyNodeMakeConnection(NodeInput *src_input, NodeInput *dst_input);

  void TryRender();

  /**
   * @brief Generic function called whenever the frames to render need to be (re)queued
   */
  void RequeueFrames();

  /**
   * @brief Process all changes to internal NodeGraph copy
   *
   * PreviewAutoCacher staggers updates to its internal NodeGraph copy, only applying them when the
   * RenderManager is not reading from it. This function is called when such an opportunity arises.
   */
  void ProcessUpdateQueue();

  bool HasActiveJobs() const;

  QList<NodeInput*> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  ViewerOutput* copied_viewer_node_;

  ViewerOutput* viewer_node_;

  bool paused_;

  TimeRange cache_range_;

  bool has_changed_;

  bool use_custom_range_;
  TimeRange custom_autocache_range_;

  TimeRangeList invalidated_video_;
  TimeRangeList invalidated_audio_;

  RenderTicketPtr single_frame_render_;

  QList<QFutureWatcher<void>*> hash_tasks_;
  QMap<RenderTicketWatcher*, TimeRange> audio_tasks_;
  QMap<RenderTicketWatcher*, QByteArray> video_tasks_;
  QMap<RenderTicketWatcher*, QByteArray> video_download_tasks_;

  QVector<QByteArray> currently_caching_hashes_;

  qint64 last_update_time_;

  bool ignore_next_mouse_button_;

  bool video_params_changed_;

  bool audio_params_changed_;

  ColorManager* color_manager_;

private slots:
  /**
   * @brief Handler for when the NodeGraph reports a video change over a certain time range
   */
  void VideoInvalidated(const OLIVE_NAMESPACE::TimeRange &range);

  /**
   * @brief Handler for when the NodeGraph reports a audio change over a certain time range
   */
  void AudioInvalidated(const OLIVE_NAMESPACE::TimeRange &range);

  /**
   * @brief Handler for when we have applied all the hashes to the FrameHashCache
   */
  void HashesProcessed();

  /**
   * @brief Handler for when the RenderManager has returned rendered audio
   */
  void AudioRendered();

  /**
   * @brief Handler for when the RenderManager has returned rendered video frames
   */
  void VideoRendered();

  /**
   * @brief Handler for when we've saved a video frame to the cache
   */
  void VideoDownloaded();

  /**
   * @brief Handler for when a NodeInput has been deleted so we clear it from the queue
   *
   * FIXME: This is hacky. It also might not be necessary anymore with recent changes to the
   *        node system, but I haven't tested yet. Either way, PreviewAutoCacher should probably
   *        be able to pick up on these sorts of things without such a slot.
   */
  void QueuedInputRemoved();

  void VideoParamsChanged();

  void AudioParamsChanged();

  void SingleFrameFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // AUTOCACHER_H
