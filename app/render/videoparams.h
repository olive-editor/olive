#ifndef VIDEOPARAMS_H
#define VIDEOPARAMS_H

#include "common/rational.h"
#include "pixelformat.h"
#include "rendermodes.h"

class VideoParams
{
public:
  VideoParams();
  VideoParams(const int& width, const int& height, const rational& time_base);

  const int& width() const;
  const int& height() const;
  const rational& time_base() const;

private:
  int width_;
  int height_;
  rational time_base_;

};

class VideoRenderingParams : public VideoParams {
public:
  VideoRenderingParams();
  VideoRenderingParams(const int& width, const int& height, const PixelFormat::Format& format, const int& divider = 1);
  VideoRenderingParams(const int& width, const int& height, const rational& time_base, const PixelFormat::Format& format, const RenderMode::Mode& mode, const int& divider = 1);
  VideoRenderingParams(const VideoParams& params, const PixelFormat::Format& format, const RenderMode::Mode& mode, const int& divider = 1);

  const int& divider() const;
  const int& effective_width() const;
  const int& effective_height() const;

  bool is_valid() const;
  const PixelFormat::Format& format() const;
  const RenderMode::Mode& mode() const;

  bool operator==(const VideoRenderingParams& rhs) const;
  bool operator!=(const VideoRenderingParams& rhs) const;

private:
  void calculate_effective_size();

  PixelFormat::Format format_;
  RenderMode::Mode mode_;

  int divider_;
  int effective_width_;
  int effective_height_;
};

#endif // VIDEOPARAMS_H
