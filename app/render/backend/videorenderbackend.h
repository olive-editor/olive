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

#include "colorprocessorcache.h"
#include "node/output/viewer/viewer.h"
#include "renderbackend.h"
#include "render/pixelformat.h"
#include "render/rendermodes.h"
#include "videorenderframecache.h"
#include "videorenderworker.h"

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

  /**
   * @brief Set parameters of the Renderer
   *
   * The Renderer owns the buffers that are used in the rendering process and this function sets the kind of buffers
   * to use. The Renderer must be stopped when calling this function.
   */
  void SetParameters(const VideoRenderingParams &params);

  void SetOperatingMode(const VideoRenderWorker::OperatingMode& mode);

  void SetOnlySignalLastFrameRequested(bool enabled);

  bool IsRendered(const rational& time) const;

  VideoRenderFrameCache* frame_cache();

  const VideoRenderingParams& params() const;

protected:
  /**
   * @brief Allocate and start the multithreaded backend
   */
  virtual bool InitInternal() override;

  /**
   * @brief Terminate and deallocate the multithreaded backend
   */
  virtual void CloseInternal() override;

  struct HashTimeMapping {
    rational time;
    QByteArray hash;
  };

  virtual void ConnectViewer(ViewerOutput* node) override;

  virtual void DisconnectViewer(ViewerOutput* node) override;

  const char *GetCachedFrame(const rational& time);

  virtual NodeInput* GetDependentInput() override;

  virtual bool CanRender() override;

  virtual TimeRange PopNextFrameFromQueue() override;

  /**
   * @brief Internal function for generating the cache ID
   */
  virtual bool GenerateCacheIDInternal(QCryptographicHash& hash) override;

  virtual void CacheIDChangedEvent(const QString& id) override;

  virtual void ConnectWorkerToThis(RenderWorker* processor) override;

  virtual void EmitCachedFrameReady(const rational &time, const QVariant& value, qint64 job_time) = 0;

  virtual void InvalidateCacheInternal(const rational &start_range, const rational &end_range) override;

  VideoRenderWorker::OperatingMode operating_mode_;

signals:
  void CachedFrameReady(const rational& time, QVariant value, qint64 job_time);
  void CachedTimeReady(const rational& time, qint64 job_time);

  void RangeInvalidated(const TimeRange& range);

private:
  bool TimeIsQueued(const TimeRange &time) const;

  bool JobIsCurrent(const NodeDependency &dep, const qint64& job_time) const;

  bool SetFrameHash(const NodeDependency& dep, const QByteArray& hash, const qint64& job_time);

  void Requeue();

  VideoRenderingParams params_;

  QByteArray cache_frame_load_buffer_;

  VideoRenderFrameCache frame_cache_;

  TimeRangeList invalidated_;

  rational last_time_requested_;

  bool only_signal_last_frame_requested_;

private slots:
  void ThreadCompletedFrame(NodeDependency path, qint64 job_time, QByteArray hash, QVariant value);
  void ThreadCompletedDownload(NodeDependency dep, qint64 job_time, QByteArray hash);
  void ThreadSkippedFrame(NodeDependency dep, qint64 job_time, QByteArray hash);
  void ThreadHashAlreadyExists(NodeDependency dep, qint64 job_time, QByteArray hash);

  void TruncateFrameCacheLength(const rational& length);

  void FrameRemovedFromDiskCache(const QByteArray& hash);

};

#endif // VIDEORENDERERBACKEND_H
