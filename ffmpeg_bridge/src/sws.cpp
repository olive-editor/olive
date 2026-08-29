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

#include "internal.h"

struct FBScaler {
	SwsContext *ctx;
};

FBScaler *fb_scaler_create(int src_width, int src_height, int src_format,
						   int dst_width, int dst_height, int dst_format,
						   int flags)
{
	SwsContext *ctx = sws_getContext(
		src_width, src_height, fb::PixFmtToAV(src_format),
		dst_width, dst_height, fb::PixFmtToAV(dst_format), flags,
		nullptr, nullptr, nullptr);
	if (!ctx) {
		return nullptr;
	}

	FBScaler *s = new FBScaler;
	s->ctx = ctx;
	return s;
}

void fb_scaler_free(FBScaler **scaler)
{
	if (scaler && *scaler) {
		sws_freeContext((*scaler)->ctx);
		delete *scaler;
		*scaler = nullptr;
	}
}

int fb_scaler_set_colorspace(FBScaler *scaler, int colorspace, int jpeg_range)
{
	if (!scaler) {
		return AVERROR(EINVAL);
	}

	const int *coeffs = sws_getCoefficients(
		fb::SwsColorspaceFromAVColorSpace(static_cast<AVColorSpace>(colorspace)));
	return sws_setColorspaceDetails(scaler->ctx, coeffs, jpeg_range, coeffs,
									jpeg_range, 0, 0x10000, 0x10000);
}

int fb_scaler_scale_frame(FBScaler *scaler, FBFrame *dst, const FBFrame *src)
{
	if (!scaler || !dst || !src) {
		return AVERROR(EINVAL);
	}
	return sws_scale_frame(scaler->ctx, dst->frame, src->frame);
}

int fb_scaler_scale_slices(FBScaler *scaler, const uint8_t *const *src_data,
						   const int *src_linesize, int src_height,
						   uint8_t *const *dst_data, const int *dst_linesize)
{
	if (!scaler) {
		return AVERROR(EINVAL);
	}
	return sws_scale(scaler->ctx, src_data, src_linesize, 0, src_height,
					 dst_data, dst_linesize);
}

void fb_get_yuv_coefficients(int colorspace, double out[4])
{
	const int *coeffs = sws_getCoefficients(
		fb::SwsColorspaceFromAVColorSpace(static_cast<AVColorSpace>(colorspace)));
	// Matches the historical usage order: crv, cbu, cgu, cgv
	out[0] = coeffs[0] / 65536.0; // crv
	out[1] = coeffs[1] / 65536.0; // cbu
	out[2] = coeffs[2] / 65536.0; // cgu
	out[3] = coeffs[3] / 65536.0; // cgv
}
