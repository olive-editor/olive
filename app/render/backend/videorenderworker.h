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
  VideoRenderWorker(DecoderCache* decoder_cache, ColorProcessorCache* color_cache, VideoRenderFrameCache* frame_cache, QObject* parent = nullptr);

  void SetParameters(const VideoRenderingParams& video_params);

public slots:
  void Download(NodeDependency dep, QByteArray hash, QVariant texture, QString filename);

signals:
  void CompletedFrame(NodeDependency path, QByteArray hash, NodeValueTable value);

  void CompletedDownload(NodeDependency path, QByteArray hash);

  void HashAlreadyBeingCached(NodeDependency path, QByteArray hash);

  void HashAlreadyExists(NodeDependency path, QByteArray hash);

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  const VideoRenderingParams& video_params();

  virtual void ParametersChangedEvent(){}

  virtual void TextureToBuffer(const QVariant& texture, QByteArray& buffer) = 0;

  virtual NodeValueTable RenderInternal(const NodeDependency& path) override;

  virtual FramePtr RetrieveFromDecoder(DecoderPtr decoder, const TimeRange& range) override;

  virtual NodeValueTable RenderBlock(TrackOutput *track, const TimeRange& range) override;

  ColorProcessorCache* color_cache() const;

private:
  void ProcessNode();

  void HashNodeRecursively(QCryptographicHash* hash, Node *n, const rational &time);

  VideoRenderingParams video_params_;

  VideoRenderFrameCache* frame_cache_;

  ColorProcessorCache* color_cache_;

  QByteArray download_buffer_;

private slots:

};

#endif // VIDEORENDERWORKER_H
