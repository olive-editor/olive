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

#ifndef VIDEORENDERERBACKEND_H
#define VIDEORENDERERBACKEND_H

#include <QLinkedList>
#include <QOpenGLTexture>

#include "node/output/viewer/viewer.h"
#include "render/pixelformat.h"
#include "render/rendermodes.h"
#include "opengl/openglframebuffer.h"
#include "opengl/openglshader.h"
#include "opengl/opengltexture.h"
#include "videorendererdownloadthread.h"
#include "videorendererprocessthread.h"

/**
 * @brief A multithreaded OpenGL based renderer for node systems
 */
class VideoRendererProcessor : public QObject
{
  Q_OBJECT
public:
  /**
   * @brief Renderer Constructor
   *
   * Constructing a Renderer object will not start any threads/backend on its own. Use Start() to do this and Stop()
   * when the Renderer is about to be destroyed.
   */
  VideoRendererProcessor(QObject* parent);

  virtual ~VideoRendererProcessor() override;

  void SetCacheName(const QString& s);

  /**
   * @brief Set parameters of the Renderer
   *
   * The Renderer owns the buffers that are used in the rendering process and this function sets the kind of buffers
   * to use. The Renderer must be stopped when calling this function.
   *
   * @param width
   *
   * Buffer width
   *
   * @param height
   *
   * Buffer height
   *
   * @param format
   *
   * Buffer pixel format
   */
  void SetParameters(const VideoRenderingParams &params);

  /**
   * @brief Return whether a frame with this hash already exists
   */
  bool HasHash(const QByteArray& hash);

  /**
   * @brief Return whether a frame is currently being cached
   */
  bool IsCaching(const QByteArray& hash);

  /**
   * @brief Check if a frame is currently being cached, and if not reserve it
   */
  bool TryCache(const QByteArray& hash);

  RenderTexturePtr GetCachedFrame(const rational& time);

  void SetViewerNode(ViewerOutput* viewer);

signals:
  void CachedFrameReady(const rational& time);

private:
  struct HashTimeMapping {
    rational time;
    QByteArray hash;
  };

  /**
   * @brief Allocate and start the multithreaded backend
   */
  void Start();

  /**
   * @brief Terminate and deallocate the multithreaded backend
   */
  void Stop();

  /**
   * @brief Internal function for generating the cache ID
   */
  void GenerateCacheIDInternal();

  /**
   * @brief Function called when there are frames in the queue to cache
   *
   * This function is NOT thread-safe and should only be called in the main thread.
   */
  void CacheNext();

  bool ShouldPushTexture(const rational &time);

  /**
   * @brief Return the path of the cached image at this time
   */
  QString CachePathName(const QByteArray &hash);

  void DeferMap(const rational &time, const QByteArray &hash);

  /**
   * @brief Internal list of RenderProcessThreads
   */
  QVector<RendererProcessThreadPtr> threads_;

  /**
   * @brief Internal variable that contains whether the Renderer has started or not
   */
  bool started_;

  VideoRenderingParams params_;

  rational last_time_requested_;

  QLinkedList<rational> cache_queue_;
  QString cache_name_;
  qint64 cache_time_;
  QString cache_id_;

  bool caching_;
  QVector<uchar*> cache_frame_load_buffer_;

  QVector<RendererDownloadThreadPtr> download_threads_;
  int last_download_thread_;

  RenderTexturePtr master_texture_;
  rational push_time_;

  OpenGLFramebuffer copy_buffer_;
  OpenGLShaderPtr copy_pipeline_;

  QMap<rational, QByteArray> time_hash_map_;

  QMutex cache_hash_list_mutex_;
  QVector<QByteArray> cache_hash_list_;

  QList<HashTimeMapping> deferred_maps_;

  bool starting_;

  ViewerOutput* viewer_node_;

private slots:
  void InvalidateCache(const rational &start_range, const rational &end_range);

  void ThreadCallback(RenderTexturePtr texture, const rational& time, const QByteArray& hash);

  void ThreadRequestSibling(NodeDependency dep);

  void ThreadSkippedFrame(const rational &time, const QByteArray &hash);

  void DownloadThreadComplete(const QByteArray &hash);

};

#endif // VIDEORENDERERBACKEND_H
