#include "pixelformatconverter.h"

#include <QtMath>
#include <QDebug>

PixelFormatConverter olive::pix_fmt_conv;

PixelFormatConverter::PixelFormatConverter()
{
  threads_.resize(qMax(1, QThread::idealThreadCount()-1));

  for (int i=0;i<threads_.size();i++) {
    threads_[i] = new PixFmtConvertThread();
    threads_[i]->start(QThread::HighPriority);
  }
}

PixelFormatConverter::~PixelFormatConverter()
{
  for (int i=0;i<threads_.size();i++) {
    delete threads_[i];
  }
}

void PixelFormatConverter::AVFrameToPipeline(uint8_t **input_buffer,
                                             int *input_linesize,
                                             const int& width,
                                             const int& height,
                                             AVPixelFormat input_fmt,
                                             void *output_buffer,
                                             olive::PixelFormat output_fmt)
{
  // TODO: Support all AVPixelFormats. There are many and this will be an ongoing process (though many will likely
  //       not need much effort (or any support at all). In the time-being this function will continue to use swscale
  //       (functioning as abstraction from the rest of the code) while remaining support is added.


  // The main focus of this function is to convert to float. There's currently no intention to support converting to
  // integer formats.
  Q_ASSERT(output_fmt == olive::PIX_FMT_RGBA16F || output_fmt == olive::PIX_FMT_RGBA32F);


  // Wait til other frame conversions are done
  mutex_.lock();


  // Split job between threads
  int lines_per_thread = qCeil(qreal(height)/qreal(threads_.size()));

  // Send conversion job to threads
  for (int i=0;i<threads_.size();i++) {

    int line_start = lines_per_thread*i;

    threads_.at(i)->Convert(input_buffer,
                            input_linesize,
                            width,
                            line_start,
                            qMin(lines_per_thread, height - line_start),
                            input_fmt,
                            output_buffer,
                            output_fmt);
  }

  // Wait for each thread to complete
  for (int i=0;i<threads_.size();i++) {
    threads_.at(i)->wait();
  }


  mutex_.unlock();
}

int PixelFormatConverter::GetBufferSize(olive::PixelFormat format, const int &width, const int &height)
{
  switch (format) {
  case olive::PIX_FMT_RGBA8:
    return width * height;
  case olive::PIX_FMT_RGBA16:
  case olive::PIX_FMT_RGBA16F:
    return 2 * width * height;
  case olive::PIX_FMT_RGBA32F:
    return 4 * width * height;
  default:
    return 0;
  }
}

PixFmtConvertThread::PixFmtConvertThread() :
  cancelled_(false)
{
}

void PixFmtConvertThread::run()
{
  while (!cancelled_) {
    wait_cond_.wait(&mutex_);
    if (cancelled_) break;

    Process();
  }
}

void PixFmtConvertThread::Convert(uint8_t **input_buffer,
                                  int *input_linesize,
                                  const int &width,
                                  const int &line_start,
                                  const int &line_count,
                                  AVPixelFormat input_fmt,
                                  void *output_buffer,
                                  olive::PixelFormat output_fmt)
{
  mutex_.lock();

  input_buffer_ = input_buffer;
  input_linesize_ = input_linesize;
  width_ = width;
  line_start_ = line_start;
  line_count_ = line_count;
  input_fmt_ = input_fmt;
  output_buffer_ = output_buffer;
  output_fmt_ = output_fmt;

  wait_cond_.wakeAll();

  mutex_.unlock();
}

void PixFmtConvertThread::Cancel()
{
  cancelled_ = true;
  wait_cond_.wakeAll();
  wait();
}

void PixFmtConvertThread::Process()
{
  switch (input_fmt_) {
  case AV_PIX_FMT_RGBA:
  {
    int in_start = input_linesize_[0]*line_start_;
    int out_start = width_*line_start_;
    float f;
    int channels = 4; // RGBA

    for (int i=0;i<line_count_;i++) {

      int in_line_start = in_start + input_linesize_[0]*i;
      int in_line_end = in_line_start + width_*channels;
      int out_line_start = out_start + width_*i;

      for (int j=in_line_start;j<in_line_end;j++) {

        // Convert 8-bit integer to float
        f = input_buffer_[0][in_line_start+j] / 255.0f;

        // Place float into output buffer
        // TODO only float support, no half float support yet
        static_cast<float*>(output_buffer_)[out_line_start+j] = f;

      }
    }
    break;
  }
  default:
    qWarning() << "PixFmtConvertThread doesn't yet know how to convert pixel format" << input_fmt_;
  }
}
