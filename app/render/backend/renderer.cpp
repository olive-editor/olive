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

#include "renderer.h"

OLIVE_NAMESPACE_ENTER

Renderer::Renderer(QObject *parent) :
  QObject(parent)
{

}

Renderer::TexturePtr Renderer::CreateTexture(const VideoParams &param, void *data, int linesize)
{
  QVariant v = CreateNativeTexture(param, data, linesize);

  if (v.isNull()) {
    return nullptr;
  }

  return std::make_shared<Texture>(this, v, param);
}

OLIVE_NAMESPACE_EXIT
