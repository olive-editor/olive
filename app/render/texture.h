/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef RENDERTEXTURE_H
#define RENDERTEXTURE_H

#include <memory>
#include <QVariant>

#include "render/videoparams.h"

namespace olive {

class AcceleratedJob;
class Renderer;

class Texture;
using TexturePtr = std::shared_ptr<Texture>;

class Texture
{
public:
  enum Interpolation {
    kNearest,
    kLinear,
    kMipmappedLinear
  };

  enum WrapMode {
    kClamp,
    kRepeat,
    kMirroredRepeat
  };

  static const Interpolation kDefaultInterpolation;
  static const WrapMode kDefaultWrapMode;

  /**
   * @brief Construct a dummy texture with no renderer backend
   */
  Texture(const VideoParams& param) :
    renderer_(nullptr),
    params_(param),
    job_(nullptr)
  {
  }

  template <typename T>
  Texture(const VideoParams &p, const T &j) :
    Texture(p)
  {
    job_ = new T(j);
  }

  /**
   * @brief Construct a real texture linked to a renderer backend
   */
  Texture(Renderer* renderer, const QVariant& native, const VideoParams& param) :
    renderer_(renderer),
    params_(param),
    id_(native),
    job_(nullptr)
  {
  }

  ~Texture();

  QVariant id() const
  {
    return id_;
  }

  const VideoParams& params() const
  {
    return params_;
  }

  template <typename T>
  static TexturePtr Job(const VideoParams &p, const T &j)
  {
    return std::make_shared<Texture>(p, j);
  }

  template <typename T>
  TexturePtr toJob(const T &job)
  {
    return Texture::Job(params_, job);
  }

  void Upload(void* data, int linesize);

  void Download(void* data, int linesize);

  bool IsDummy() const
  {
    return !renderer_;
  }

  int width() const
  {
    return params_.effective_width();
  }

  int height() const
  {
    return params_.effective_height();
  }

  void set_wrap_mode(Texture::WrapMode wrapMode) const
  {
    wrapMode = wrapMode;
  }

  QVector2D virtual_resolution() const
  {
    return QVector2D(params_.square_pixel_width(), params_.height());
  }

  PixelFormat format() const
  {
    return params_.format();
  }

  int channel_count() const
  {
    return params_.channel_count();
  }

  int divider() const
  {
    return params_.divider();
  }

  const rational& pixel_aspect_ratio() const
  {
    return params_.pixel_aspect_ratio();
  }

  Renderer* renderer() const
  {
    return renderer_;
  }

  bool IsJob() const { return job_; }
  AcceleratedJob *job() const { return job_; }

private:
  Renderer* renderer_;

  VideoParams params_;

  QVariant id_;

  AcceleratedJob *job_;

};

}

Q_DECLARE_METATYPE(olive::TexturePtr)

#endif // RENDERTEXTURE_H
