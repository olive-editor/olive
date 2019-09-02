#ifndef RENDERERDOWNLOADTHREAD_H
#define RENDERERDOWNLOADTHREAD_H

#include "rendererthreadbase.h"

class RendererDownloadThread : public RendererThreadBase
{
  Q_OBJECT
public:
  RendererDownloadThread(QOpenGLContext* share_ctx,
                         const int& width,
                         const int& height,
                         const olive::PixelFormat& format,
                         const olive::RenderMode& mode);

  void Queue(RenderTexturePtr texture, const QString &fn, const rational &time);

signals:
  void Downloaded(const rational& time);

protected:
  virtual void ProcessLoop() override;

private:
  GLuint read_buffer_;

  QVector<RenderTexturePtr> texture_queue_;

  QVector<QString> download_filenames_;

  QVector<rational> texture_times_;

  QMutex texture_queue_lock_;

};

using RendererDownloadThreadPtr = std::shared_ptr<RendererDownloadThread>;

#endif // RENDERERDOWNLOADTHREAD_H
