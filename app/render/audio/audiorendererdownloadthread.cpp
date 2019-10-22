#include "audiorendererdownloadthread.h"

#include <QFile>
#include <QFloat16>
#include <OpenImageIO/imageio.h>

#include "common/define.h"
#include "render/pixelservice.h"

AudioRendererDownloadThread::AudioRendererDownloadThread(QOpenGLContext *share_ctx,
                                                         const int &width,
                                                         const int &height,
                                                         const int &divider,
                                                         const olive::PixelFormat &format,
                                                         const olive::RenderMode &mode) :
  AudioRendererThreadBase(share_ctx, width, height, divider, format, mode),
  cancelled_(false)
{
}

void AudioRendererDownloadThread::Queue(RenderTexturePtr texture, const QString& fn, const QByteArray &hash)
{
  texture_queue_lock_.lock();

  texture_queue_.append({texture, fn, hash});

  wait_cond_.wakeAll();

  texture_queue_lock_.unlock();
}

void AudioRendererDownloadThread::Cancel()
{
  cancelled_ = true;

  texture_queue_lock_.lock();
  wait_cond_.wakeAll();
  texture_queue_lock_.unlock();

  wait();
}

void AudioRendererDownloadThread::ProcessLoop()
{
  QOpenGLFunctions* f = render_instance()->context()->functions();
  QOpenGLExtraFunctions* xf = render_instance()->context()->extraFunctions();

  f->glGenFramebuffers(1, &read_buffer_);

  DownloadQueueEntry entry;

  int buffer_size = PixelService::GetBufferSize(render_instance()->format(),
                                                render_instance()->width(),
                                                render_instance()->height());

  QVector<uchar> data_buffer;
  data_buffer.resize(buffer_size);

  PixelFormatInfo format_info = PixelService::GetPixelFormatInfo(render_instance()->format());

  // Set up OIIO::ImageSpec for compressing cached images on disk
  OIIO::ImageSpec spec(render_instance()->width(), render_instance()->height(), kRGBAChannels, format_info.oiio_desc);
  spec.attribute("compression", "dwaa:200");

  while (!cancelled_) {
    // Check queue for textures to download (use mutex to prevent collisions)
    texture_queue_lock_.lock();

    while (texture_queue_.isEmpty()) {
      // Main waiting condition
      wait_cond_.wait(&texture_queue_lock_);

      if (cancelled_) {
        break;
      }
    }
    if (cancelled_) {
      texture_queue_lock_.unlock();
      break;
    }

    entry = texture_queue_.takeFirst();

    texture_queue_lock_.unlock();

    // Download the texture

    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, read_buffer_);

    xf->glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               entry.texture->texture(),
                               0);

    f->glReadPixels(0,
                    0,
                    entry.texture->width(),
                    entry.texture->height(),
                    format_info.pixel_format,
                    format_info.pixel_type,
                    data_buffer.data());

    xf->glFramebufferTexture2D(GL_READ_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               0,
                               0);

    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    std::string working_fn_std = entry.filename.toStdString();

    std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(working_fn_std);

    if (out) {
      out->open(working_fn_std, spec);
      out->write_image(format_info.oiio_desc, data_buffer.data());
      out->close();

      emit Downloaded(entry.hash);
    } else {
      qWarning() << tr("Failed to open output file \"%1\"").arg(entry.filename);
    }
  }

  f->glDeleteFramebuffers(1, &read_buffer_);
}
