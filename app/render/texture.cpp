/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "texture.h"

#include "render/job/acceleratedjob.h"
#include "renderer.h"

namespace olive
{

const Texture::Interpolation Texture::kDefaultInterpolation =
	Texture::kMipmappedLinear;

Texture::~Texture()
{
	if (IsRendererAlive()) {
		renderer_->DestroyTexture(this);
	}

	if (job_) {
		delete job_;
	}
}

void Texture::Upload(void *data, int linesize)
{
	if (IsRendererAlive()) {
		renderer_->UploadToTexture(this->id(), this->params(), data, linesize);
	}
}

void Texture::Download(void *data, int linesize)
{
	if (IsRendererAlive()) {
		renderer_->DownloadFromTexture(this->id(), this->params(), data,
									   linesize);
	}
}

}
