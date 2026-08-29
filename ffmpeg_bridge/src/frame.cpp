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

#include <string.h>

FBFrame *fb_frame_alloc(void)
{
	AVFrame *avf = av_frame_alloc();
	if (!avf) {
		return nullptr;
	}

	FBFrame *f = new FBFrame;
	f->frame = avf;
	return f;
}

void fb_frame_free(FBFrame **frame)
{
	if (frame && *frame) {
		av_frame_free(&(*frame)->frame);
		delete *frame;
		*frame = nullptr;
	}
}

void fb_frame_unref(FBFrame *frame)
{
	if (frame && frame->frame) {
		av_frame_unref(frame->frame);
	}
}

int fb_frame_get_buffer(FBFrame *frame, int align)
{
	if (!frame || !frame->frame) {
		return AVERROR(EINVAL);
	}
	return av_frame_get_buffer(frame->frame, align);
}

int fb_frame_make_writable(FBFrame *frame)
{
	if (!frame || !frame->frame) {
		return AVERROR(EINVAL);
	}
	return av_frame_make_writable(frame->frame);
}

int fb_frame_copy_props(FBFrame *dst, const FBFrame *src)
{
	if (!dst || !src) {
		return AVERROR(EINVAL);
	}
	return av_frame_copy_props(dst->frame, src->frame);
}

int fb_frame_hw_transfer_data(FBFrame *dst, const FBFrame *src)
{
	if (!dst || !src) {
		return AVERROR(EINVAL);
	}
	return av_hwframe_transfer_data(dst->frame, src->frame, 0);
}

int fb_frame_is_hw(const FBFrame *frame)
{
	return frame && frame->frame && frame->frame->hw_frames_ctx != nullptr;
}

int fb_frame_get_width(const FBFrame *frame)
{
	return frame ? frame->frame->width : 0;
}

void fb_frame_set_width(FBFrame *frame, int width)
{
	if (frame) {
		frame->frame->width = width;
	}
}

int fb_frame_get_height(const FBFrame *frame)
{
	return frame ? frame->frame->height : 0;
}

void fb_frame_set_height(FBFrame *frame, int height)
{
	if (frame) {
		frame->frame->height = height;
	}
}

int fb_frame_get_format(const FBFrame *frame)
{
	return frame ? fb::PixFmtFromAV(AVPixelFormat(frame->frame->format)) :
				   FB_PIX_FMT_NONE;
}

void fb_frame_set_format(FBFrame *frame, int format)
{
	if (frame) {
		frame->frame->format = fb::PixFmtToAV(format);
	}
}

int64_t fb_frame_get_pts(const FBFrame *frame)
{
	return frame ? frame->frame->pts : FB_NOPTS_VALUE;
}

void fb_frame_set_pts(FBFrame *frame, int64_t pts)
{
	if (frame) {
		frame->frame->pts = pts;
	}
}

int64_t fb_frame_get_best_effort_timestamp(const FBFrame *frame)
{
	return frame ? frame->frame->best_effort_timestamp : FB_NOPTS_VALUE;
}

int fb_frame_get_nb_samples(const FBFrame *frame)
{
	return frame ? frame->frame->nb_samples : 0;
}

void fb_frame_set_nb_samples(FBFrame *frame, int nb_samples)
{
	if (frame) {
		frame->frame->nb_samples = nb_samples;
	}
}

int fb_frame_get_sample_rate(const FBFrame *frame)
{
	return frame ? frame->frame->sample_rate : 0;
}

void fb_frame_set_sample_rate(FBFrame *frame, int sample_rate)
{
	if (frame) {
		frame->frame->sample_rate = sample_rate;
	}
}

int fb_frame_get_color_range(const FBFrame *frame)
{
	return frame ? int(frame->frame->color_range) : FB_COLOR_RANGE_UNSPEC;
}

void fb_frame_set_color_range(FBFrame *frame, int color_range)
{
	if (frame) {
		frame->frame->color_range = static_cast<AVColorRange>(color_range);
	}
}

int fb_frame_get_colorspace(const FBFrame *frame)
{
	return frame ? int(frame->frame->colorspace) : FB_COL_SPC_UNSPEC;
}

void fb_frame_set_colorspace(FBFrame *frame, int colorspace)
{
	if (frame) {
		frame->frame->colorspace = static_cast<AVColorSpace>(colorspace);
	}
}

uint64_t fb_frame_get_channel_layout_mask(const FBFrame *frame)
{
	if (!frame) {
		return 0;
	}
	if (frame->frame->ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
		return frame->frame->ch_layout.u.mask;
	}
	return 0;
}

void fb_frame_set_channel_layout_mask(FBFrame *frame, uint64_t mask)
{
	if (frame) {
		av_channel_layout_uninit(&frame->frame->ch_layout);
		av_channel_layout_from_mask(&frame->frame->ch_layout, mask);
	}
}

uint8_t *fb_frame_get_data(FBFrame *frame, int plane)
{
	if (!frame || plane < 0 || plane >= AV_NUM_DATA_POINTERS) {
		return nullptr;
	}
	return frame->frame->data[plane];
}

const uint8_t *fb_frame_get_data_const(const FBFrame *frame, int plane)
{
	if (!frame || plane < 0 || plane >= AV_NUM_DATA_POINTERS) {
		return nullptr;
	}
	return frame->frame->data[plane];
}

void fb_frame_set_data(FBFrame *frame, int plane, uint8_t *data)
{
	if (frame && plane >= 0 && plane < AV_NUM_DATA_POINTERS) {
		frame->frame->data[plane] = data;
	}
}

int fb_frame_get_linesize(const FBFrame *frame, int plane)
{
	if (!frame || plane < 0 || plane >= AV_NUM_DATA_POINTERS) {
		return 0;
	}
	return frame->frame->linesize[plane];
}

void fb_frame_set_linesize(FBFrame *frame, int plane, int linesize)
{
	if (frame && plane >= 0 && plane < AV_NUM_DATA_POINTERS) {
		frame->frame->linesize[plane] = linesize;
	}
}
