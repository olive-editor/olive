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

struct FBResampler {
	SwrContext *ctx;
};

FBResampler *fb_resampler_create(uint64_t out_layout_mask, int out_format,
								 int out_rate, uint64_t in_layout_mask,
								 int in_format, int in_rate)
{
	AVChannelLayout out_layout, in_layout;
	fb::ChannelLayoutFromMask(&out_layout, out_layout_mask, 0);
	fb::ChannelLayoutFromMask(&in_layout, in_layout_mask, 0);

	SwrContext *ctx = nullptr;
	int r = swr_alloc_set_opts2(&ctx, &out_layout,
								static_cast<AVSampleFormat>(out_format), out_rate,
								&in_layout, static_cast<AVSampleFormat>(in_format),
								in_rate, 0, nullptr);

	av_channel_layout_uninit(&out_layout);
	av_channel_layout_uninit(&in_layout);

	if (r < 0 || !ctx) {
		return nullptr;
	}

	if (swr_init(ctx) < 0) {
		swr_free(&ctx);
		return nullptr;
	}

	FBResampler *resampler = new FBResampler;
	resampler->ctx = ctx;
	return resampler;
}

void fb_resampler_free(FBResampler **resampler)
{
	if (resampler && *resampler) {
		swr_free(&(*resampler)->ctx);
		delete *resampler;
		*resampler = nullptr;
	}
}

int fb_resampler_get_out_samples(FBResampler *resampler, int in_samples)
{
	if (!resampler) {
		return AVERROR(EINVAL);
	}
	return swr_get_out_samples(resampler->ctx, in_samples);
}

int fb_resampler_convert(FBResampler *resampler, uint8_t **out, int out_count,
						 const uint8_t **in, int in_count)
{
	if (!resampler) {
		return AVERROR(EINVAL);
	}
	return swr_convert(resampler->ctx, out, out_count, in, in_count);
}

int fb_resampler_convert_frame(FBResampler *resampler, uint8_t **out,
							   int out_count, const FBFrame *in_frame)
{
	if (!resampler || !in_frame || !in_frame->frame) {
		return AVERROR(EINVAL);
	}
	return swr_convert(resampler->ctx, out, out_count,
					   (const uint8_t **)in_frame->frame->extended_data,
					   in_frame->frame->nb_samples);
}
