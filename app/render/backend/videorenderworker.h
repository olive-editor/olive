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

#ifndef VIDEORENDERWORKER_H
#define VIDEORENDERWORKER_H

#include <QCryptographicHash>

#include "colorprocessorcache.h"
#include "node/dependency.h"
#include "render/videoparams.h"
#include "renderworker.h"
#include "videorenderframecache.h"

OLIVE_NAMESPACE_ENTER

class VideoRenderWorker : public RenderWorker {
  Q_OBJECT
public:
  /**
   * @brief VideoRenderWorker uses hashes to recognize frames that are identical to others in the render queue
   *
   * This mode can modify the behavior of the worker, either to disable hash verification or only generate hashes and
   * not render. These are only used in the context of exporting.
   *
   * These are also flags that can be or'd together, mostly for the convenience of checking which functionalities are
   * disabled and enabled.
   */
  enum OperatingMode {
    /// Generate hashes but don't render or download anything
    kHashOnly = 0x1,

    /// Render but don't download or hash
    kRenderOnly = 0x2,

    /// Enable download (NEVER USE THIS ON ITS OWN, this is only here for checking flags, download-only mode makes no sense)
    kDownloadOnly = 0x4,

    /// Use hash verification and render, but don't cache any frames to disk
    kHashAndRenderOnly = 0x3,

    /// Render and download, but don't use hash verification
    kRenderAndCacheOnly = 0x6,

    /// Render and use hashes to identify exact matches (default)
    kHashRenderCache = 0x7
  };

  VideoRenderWorker(VideoRenderFrameCache* frame_cache, QObject* parent = nullptr);

  void SetParameters(const VideoRenderingParams& video_params);

  void SetOperatingMode(const OperatingMode& mode);

signals:
  void CompletedDownload(NodeDependency path, qint64 job_time, QByteArray hash, bool texture_existed);

  void HashAlreadyBeingCached(NodeDependency path, qint64 job_time, QByteArray hash);

  void HashAlreadyExists(NodeDependency path, qint64 job_time, QByteArray hash);

  void GeneratedFrame(const rational &time, FramePtr frame);

  void Aborted();

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  const VideoRenderingParams& video_params();

  virtual void ParametersChangedEvent(){}

  virtual void TextureToBuffer(const QVariant& texture, void *buffer) = 0;

  virtual NodeValueTable RenderInternal(const NodeDependency& CurrentPath, const qint64& job_time) override;

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range) override;

  virtual void ReportUnavailableFootage(StreamPtr stream, Decoder::RetrieveState state, const rational& stream_time) override;

  ColorProcessorCache* color_cache();

private:
  void HashNodeRecursively(QCryptographicHash* hash, const Node *n, const rational &time);

  void Download(const rational &time, QVariant texture, QString filename);

  void ResizeDownloadBuffer();

  VideoRenderingParams video_params_;

  VideoRenderFrameCache* frame_cache_;

  ColorProcessorCache color_cache_;

  QByteArray download_buffer_;

  OperatingMode operating_mode_;

private slots:

};

OLIVE_NAMESPACE_EXIT

#endif // VIDEORENDERWORKER_H
