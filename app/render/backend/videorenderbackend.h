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

OLIVE_NAMESPACE_ENTER

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

  void SetFrameGenerationParams(int width, int height, const QMatrix4x4& matrix);

  void SetOnlySignalLastFrameRequested(bool enabled);

  bool IsRendered(const rational& time) const;

  void SetLimitCaching(bool limit);

  QString GetCachedFrame(const rational& time);

  void UpdateLastRequestedTime(const rational& time);

  VideoRenderFrameCache* frame_cache();

  const VideoRenderingParams& params() const;

protected:
  struct HashTimeMapping {
    rational time;
    QByteArray hash;
  };

  virtual void ConnectViewer(ViewerOutput* node) override;

  virtual void DisconnectViewer(ViewerOutput* node) override;

  virtual NodeInput* GetDependentInput() override;

  virtual bool CanRender() override;

  virtual TimeRange PopNextFrameFromQueue() override;

  /**
   * @brief Internal function for generating the cache ID
   */
  virtual bool GenerateCacheIDInternal(QCryptographicHash& hash) override;

  virtual void CacheIDChangedEvent(const QString& id) override;

  virtual void ConnectWorkerToThis(RenderWorker* processor) override;

  virtual void InvalidateCacheInternal(const rational &start_range, const rational &end_range) override;

  virtual void ParamsChangedEvent(){}

  VideoRenderWorker::OperatingMode operating_mode_;

signals:
  void CachedTimeReady(const rational& time, qint64 job_time);

  void RangeInvalidated(const TimeRange& range);

  void GeneratedFrame(FramePtr frame);

private:
  bool TimeIsQueued(const TimeRange &time) const;

  bool JobIsCurrent(const NodeDependency &dep, const qint64& job_time) const;

  bool SetFrameHash(const NodeDependency& dep, const QByteArray& hash, const qint64& job_time);

  void Requeue();

  VideoRenderingParams params_;

  VideoRenderFrameCache frame_cache_;

  TimeRangeList invalidated_;

  rational last_time_requested_;

  bool only_signal_last_frame_requested_;

  bool limit_caching_;

  bool pop_toggle_;

private slots:
  void ThreadCompletedDownload(NodeDependency dep, qint64 job_time, QByteArray hash, bool texture_existed);
  void ThreadSkippedFrame(NodeDependency dep, qint64 job_time, QByteArray hash);
  void ThreadHashAlreadyExists(NodeDependency dep, qint64 job_time, QByteArray hash);
  void ThreadGeneratedFrame();

  void TruncateFrameCacheLength(const rational& length);

  void FrameRemovedFromDiskCache(const QByteArray& hash);

};

OLIVE_NAMESPACE_EXIT

#endif // VIDEORENDERERBACKEND_H
