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

#include "image.h"

#include "ofxImageEffect.h"

#include <algorithm>

namespace olive
{
namespace plugin
{

static const char *PixelDepthToOfx(core::PixelFormat format)
{
	switch (format) {
	case core::PixelFormat::U8:
		return kOfxBitDepthByte;
	case core::PixelFormat::U16:
		return kOfxBitDepthShort;
	case core::PixelFormat::F16:
		return kOfxBitDepthHalf;
	case core::PixelFormat::F32:
		return kOfxBitDepthFloat;
	case core::PixelFormat::INVALID:
	case core::PixelFormat::COUNT:
		break;
	}

	return kOfxBitDepthNone;
}

static const char *ComponentsToOfx(int channel_count)
{
	switch (channel_count) {
	case 1:
		return kOfxImageComponentAlpha;
	case 3:
		return kOfxImageComponentRGB;
	case 4:
		return kOfxImageComponentRGBA;
	default:
		break;
	}

	return kOfxImageComponentNone;
}

Image::Image(OFX::Host::ImageEffect::ClipInstance &clip_instance)
	: OFX::Host::ImageEffect::Image(clip_instance)
	, width_(0)
	, height_(0)
	, format_(core::PixelFormat::INVALID)
	, premultiplied_alpha_(false)
	, channel_count_(0)
	, row_bytes_(0)
	, bounds_{ 0, 0, 0, 0 }
	, rod_{ 0, 0, 0, 0 }
{
}

Image::Image(OFX::Host::ImageEffect::ClipInstance &clip_instance,
			 const VideoParams &params, const OfxRectI &bounds,
			 const OfxRectI &rod, bool clear)
	: OFX::Host::ImageEffect::Image(clip_instance)
	, width_(0)
	, height_(0)
	, format_(core::PixelFormat::INVALID)
	, premultiplied_alpha_(false)
	, channel_count_(0)
	, row_bytes_(0)
	, bounds_{ 0, 0, 0, 0 }
	, rod_{ 0, 0, 0, 0 }
{
	AllocateFromParams(params, bounds, rod, clear);
}

Image::~Image()
{
}

void Image::AllocateFromParams(const VideoParams &params,
							   const OfxRectI &bounds, const OfxRectI &rod,
							   bool clear)
{
	Allocate(bounds.x2 - bounds.x1, bounds.y2 - bounds.y1, params.format(),
			 params.channel_count(), params.premultiplied_alpha(), bounds, rod,
			 clear);
}

void Image::EnsureAllocatedFromParams(const VideoParams &params,
									  const OfxRectI &bounds,
									  const OfxRectI &rod, bool clear)
{
	bool same = (width_ == bounds.x2 - bounds.x1) &&
				(height_ == bounds.y2 - bounds.y1) &&
				(format_ == params.format()) &&
				(channel_count_ == params.channel_count()) &&
				(premultiplied_alpha_ == params.premultiplied_alpha()) &&
				(bounds_.x1 == bounds.x1) && (bounds_.y1 == bounds.y1) &&
				(bounds_.x2 == bounds.x2) && (bounds_.y2 == bounds.y2) &&
				(rod_.x1 == rod.x1) && (rod_.y1 == rod.y1) &&
				(rod_.x2 == rod.x2) && (rod_.y2 == rod.y2);

	if (!same) {
		AllocateFromParams(params, bounds, rod, clear);
	} else if (clear && !image_.empty()) {
		std::fill(image_.begin(), image_.end(), 0);
	}
}

void Image::Allocate(int width, int height, core::PixelFormat format,
					 int channel_count, bool premultiplied_alpha,
					 const OfxRectI &bounds, const OfxRectI &rod, bool clear)
{
	width_ = width;
	height_ = height;
	format_ = format;
	channel_count_ = channel_count;
	premultiplied_alpha_ = premultiplied_alpha;
	bounds_ = bounds;
	rod_ = rod;

	int bytes_per_component = format_.byte_count();
	row_bytes_ = width_ * channel_count_ * bytes_per_component;
	int buffer_size = row_bytes_ * height_;
	if (buffer_size < 0) {
		buffer_size = 0;
	}

	image_.resize(static_cast<size_t>(buffer_size));
	if (clear && !image_.empty()) {
		std::fill(image_.begin(), image_.end(), 0);
	}

	setPointerProperty(kOfxImagePropData, image_.data());
	setIntProperty(kOfxImagePropRowBytes, row_bytes_);
	setIntProperty(kOfxImagePropBounds, bounds.x1, 0);
	setIntProperty(kOfxImagePropBounds, bounds.y1, 1);
	setIntProperty(kOfxImagePropBounds, bounds.x2, 2);
	setIntProperty(kOfxImagePropBounds, bounds.y2, 3);
	setIntProperty(kOfxImagePropRegionOfDefinition, rod.x1, 0);
	setIntProperty(kOfxImagePropRegionOfDefinition, rod.y1, 1);
	setIntProperty(kOfxImagePropRegionOfDefinition, rod.x2, 2);
	setIntProperty(kOfxImagePropRegionOfDefinition, rod.y2, 3);
	setStringProperty(kOfxImageEffectPropComponents,
					  ComponentsToOfx(channel_count_));
	setStringProperty(kOfxImageEffectPropPixelDepth, PixelDepthToOfx(format_));
	setStringProperty(kOfxImageEffectPropPreMultiplication,
					  premultiplied_alpha_ ? kOfxImagePreMultiplied :
											 kOfxImageUnPreMultiplied);
}

core::PixelFormat Image::pixel_format()
{
	if (format_ != core::PixelFormat::INVALID) {
		return format_;
	}

	std::string type = getStringProperty(kOfxImageEffectPropPixelDepth);
	if (type == kOfxBitDepthByte) {
		format_ = core::PixelFormat::U8;
	} else if (type == kOfxBitDepthShort) {
		format_ = core::PixelFormat::U16;
	} else if (type == kOfxBitDepthHalf) {
		format_ = core::PixelFormat::F16;
	} else if (type == kOfxBitDepthFloat) {
		format_ = core::PixelFormat::F32;
	} else {
		format_ = core::PixelFormat::INVALID;
	}
	return format_;
}

bool Image::premultiplied_alpha()
{
	std::string premultiplied =
		getStringProperty(kOfxImageEffectPropPreMultiplication);
	premultiplied_alpha_ = (premultiplied == kOfxImagePreMultiplied);
	return premultiplied_alpha_;
}

int Image::width()
{
	int bounds[4] = { 0 };
	getIntPropertyN(kOfxImagePropBounds, bounds, 4);
	width_ = bounds[2] - bounds[0];
	return width_;
}

int Image::height()
{
	int bounds[4] = { 0 };
	getIntPropertyN(kOfxImagePropBounds, bounds, 4);
	height_ = bounds[3] - bounds[1];
	return height_;
}

int Image::channel_count()
{
	std::string type = getStringProperty(kOfxImageEffectPropComponents);
	if (type == kOfxImageComponentAlpha) {
		channel_count_ = 1;
	} else if (type == kOfxImageComponentRGBA) {
		channel_count_ = 4;
	} else if (type == kOfxImageComponentRGB) {
		channel_count_ = 3;
	} else {
		channel_count_ = 0;
	}
	return channel_count_;
}

} // namespace plugin
} // namespace olive
