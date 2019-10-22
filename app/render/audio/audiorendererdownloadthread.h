#ifndef AUDIORENDERERDOWNLOADTHREAD_H
#define AUDIORENDERERDOWNLOADTHREAD_H

#include "audiorendererthreadbase.h"

/*class AudioRendererDownloadThread : public AudioRendererThreadBase
{
  Q_OBJECT
public:
  AudioRendererDownloadThread(QOpenGLContext* share_ctx,
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

using AudioRendererDownloadThreadPtr = std::shared_ptr<AudioRendererDownloadThread>;*/

#endif // AUDIORENDERERDOWNLOADTHREAD_H
