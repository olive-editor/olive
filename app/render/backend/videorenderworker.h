#ifndef VIDEORENDERWORKER_H
#define VIDEORENDERWORKER_H

#include <QCryptographicHash>

#include "colorprocessorcache.h"
#include "node/dependency.h"
#include "render/videoparams.h"
#include "renderworker.h"
#include "videorenderframecache.h"

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

  VideoRenderWorker(VideoRenderFrameCache* frame_cache, DecoderCache *decoder_cache, QObject* parent = nullptr);

  void SetParameters(const VideoRenderingParams& video_params);

  void SetOperatingMode(const OperatingMode& mode);

signals:
  void CompletedFrame(NodeDependency CurrentPath, qint64 job_time, QByteArray hash, QVariant value);

  void CompletedDownload(NodeDependency CurrentPath, qint64 job_time, QByteArray hash, bool texture_existed);

  void HashAlreadyBeingCached(NodeDependency CurrentPath, qint64 job_time, QByteArray hash);

  void HashAlreadyExists(NodeDependency CurrentPath, qint64 job_time, QByteArray hash);

  void Aborted();

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  const VideoRenderingParams& video_params();

  virtual void ParametersChangedEvent(){}

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) = 0;

  virtual NodeValueTable RenderInternal(const NodeDependency& CurrentPath, const qint64& job_time) override;

  virtual FramePtr RetrieveFromDecoder(DecoderPtr decoder, const TimeRange& range) override;

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range) override;

  virtual void ReportUnavailableFootage(StreamPtr stream, Decoder::RetrieveState state, const rational& stream_time) override;

  ColorProcessorCache* color_cache();

private:
  void HashNodeRecursively(QCryptographicHash* hash, const Node *n, const rational &time);

  void Download(QVariant texture, QString filename);

  void ResizeDownloadBuffer();

  VideoRenderingParams video_params_;

  VideoRenderFrameCache* frame_cache_;

  ColorProcessorCache color_cache_;

  QByteArray download_buffer_;

  OperatingMode operating_mode_;

private slots:

};

#endif // VIDEORENDERWORKER_H
