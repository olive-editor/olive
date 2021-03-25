#ifndef AUTOCACHER_H
#define AUTOCACHER_H

#include <QtConcurrent/QtConcurrent>

#include "config/config.h"
#include "node/graph.h"
#include "node/node.h"
#include "node/output/viewer/viewer.h"
#include "project/project.h"
#include "render/colormanager.h"
#include "threading/threadticketwatcher.h"

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

  /**
   * @brief Set the viewer node to auto-cache
   */
  void SetViewerNode(Sequence *viewer_node);

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

  void ClearHashQueue(bool wait = false);
  void ClearVideoQueue(bool wait = false);
  void ClearAudioQueue(bool wait = false);
  void ClearVideoDownloadQueue(bool wait = false);

  void SetColorManager(ColorManager* manager)
  {
    color_manager_ = manager;
  }

private:
  static void GenerateHashes(Sequence *viewer, FrameHashCache *cache, const QVector<rational>& times, qint64 job_time);

  void TryRender();

  /**
   * @brief Process all changes to internal NodeGraph copy
   *
   * PreviewAutoCacher staggers updates to its internal NodeGraph copy, only applying them when the
   * RenderManager is not reading from it. This function is called when such an opportunity arises.
   */
  void ProcessUpdateQueue();

  bool HasActiveJobs() const;

  void AddNode(Node* node);
  void RemoveNode(Node* node);
  void AddEdge(const NodeOutput& output, const NodeInput& input);
  void RemoveEdge(const NodeOutput& output, const NodeInput& input);
  void CopyValue(const NodeInput& input);

  void InsertIntoCopyMap(Node* node, Node* copy);

  void UpdateLastSyncedValue();

  class QueuedJob {
  public:
    enum Type {
      kNodeAdded,
      kNodeRemoved,
      kEdgeAdded,
      kEdgeRemoved,
      kValueChanged
    };

    Type type;
    Node* node;
    NodeInput input;
    NodeOutput output;
  };

  Sequence* viewer_node_;

  Project copied_project_;

  QVector<QueuedJob> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  Sequence* copied_viewer_node_;
  QVector<Node*> created_nodes_;

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
  QVector<RenderTicketWatcher*> single_frame_tasks_;

  QVector<QByteArray> currently_caching_hashes_;

  qint64 last_update_time_;

  bool ignore_next_mouse_button_;

  ColorManager* color_manager_;

  QTimer delayed_requeue_timer_;

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

  void NodeAdded(Node* node);

  void NodeRemoved(Node* node);

  void EdgeAdded(const NodeOutput& output, const NodeInput& input);

  void EdgeRemoved(const NodeOutput& output, const NodeInput& input);

  void ValueChanged(const NodeInput& input);

  void SingleFrameFinished();

  /**
   * @brief Generic function called whenever the frames to render need to be (re)queued
   */
  void RequeueFrames();

};

}

#endif // AUTOCACHER_H
