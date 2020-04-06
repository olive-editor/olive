/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef VIDEOPARAMS_H
#define VIDEOPARAMS_H

#include "common/rational.h"
#include "pixelformat.h"
#include "rendermodes.h"

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT

#endif // VIDEOPARAMS_H
