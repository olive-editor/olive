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

#ifndef RENDERTEXTURE_H
#define RENDERTEXTURE_H

#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class Renderer;

class Texture
{
public:
  enum Type {
    k2D,
    k3D
  };

  enum Interpolation {
    kNearest,
    kLinear,
    kMipmappedLinear
  };

  enum ChannelFormat {
    kRGBA,
    kRGB,
    kRedOnly
  };

  static const Interpolation kDefaultInterpolation;

  Texture(Renderer* renderer, const QVariant& native, const VideoParams& param, Type type) :
    renderer_(renderer),
    params_(param),
    id_(native),
    meaningful_alpha_(true),
    type_(type)
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

  void Upload(void* data, int linesize);

  int width() const
  {
    return params_.width();
  }

  int height() const
  {
    return params_.height();
  }

  PixelFormat::Format format() const
  {
    return params_.format();
  }

  int divider() const
  {
    return params_.divider();
  }

  const rational& pixel_aspect_ratio() const
  {
    return params_.pixel_aspect_ratio();
  }

  bool has_meaningful_alpha() const
  {
    return meaningful_alpha_;
  }

  void set_has_meaningful_alpha(bool e)
  {
    meaningful_alpha_ = e;
  }

  Type type() const
  {
    return type_;
  }

private:
  Renderer* renderer_;

  VideoParams params_;

  QVariant id_;

  bool meaningful_alpha_;

  Type type_;

};

using TexturePtr = std::shared_ptr<Texture>;

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::TexturePtr);

#endif // RENDERTEXTURE_H
