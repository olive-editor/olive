#ifndef PIXELFORMATCONVERTER_H
#define PIXELFORMATCONVERTER_H

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "rendering/pixelformats.h"

class PixFmtConvertThread : public QThread {
public:
  PixFmtConvertThread();

  virtual void run() override;

  void Convert(uint8_t** input_buffer,
               int* input_linesize,
               const int &width,
               const int &line_start,
               const int &line_count,
               AVPixelFormat input_fmt,
               void* output_buffer,
               olive::PixelFormat output_fmt);

  void WaitUntilComplete();

  void Cancel();

private:
  // Main processing function
  void Process();

  // Threading variables
  QWaitCondition wait_cond_;
  QMutex mutex_;
  QMutex mutex2_;
  bool cancelled_;

  // Input variables for conversion
  uint8_t** input_buffer_;
  int* input_linesize_;
  int width_;
  int line_start_;
  int line_count_;
  AVPixelFormat input_fmt_;

  // Output variables for conversion
  void* output_buffer_;
  olive::PixelFormat output_fmt_;
};

/**
 * @brief The PixelFormatConverter class
 *
 * A background process for optimized conversion of frames to Olive's internal pipeline format. It is designed as a
 * replacement of FFmpeg's swscale for all pixel format conversions. It's main advantages over swscale are:
 *
 * * Support for floating-point pixel formats
 * * Better control over the conversion from YCbCr to RGB
 * * Multithreaded by design
 *
 * In the old system, frame pixel formats were converted twice - once to RGBA32 or RGBA64 by swscale, then a second
 * time to RGBA16F or RGBA32F by OpenGL. In addition to the aforementioned advantages, a future goal of this class is to
 * reduce those two steps to one.
 */
class PixelFormatConverter
{
public:
  PixelFormatConverter();

  ~PixelFormatConverter();

  /**
   * @brief Converts data from FFmpeg (or equivalent data) to data ready for Olive's buffer
   *
   * @param input_buffer
   *
   * The buffer of data to convert. This can be fed AVFrame->data directly.
   *
   * @param input_linesize
   *
   * The linesize of the data to convert. This can be fed AVFrame->linesize directly.
   *
   * @param width
   *
   * Width of the frame. This will be the same in the output buffer.
   *
   * @param height
   *
   * Height of the frame. This will be the same in the output buffer.
   *
   * @param input_fmt
   *
   * Expects a value from the AVPixelFormat enum.
   *
   * @param output_buffer
   *
   * The buffer to place the converted data. This function does NOT allocate data, this must be done beforehand.
   * The helper function GetBufferSize() can be used to determine the minimum size (in bytes) that this buffer
   * needs to have allocated for this function to be successful.
   *
   * @param output_fmt
   *
   * The format to convert to. Must be a member of the olive::PixelFormat enum.
   */
  void AVFrameToPipeline(uint8_t** input_buffer,
                         int* input_linesize,
                         const int &width,
                         const int &height,
                         AVPixelFormat input_fmt,
                         void* output_buffer,
                         olive::PixelFormat output_fmt);

  /**
   * @brief Returns the minimum buffer size (in bytes) necessary for a given format, width, and height.
   *
   * @param format
   *
   * The format of the data the buffer should contain. Must be a member of the olive::PixelFormat enum.
   *
   * @param width
   *
   * The width (in pixels) of the buffer.
   *
   * @param height
   *
   * The height (in pixels) of the buffer.
   */
  static int GetBufferSize(olive::PixelFormat format, const int& width, const int& height);

private:
  QMutex mutex_;
  QVector<PixFmtConvertThread*> threads_;
};

namespace olive {
extern PixelFormatConverter* pix_fmt_conv;
}

#endif // PIXELFORMATCONVERTER_H
