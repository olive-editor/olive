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
  VideoRenderWorker(VideoRenderFrameCache* frame_cache, QObject* parent = nullptr);

  void SetParameters(const VideoRenderingParams& video_params);

signals:
  void CompletedFrame(NodeDependency path, qint64 job_time, QByteArray hash, QVariant value);

  void CompletedDownload(NodeDependency path, qint64 job_time, QByteArray hash);

  void HashAlreadyBeingCached(NodeDependency path, qint64 job_time, QByteArray hash);

  void HashAlreadyExists(NodeDependency path, qint64 job_time, QByteArray hash);

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  const VideoRenderingParams& video_params();

  virtual void ParametersChangedEvent(){}

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) = 0;

  virtual NodeValueTable RenderInternal(const NodeDependency& path, const qint64& job_time) override;

  virtual FramePtr RetrieveFromDecoder(DecoderPtr decoder, const TimeRange& range) override;

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range) override;

  ColorProcessorCache* color_cache();

private:
  void HashNodeRecursively(QCryptographicHash* hash, const Node *n, const rational &time);

  void Download(NodeDependency dep, QByteArray hash, QVariant texture, QString filename);

  VideoRenderingParams video_params_;

  VideoRenderFrameCache* frame_cache_;

  ColorProcessorCache color_cache_;

  QByteArray download_buffer_;

private slots:

};

#endif // VIDEORENDERWORKER_H
