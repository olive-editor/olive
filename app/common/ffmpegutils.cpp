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

#include "common/ffmpegutils.h"

namespace olive
{

int FFmpegUtils::GetCompatibleBridgePixelFormat(int pix_fmt,
												PixelFormat maximum)
{
	int possible_pix_fmts[4];

	possible_pix_fmts[0] = FB_PIX_FMT_RGBA;

	if (maximum == PixelFormat::U8) {
		possible_pix_fmts[1] = FB_PIX_FMT_NONE;
	} else {
		possible_pix_fmts[1] = FB_PIX_FMT_RGBA64LE;
		if (maximum == PixelFormat::F32) {
			possible_pix_fmts[2] = FB_PIX_FMT_RGBAF32LE;
			possible_pix_fmts[3] = FB_PIX_FMT_NONE;
		} else {
			possible_pix_fmts[2] = FB_PIX_FMT_NONE;
		}
	}

	return fb_find_best_pix_fmt_of_list(possible_pix_fmts, pix_fmt);
}

SampleFormat FFmpegUtils::GetNativeSampleFormat(int smp_fmt)
{
	switch (smp_fmt) {
	case FB_SAMPLE_FMT_U8:
		return SampleFormat::U8;
	case FB_SAMPLE_FMT_S16:
		return SampleFormat::S16;
	case FB_SAMPLE_FMT_S32:
		return SampleFormat::S32;
	case FB_SAMPLE_FMT_S64:
		return SampleFormat::S64;
	case FB_SAMPLE_FMT_FLT:
		return SampleFormat::F32;
	case FB_SAMPLE_FMT_DBL:
		return SampleFormat::F64;
	case FB_SAMPLE_FMT_U8P:
		return SampleFormat::U8P;
	case FB_SAMPLE_FMT_S16P:
		return SampleFormat::S16P;
	case FB_SAMPLE_FMT_S32P:
		return SampleFormat::S32P;
	case FB_SAMPLE_FMT_S64P:
		return SampleFormat::S64P;
	case FB_SAMPLE_FMT_FLTP:
		return SampleFormat::F32P;
	case FB_SAMPLE_FMT_DBLP:
		return SampleFormat::F64P;
	default:
		break;
	}

	return SampleFormat::INVALID;
}

int FFmpegUtils::GetFFmpegSampleFormat(const SampleFormat &smp_fmt)
{
	switch (smp_fmt) {
	case SampleFormat::U8:
		return FB_SAMPLE_FMT_U8;
	case SampleFormat::S16:
		return FB_SAMPLE_FMT_S16;
	case SampleFormat::S32:
		return FB_SAMPLE_FMT_S32;
	case SampleFormat::S64:
		return FB_SAMPLE_FMT_S64;
	case SampleFormat::F32:
		return FB_SAMPLE_FMT_FLT;
	case SampleFormat::F64:
		return FB_SAMPLE_FMT_DBL;
	case SampleFormat::U8P:
		return FB_SAMPLE_FMT_U8P;
	case SampleFormat::S16P:
		return FB_SAMPLE_FMT_S16P;
	case SampleFormat::S32P:
		return FB_SAMPLE_FMT_S32P;
	case SampleFormat::S64P:
		return FB_SAMPLE_FMT_S64P;
	case SampleFormat::F32P:
		return FB_SAMPLE_FMT_FLTP;
	case SampleFormat::F64P:
		return FB_SAMPLE_FMT_DBLP;
	case SampleFormat::INVALID:
	case SampleFormat::COUNT:
		break;
	}

	return FB_SAMPLE_FMT_NONE;
}

int FFmpegUtils::ConvertJPEGSpaceToRegularSpace(int f)
{
	switch (f) {
	case FB_PIX_FMT_YUVJ420P:
		return FB_PIX_FMT_YUV420P;
	case FB_PIX_FMT_YUVJ422P:
		return FB_PIX_FMT_YUV422P;
	case FB_PIX_FMT_YUVJ444P:
		return FB_PIX_FMT_YUV444P;
	case FB_PIX_FMT_YUVJ440P:
		return FB_PIX_FMT_YUV440P;
	case FB_PIX_FMT_YUVJ411P:
		return FB_PIX_FMT_YUV411P;
	default:
		break;
	}

	return f;
}

int FFmpegUtils::GetFFmpegPixelFormat(const PixelFormat &pix_fmt,
									  int channel_layout)
{
	if (channel_layout == VideoParams::kRGBChannelCount) {
		switch (pix_fmt) {
		case PixelFormat::U8:
			return FB_PIX_FMT_RGB24;
		case PixelFormat::U10:
			return FB_PIX_FMT_NONE;
		case PixelFormat::U16:
			return FB_PIX_FMT_RGB48LE;
		case PixelFormat::F16:
			return FB_PIX_FMT_RGBF16LE;
		case PixelFormat::F32:
			return FB_PIX_FMT_RGBF32LE;
		case PixelFormat::INVALID:
		case PixelFormat::COUNT:
			break;
		}
	} else if (channel_layout == VideoParams::kRGBAChannelCount) {
		switch (pix_fmt) {
		case PixelFormat::U8:
			return FB_PIX_FMT_RGBA;
		case PixelFormat::U10:
			return FB_PIX_FMT_NONE;
		case PixelFormat::U16:
			return FB_PIX_FMT_RGBA64LE;
		case PixelFormat::F16:
			return FB_PIX_FMT_RGBAF16LE;
		case PixelFormat::F32:
			return FB_PIX_FMT_RGBAF32LE;
		case PixelFormat::INVALID:
		case PixelFormat::COUNT:
			break;
		}
	}

	return FB_PIX_FMT_NONE;
}

PixelFormat FFmpegUtils::GetCompatiblePixelFormat(const PixelFormat &pix_fmt)
{
	switch (pix_fmt) {
	case PixelFormat::U8:
		return PixelFormat::U8;
	case PixelFormat::U10:
		return PixelFormat::U8;
	case PixelFormat::U16:
	case PixelFormat::F16:
	case PixelFormat::F32:
		return PixelFormat::U16;
	case PixelFormat::INVALID:
	case PixelFormat::COUNT:
		break;
	}

	return PixelFormat::INVALID;
}

}
