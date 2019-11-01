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

#include "node/output/viewer/viewer.h"
#include "renderbackend.h"
#include "render/pixelformat.h"
#include "render/rendermodes.h"

/**
 * @brief A multithreaded OpenGL based renderer for node systems
 */
class VideoRenderBackend : public RenderBackend
{
  Q_OBJECT
public:
  /**
   * @brief Renderer Constructor
   *
   * Constructing a Renderer object will not start any threads/backend on its own. Use Start() to do this and Stop()
   * when the Renderer is about to be destroyed.
   */
  VideoRenderBackend(QObject* parent = nullptr);

  virtual ~VideoRenderBackend() override;

  void SetCacheName(const QString& s);

  /**
   * @brief Allocate and start the multithreaded backend
   */
  virtual bool Init() override;

  /**
   * @brief Terminate and deallocate the multithreaded backend
   */
  virtual void Close() override;

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

  bool IsStarted();

public slots:
  virtual void InvalidateCache(const rational &start_range, const rational &end_range) override;

protected:
  struct HashTimeMapping {
    rational time;
    QByteArray hash;
  };

  virtual void ViewerNodeChangedEvent(ViewerOutput* node) override;

  virtual void GenerateFrame(const rational&) = 0;

  const char *GetCachedFrame(const rational& time);

  /**
   * @brief Return the path of the cached image at this time
   */
  QString CachePathName(const QByteArray &hash);

  void DeferMap(const rational &time, const QByteArray &hash);

  /**
   * @brief Function called when there are frames in the queue to cache
   *
   * This function is NOT thread-safe and should only be called in the main thread.
   */
  void CacheNext();

  const QVector<QThread*>& threads();

  const VideoRenderingParams& params() const;

  QMap<rational, QByteArray> time_hash_map_;

  QList<HashTimeMapping> deferred_maps_;

  QMutex cache_hash_list_mutex_;
  QVector<QByteArray> cache_hash_list_;

  rational last_time_requested_;

  bool caching_;

signals:
  void CachedFrameReady(const rational& time);

private:
  /**
   * @brief Internal function for generating the cache ID
   */
  void GenerateCacheIDInternal();

  /**
   * @brief Internal list of RenderProcessThreads
   */
  QVector<QThread*> threads_;

  /**
   * @brief Internal variable that contains whether the Renderer has started or not
   */
  bool started_;

  VideoRenderingParams params_;

  QLinkedList<rational> cache_queue_;
  QString cache_name_;
  qint64 cache_time_;
  QString cache_id_;

  QByteArray cache_frame_load_buffer_;

  /*QVector<RendererDownloadThreadPtr> download_threads_;
  int last_download_thread_;*/

private slots:


};

#endif // VIDEORENDERERBACKEND_H
