#ifndef RENDERERDOWNLOADTHREAD_H
#define RENDERERDOWNLOADTHREAD_H

#include "videorendererthreadbase.h"

class VideoRendererDownloadThread : public VideoRendererThreadBase
{
  Q_OBJECT
public:
  VideoRendererDownloadThread(QOpenGLContext* share_ctx,
                         const int& width,
                         const int& height,
                         const int &divider,
                         const olive::PixelFormat& format,
                         const olive::RenderMode& mode);

  void Queue(RenderTexturePtr texture, const QString &fn, const QByteArray &hash);

public slots:
  virtual void Cancel() override;

signals:
  void Downloaded(const QByteArray& hash);

protected:
  virtual void ProcessLoop() override;

private:
  struct DownloadQueueEntry {
    RenderTexturePtr texture;
    QString filename;
    QByteArray hash;
  };

  GLuint read_buffer_;

  QVector<DownloadQueueEntry> texture_queue_;

  QMutex texture_queue_lock_;

  QAtomicInt cancelled_;

  QByteArray hash_;

};

using RendererDownloadThreadPtr = std::shared_ptr<VideoRendererDownloadThread>;

#endif // RENDERERDOWNLOADTHREAD_H
