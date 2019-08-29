#include "rendererdownloadthread.h"

#include <QFile>

#include "render/pixelservice.h"

RendererDownloadThread::RendererDownloadThread(QOpenGLContext *share_ctx,
                                               const int &width,
                                               const int &height,
                                               const olive::PixelFormat &format,
                                               const olive::RenderMode &mode) :
  RendererThreadBase(share_ctx, width, height, format, mode)
{

}

void RendererDownloadThread::Queue(RenderTexturePtr texture, const QString& fn)
{
  texture_queue_lock_.lock();

  texture_queue_.append(texture);
  download_filenames_.append(fn);

  wait_cond_.wakeAll();

  texture_queue_lock_.unlock();
}

void RendererDownloadThread::ProcessLoop()
{
  QOpenGLFunctions* f = render_instance()->context()->functions();
  QOpenGLExtraFunctions* xf = render_instance()->context()->extraFunctions();

  f->glGenFramebuffers(1, &read_buffer_);

  RenderTexturePtr working_texture;
  QString working_filename;

  int buffer_size = PixelService::GetBufferSize(render_instance()->format(),
                                                render_instance()->width(),
                                                render_instance()->height());

  uchar* data_buffer = new uchar[buffer_size];

  while (!Cancelled()) {
    // Check queue for textures to download (use mutex to prevent collisions)
    texture_queue_lock_.lock();

    do {
      if (texture_queue_.isEmpty()) {
        working_texture = nullptr;
      } else {
        working_texture = texture_queue_.takeFirst();
        working_filename = download_filenames_.takeFirst();
      }

      if (working_texture == nullptr) {
        // Main waiting condition
        wait_cond_.wait(&texture_queue_lock_);
      }
    } while (working_texture == nullptr);

    texture_queue_lock_.unlock();

    // Download the texture
    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, read_buffer_);

    xf->glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               working_texture->texture(),
                               0);

    PixelFormatInfo format_info = PixelService::GetPixelFormatInfo(working_texture->format());

    f->glReadPixels(0,
                    0,
                    working_texture->width(),
                    working_texture->height(),
                    format_info.pixel_format,
                    format_info.pixel_type,
                    data_buffer);

    render_instance()->context()->extraFunctions()->glFramebufferTexture2D(
          GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0
          );

    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    QFile data_dump(working_filename);

    if (data_dump.open(QFile::WriteOnly)) {
      data_dump.write(reinterpret_cast<char*>(data_buffer), buffer_size);

      data_dump.close();
    }

    qDebug() << "Saved" << working_filename;

  }

  delete [] data_buffer;

  f->glDeleteFramebuffers(1, &read_buffer_);
}
