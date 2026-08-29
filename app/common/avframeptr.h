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

#ifndef AVFRAMEPTR_H
#define AVFRAMEPTR_H

#include <stdint.h>

#include <memory>

#include <ffmpeg_bridge/ffmpeg_bridge.h>

namespace olive
{

/**
 * @brief C++ adapter around the ffmpeg_bridge frame handle
 *
 * Mirrors the AVFrame field access the codebase used to perform directly,
 * but every operation goes through the pure C bridge API so the editor
 * never touches FFmpeg itself. The underlying frame object always lives
 * inside the bridge library.
 */
class AVFrame {
public:
	AVFrame() :
		handle_(fb_frame_alloc())
	{
	}

	explicit AVFrame(FBFrame *handle) :
		handle_(handle)
	{
	}

	~AVFrame()
	{
		if (handle_) {
			fb_frame_free(&handle_);
		}
	}

	AVFrame(const AVFrame &) = delete;
	AVFrame &operator=(const AVFrame &) = delete;

	FBFrame *handle() const { return handle_; }

	int width() const { return fb_frame_get_width(handle_); }
	void set_width(int w) { fb_frame_set_width(handle_, w); }
	int height() const { return fb_frame_get_height(handle_); }
	void set_height(int h) { fb_frame_set_height(handle_, h); }
	int format() const { return fb_frame_get_format(handle_); }
	void set_format(int f) { fb_frame_set_format(handle_, f); }
	int64_t pts() const { return fb_frame_get_pts(handle_); }
	void set_pts(int64_t p) { fb_frame_set_pts(handle_, p); }
	int64_t best_effort_timestamp() const
	{
		return fb_frame_get_best_effort_timestamp(handle_);
	}
	int nb_samples() const { return fb_frame_get_nb_samples(handle_); }
	void set_nb_samples(int n) { fb_frame_set_nb_samples(handle_, n); }
	int sample_rate() const { return fb_frame_get_sample_rate(handle_); }
	void set_sample_rate(int r) { fb_frame_set_sample_rate(handle_, r); }
	int color_range() const { return fb_frame_get_color_range(handle_); }
	void set_color_range(int r) { fb_frame_set_color_range(handle_, r); }
	int colorspace() const { return fb_frame_get_colorspace(handle_); }
	void set_colorspace(int cs) { fb_frame_set_colorspace(handle_, cs); }
	uint64_t channel_layout_mask() const
	{
		return fb_frame_get_channel_layout_mask(handle_);
	}
	void set_channel_layout_mask(uint64_t m)
	{
		fb_frame_set_channel_layout_mask(handle_, m);
	}

	bool is_hw() const { return fb_frame_is_hw(handle_) != 0; }
	int hw_transfer_data(const AVFrame *src)
	{
		return fb_frame_hw_transfer_data(handle_, src->handle_);
	}
	int get_buffer(int align) { return fb_frame_get_buffer(handle_, align); }
	int make_writable() { return fb_frame_make_writable(handle_); }

	uint8_t *data(int plane) { return fb_frame_get_data(handle_, plane); }
	const uint8_t *data(int plane) const
	{
		return fb_frame_get_data_const(handle_, plane);
	}
	void set_data(int plane, uint8_t *d)
	{
		fb_frame_set_data(handle_, plane, d);
	}
	int linesize(int plane) const
	{
		return fb_frame_get_linesize(handle_, plane);
	}
	void set_linesize(int plane, int l)
	{
		fb_frame_set_linesize(handle_, plane, l);
	}

private:
	FBFrame *handle_;
};

using AVFramePtr = std::shared_ptr<AVFrame>;

inline AVFramePtr CreateAVFramePtr(FBFrame *f)
{
	return std::make_shared<AVFrame>(f);
}

inline AVFramePtr CreateAVFramePtr()
{
	return std::make_shared<AVFrame>();
}

}

#endif // AVFRAMEPTR_H
