/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "render/videoparams.h"

namespace olive {

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

  static const Interpolation kDefaultInterpolation;

  Texture(Renderer* renderer, const QVariant& native, const VideoParams& param, Type type) :
    renderer_(renderer),
    params_(param),
    id_(native),
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
    return params_.effective_width();
  }

  int height() const
  {
    return params_.effective_height();
  }

  VideoParams::Format format() const
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

  Type type() const
  {
    return type_;
  }

private:
  Renderer* renderer_;

  VideoParams params_;

  QVariant id_;

  Type type_;

};

using TexturePtr = std::shared_ptr<Texture>;

}

Q_DECLARE_METATYPE(olive::TexturePtr)

#endif // RENDERTEXTURE_H
