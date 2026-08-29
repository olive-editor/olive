/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2026 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef OLIVE_EDITOR_PLUGIN_IMAGE_H
#define OLIVE_EDITOR_PLUGIN_IMAGE_H

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxhClip.h"
#include "olive/core/render/pixelformat.h"
#include "render/loopmode.h"
#include "render/videoparams.h"
#include <cstdint>
#include <string>
#include <vector>
namespace olive
{
namespace plugin
{
class Image : public OFX::Host::ImageEffect::Image {
public:
	Image(OFX::Host::ImageEffect::ClipInstance &clip_instance);
	Image(OFX::Host::ImageEffect::ClipInstance &clip_instance,
		  const VideoParams &params, const OfxRectI &bounds,
		  const OfxRectI &rod, bool clear = true);
	~Image();
	uint8_t *data()
	{
		return (uint8_t *)getPointerProperty(kOfxImagePropData);
	}
	int width();
	int height();
	core::PixelFormat pixel_format();
	bool premultiplied_alpha();
	int channel_count();

	void AllocateFromParams(const VideoParams &params, const OfxRectI &bounds,
							const OfxRectI &rod, bool clear = true);
	void EnsureAllocatedFromParams(const VideoParams &params,
								   const OfxRectI &bounds, const OfxRectI &rod,
								   bool clear = false);
	void Allocate(int width, int height, core::PixelFormat format,
				  int channel_count, bool premultiplied_alpha,
				  const OfxRectI &bounds, const OfxRectI &rod,
				  bool clear = true);
	int row_bytes() const
	{
		return row_bytes_;
	}

protected:
	std::vector<uint8_t> image_;
	int width_;
	int height_;
	core::PixelFormat format_;
	bool premultiplied_alpha_;
	int channel_count_;
	int row_bytes_;
	OfxRectI bounds_;
	OfxRectI rod_;
};
}
}

#endif //OLIVE_EDITOR_PLUGIN_IMAGE_H
