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

#include "ffmpegdecoder.h"

#include <cstring>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtMath>
#include <QThread>

#include "codec/planarfiledevice.h"
#include "codec/timecodemetadata.h"
#include "common/ffmpegutils.h"
#include "common/filefunctions.h"
#include "render/renderer.h"
#include "render/subtitleparams.h"

namespace olive
{

static FramePtr CopyPackedAVFrameToFrame(const AVFramePtr &src,
										 PixelFormat format, int channel_count,
										 const rational &timestamp)
{
	if (!src || !src->data(0)) {
		return nullptr;
	}

	VideoParams params(src->width(), src->height(), format, channel_count);
	FramePtr frame = Frame::Create();
	frame->set_video_params(params);
	frame->set_timestamp(timestamp);
	if (!frame->allocate()) {
		return nullptr;
	}

	const int row_bytes = params.effective_width() *
						  VideoParams::GetBytesPerPixel(format, channel_count);
	for (int y = 0; y < frame->height(); y++) {
		memcpy(frame->data() + y * frame->linesize_bytes(),
			   src->data(0) + y * src->linesize(0), size_t(row_bytes));
	}

	return frame;
}

static VideoParams::Interlacing FFmpegFieldOrderToOlive(int fo)
{
	switch (fo) {
	case FB_FIELD_ORDER_TT:
		return VideoParams::kInterlacedTopFirst;
	case FB_FIELD_ORDER_BB:
		return VideoParams::kInterlacedBottomFirst;
	case FB_FIELD_ORDER_PROGRESSIVE:
	default:
		return VideoParams::kInterlaceNone;
	}
}

QVariant Yuv2RgbShader;
QVariant DeinterlaceShader;

namespace
{

int CancelThunk(void *userdata)
{
	CancelAtom *cancelled = static_cast<CancelAtom *>(userdata);
	return (cancelled && cancelled->IsCancelled()) ? 1 : 0;
}

TimecodeMetadata::SourceTime ExtractSourceStartTime(FBProbe *probe,
													int stream_index,
													const rational &timebase,
													int sample_rate)
{
	char buf[1024];

	if (fb_probe_get_metadata(probe, stream_index, "timecode", buf,
							  sizeof(buf)) == 1) {
		TimecodeMetadata::SourceTime parsed =
			TimecodeMetadata::FromTimecodeString(QString::fromUtf8(buf),
												 timebase);
		if (parsed.valid) {
			return parsed;
		}
	}

	if (fb_probe_get_metadata(probe, stream_index, "time_reference", buf,
							  sizeof(buf)) == 1) {
		TimecodeMetadata::SourceTime parsed =
			TimecodeMetadata::FromBwfTimeReference(QString::fromUtf8(buf),
												   sample_rate);
		if (parsed.valid) {
			return parsed;
		}
	}

	return TimecodeMetadata::SourceTime();
}

struct SubtitleReadContext {
	SubtitleParams *sub;
	rational time_base;
};

void SubtitleReadThunk(int64_t pts, int64_t duration, const char *text,
					   int text_size, void *userdata)
{
	SubtitleReadContext *ctx = static_cast<SubtitleReadContext *>(userdata);

	TimeRange time(Timecode::timestamp_to_time(pts, ctx->time_base),
				   Timecode::timestamp_to_time(pts + duration, ctx->time_base));

	ctx->sub->push_back(
		Subtitle(time, QString::fromUtf8(text, text_size)));
}

} // namespace

FFmpegDecoder::FFmpegDecoder()
	: scaler_(nullptr)
	, working_packet_(nullptr)
	, cache_at_zero_(false)
	, cache_at_eof_(false)
	, instance_(nullptr)
	, stream_start_time_(0)
	, stream_duration_(0)
	, format_start_time_(FB_NOPTS_VALUE)
	, input_sample_format_(FB_SAMPLE_FMT_NONE)
	, input_sample_rate_(0)
	, input_channel_layout_mask_(0)
{
}

bool FFmpegDecoder::OpenInternal()
{
	instance_ = fb_decoder_create();
	if (!instance_) {
		return false;
	}

	if (fb_decoder_open(instance_, stream().filename().toUtf8(),
						stream().stream()) == 0) {
		// Cache the stream parameters the decoder logic needs; the stream
		// object itself always lives inside the bridge library
		FBStreamInfo info;
		if (fb_decoder_get_stream_info(instance_, &info) != 0) {
			fb_decoder_free(&instance_);
			return false;
		}

		stream_time_base_ = rational(info.time_base_num, info.time_base_den);
		stream_start_time_ = info.start_time;
		stream_duration_ = info.duration;
		format_start_time_ = fb_decoder_get_format_start_time(instance_);
		input_sample_format_ = info.sample_format;
		input_sample_rate_ = info.sample_rate;
		input_channel_layout_mask_ = info.channel_layout_mask;

		// Store one second in the source's timebase
		second_ts_ = qRound64(stream_time_base_.flipped().toDouble());

		working_packet_ = fb_packet_alloc();
		return true;
	}

	fb_decoder_free(&instance_);
	return false;
}

TexturePtr FFmpegDecoder::ProcessFrameIntoTexture(AVFramePtr f,
												  const RetrieveVideoParams &p,
												  const AVFramePtr original)
{
	// Determine native format
	int ideal_fmt = FFmpegUtils::GetCompatibleBridgePixelFormat(f->format());
	PixelFormat native_fmt = GetNativePixelFormat(ideal_fmt);
	int native_channels = GetNativeChannelCount(ideal_fmt);

	// Determine pixel aspect ratio
	int sar_num, sar_den;
	rational pixel_aspect_ratio(1, 1);
	if (fb_decoder_guess_sample_aspect_ratio(instance_, nullptr, &sar_num,
											 &sar_den) == 0 &&
		sar_den != 0) {
		pixel_aspect_ratio = rational(sar_num, sar_den);
	}

	// Set up video params
	VideoParams vp(original->width(), original->height(), native_fmt,
				   native_channels, pixel_aspect_ratio,
				   VideoParams::kInterlaceNone, p.divider);

	// For YUV formats, force the output texture to F32 RGBA for maximum precision
	switch (f->format()) {
	case FB_PIX_FMT_YUV420P:
	case FB_PIX_FMT_YUV422P:
	case FB_PIX_FMT_YUV444P:
	case FB_PIX_FMT_YUV420P10LE:
	case FB_PIX_FMT_YUV422P10LE:
	case FB_PIX_FMT_YUV444P10LE:
	case FB_PIX_FMT_YUV420P12LE:
	case FB_PIX_FMT_YUV422P12LE:
	case FB_PIX_FMT_YUV444P12LE:
		vp.set_format(PixelFormat::F32);
		vp.set_channel_count(VideoParams::kRGBAChannelCount);
		break;
	default:
		break;
	}

	// Create texture
	TexturePtr tex = p.renderer->CreateTexture(vp);

	switch (f->format()) {
	case FB_PIX_FMT_YUV420P:
	case FB_PIX_FMT_YUV422P:
	case FB_PIX_FMT_YUV444P:
	case FB_PIX_FMT_YUV420P10LE:
	case FB_PIX_FMT_YUV422P10LE:
	case FB_PIX_FMT_YUV444P10LE:
	case FB_PIX_FMT_YUV420P12LE:
	case FB_PIX_FMT_YUV422P12LE:
	case FB_PIX_FMT_YUV444P12LE: {
		// Run through YUV to RGB shader
		if (Yuv2RgbShader.isNull()) {
			// Compile shader
			Yuv2RgbShader = p.renderer->CreateNativeShader(
				ShaderCode(FileFunctions::ReadFileAsString(
					QStringLiteral(":/shaders/yuv2rgb.frag"))));
			if (Yuv2RgbShader.isNull()) {
				return nullptr;
			}
		}

		int px_size;
		int bits_per_pixel;
		switch (f->format()) {
		case FB_PIX_FMT_YUV420P:
		case FB_PIX_FMT_YUV422P:
		case FB_PIX_FMT_YUV444P:
		default:
			px_size = 1;
			bits_per_pixel = 8;
			break;
		case FB_PIX_FMT_YUV420P10LE:
		case FB_PIX_FMT_YUV422P10LE:
		case FB_PIX_FMT_YUV444P10LE:
			px_size = 2;
			bits_per_pixel = 10;
			break;
		case FB_PIX_FMT_YUV420P12LE:
		case FB_PIX_FMT_YUV422P12LE:
		case FB_PIX_FMT_YUV444P12LE:
			px_size = 2;
			bits_per_pixel = 12;
			break;
		}

		AVFramePtr hw_in = f;

		VideoParams plane_params = vp;
		plane_params.set_channel_count(1);
		// YUV 平面是单通道原始数据：8bit 用 U8，10/12bit 用 U16
		plane_params.set_format(px_size == 1 ? PixelFormat::U8 : PixelFormat::U16);
	

		TexturePtr y_plane = p.renderer->CreateTexture(
			plane_params, hw_in->data(0), hw_in->linesize(0) / px_size);
		y_plane->handleFrame(hw_in);
		switch (f->format()) {
		case FB_PIX_FMT_YUV420P:
		case FB_PIX_FMT_YUV422P:
		case FB_PIX_FMT_YUV420P10LE:
		case FB_PIX_FMT_YUV422P10LE:
		case FB_PIX_FMT_YUV420P12LE:
		case FB_PIX_FMT_YUV422P12LE:
			plane_params.set_width(plane_params.width() / 2);
			break;
		}

		switch (f->format()) {
		case FB_PIX_FMT_YUV420P:
		case FB_PIX_FMT_YUV420P10LE:
		case FB_PIX_FMT_YUV420P12LE:
			plane_params.set_height(plane_params.height() / 2);
			break;
		}

		TexturePtr u_plane = p.renderer->CreateTexture(
			plane_params, hw_in->data(1), hw_in->linesize(1) / px_size);
		u_plane->handleFrame(hw_in);

		TexturePtr v_plane = p.renderer->CreateTexture(
			plane_params, hw_in->data(2), hw_in->linesize(2) / px_size);
		v_plane->handleFrame(hw_in);

		ShaderJob job;
		job.Insert(QStringLiteral("y_channel"),
				   NodeValue(NodeValue::kTexture,
							 QVariant::fromValue(y_plane)));
		job.Insert(QStringLiteral("u_channel"),
				   NodeValue(NodeValue::kTexture,
							 QVariant::fromValue(u_plane)));
		job.Insert(QStringLiteral("v_channel"),
				   NodeValue(NodeValue::kTexture,
							 QVariant::fromValue(v_plane)));
		job.Insert(QStringLiteral("bits_per_pixel"),
				   NodeValue(NodeValue::kInt, bits_per_pixel));
		job.Insert(QStringLiteral("full_range"),
				   NodeValue(NodeValue::kBoolean,
							 hw_in->color_range() == FB_COLOR_RANGE_JPEG));

		double yuv_coeffs[4];
		fb_get_yuv_coefficients(hw_in->colorspace(), yuv_coeffs);
		job.Insert(QStringLiteral("yuv_crv"),
				   NodeValue(NodeValue::kFloat, yuv_coeffs[0]));
		job.Insert(QStringLiteral("yuv_cgu"),
				   NodeValue(NodeValue::kFloat, yuv_coeffs[2]));
		job.Insert(QStringLiteral("yuv_cgv"),
				   NodeValue(NodeValue::kFloat, yuv_coeffs[3]));
		job.Insert(QStringLiteral("yuv_cbu"),
				   NodeValue(NodeValue::kFloat, yuv_coeffs[1]));

		tex = p.renderer->CreateTexture(vp);
		p.renderer->BlitToTexture(Yuv2RgbShader, job, tex.get(), false);
		break;
	}
	case FB_PIX_FMT_RGBA:
	case FB_PIX_FMT_RGBA64LE:
		// RGBA can be uploaded directly to the texture
		tex->handleFrame(f);
		tex->Upload(f->data(0), f->linesize(0) / vp.GetBytesPerPixel());
		break;
	case FB_PIX_FMT_RGBAF32LE:
		// RGBA F32 can be uploaded directly to the texture
		tex->handleFrame(f);
		tex->Upload(f->data(0), f->linesize(0) / vp.GetBytesPerPixel());
		break;
	}

	// Deinterlace if necessary
	if (p.src_interlacing != VideoParams::kInterlaceNone) {
		if (DeinterlaceShader.isNull()) {
			// Compile shader
			DeinterlaceShader = p.renderer->CreateNativeShader(
				ShaderCode(FileFunctions::ReadFileAsString(
					QStringLiteral(":/shaders/deinterlace2.frag"))));
			if (DeinterlaceShader.isNull()) {
				return nullptr;
			}
		}

		int fr_num, fr_den;
		rational frame_rate_tb;
		if (fb_decoder_guess_frame_rate(instance_, original->handle(), &fr_num,
										&fr_den) == 0 &&
			fr_num != 0) {
			frame_rate_tb = rational(fr_num, fr_den);
		}

		// Double frame rate for interlaced fields
		frame_rate_tb *= 2;

		// Flip frame rate so it can be used as a timebase
		frame_rate_tb.flip();

		int64_t req = Timecode::time_to_timestamp(
			p.time + rational(format_start_time_, FB_TIME_BASE), frame_rate_tb);
		int64_t frm = Timecode::rescale_timestamp(original->pts(),
												  stream_time_base_,
												  frame_rate_tb);

		bool first = (req == frm);
		bool top_first =
			(p.src_interlacing == VideoParams::kInterlacedTopFirst);

		int interlacing = (first == top_first) ? 1 : 2;

		TexturePtr deinterlaced = p.renderer->CreateTexture(tex->params());

		ShaderJob job;
		job.Insert(QStringLiteral("ove_maintex"),
				   NodeValue(NodeValue::kTexture, tex));
		job.Insert(QStringLiteral("interlacing"),
				   NodeValue(NodeValue::kInt, interlacing));
		job.Insert(QStringLiteral("pixel_height"),
				   NodeValue(NodeValue::kInt, original->height()));

		p.renderer->BlitToTexture(DeinterlaceShader, job, deinterlaced.get(),
								  false);

		tex = deinterlaced;
	}

	return tex;
}

TexturePtr FFmpegDecoder::RetrieveVideoInternal(const RetrieveVideoParams &p)
{
	if (AVFramePtr f = RetrieveFrame(p.time, p.cancelled)) {
		if (p.cancelled && p.cancelled->IsCancelled()) {
			return nullptr;
		}

		AVFramePtr original = f;

		// Disregard "JPEG" pixel formats because we allow the user to override that
		f->set_format(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(f->format()));

		// Force frame's color range to whatever it's set to in Olive
		f->set_color_range(p.force_range == VideoParams::kColorRangeFull ?
							   FB_COLOR_RANGE_JPEG :
							   FB_COLOR_RANGE_MPEG);

		// Perform any CPU processing required
		AVFramePtr ptr = PreProcessFrame(f, p);
		f = std::move(ptr);
		if (!f) {
			qWarning() << "PreProcessFrame failed";
			return nullptr;
		}

		// Finally, perform any GPU processing required
		TexturePtr texture = ProcessFrameIntoTexture(f, p, original);

		if (!texture) {
			qWarning() << "ProcessFrameIntoTexture returned null";
		}

		return texture;
	}

	return nullptr;
}

FramePtr FFmpegDecoder::RetrieveVideoFrameInternal(const RetrieveVideoParams &p)
{
	if (AVFramePtr f = RetrieveFrame(p.time, p.cancelled)) {
		if (p.cancelled && p.cancelled->IsCancelled()) {
			return nullptr;
		}

		f->set_format(FFmpegUtils::ConvertJPEGSpaceToRegularSpace(f->format()));
		f->set_color_range(p.force_range == VideoParams::kColorRangeFull ?
							   FB_COLOR_RANGE_JPEG :
							   FB_COLOR_RANGE_MPEG);

		AVFramePtr dest = CreateAVFramePtr();
		dest->set_width(f->width());
		dest->set_height(f->height());
		dest->set_format(p.maximum_format == PixelFormat::U8 ?
							 FB_PIX_FMT_RGBA :
							 FB_PIX_FMT_RGBA64LE);
		dest->set_color_range(f->color_range());
		dest->set_colorspace(f->colorspace());
		if (p.divider > 1) {
			dest->set_width(
				VideoParams::GetScaledDimension(dest->width(), p.divider));
			dest->set_height(
				VideoParams::GetScaledDimension(dest->height(), p.divider));
		}

		int r = dest->get_buffer(0);
		if (r < 0) {
			FFmpegError(r);
			return nullptr;
		}

		FBScaler *cpu_scaler = fb_scaler_create(f->width(), f->height(),
												f->format(), dest->width(),
												dest->height(), dest->format(),
												FB_SCALER_POINT);
		if (!cpu_scaler) {
			qCritical() << "Failed to create CPU frame conversion context";
			return nullptr;
		}

		fb_scaler_set_colorspace(cpu_scaler, dest->colorspace(),
								 dest->color_range() == FB_COLOR_RANGE_JPEG);

		r = fb_scaler_scale_frame(cpu_scaler, dest->handle(), f->handle());
		fb_scaler_free(&cpu_scaler);
		if (r < 0) {
			FFmpegError(r);
			return nullptr;
		}

		// sws_scale does not initialize the alpha channel when converting
		// from non-alpha source formats (e.g. YUV). fb_frame_get_buffer
		// zero-initializes the destination, leaving alpha at 0. The color
		// management shader later multiplies RGB by alpha, producing black.
		// Ensure alpha is opaque for source formats that have no alpha.
		if (!fb_pix_fmt_has_alpha(f->format())) {
			const int bpc = (dest->format() == FB_PIX_FMT_RGBA) ? 1 : 2;
			const int stride = dest->linesize(0);
			for (int y = 0; y < dest->height(); ++y) {
				uchar *row = dest->data(0) + y * stride;
				for (int x = 0; x < dest->width(); ++x) {
					if (bpc == 1) {
						row[x * 4 + 3] = 0xFF;
					} else {
						*reinterpret_cast<uint16_t *>(row + x * 8 + 6) = 0xFFFF;
					}
				}
			}
		}

		return CopyPackedAVFrameToFrame(dest,
										dest->format() == FB_PIX_FMT_RGBA ?
											PixelFormat::U8 :
											PixelFormat::U16,
										VideoParams::kRGBAChannelCount, p.time);
	}

	return nullptr;
}

void FFmpegDecoder::CloseInternal()
{
	if (working_packet_) {
		fb_packet_free(&working_packet_);
		working_packet_ = nullptr;
	}

	ClearFrameCache();
	FreeScaler();

	if (instance_) {
		fb_decoder_free(&instance_);
	}
}

rational FFmpegDecoder::GetAudioStartOffset() const
{
	if (instance_) {
		rational fmt_start = rational(format_start_time_, FB_TIME_BASE);
		rational str_start = stream_time_base_ * stream_start_time_;
		return str_start - fmt_start;
	} else {
		return 0;
	}
}

QString FFmpegDecoder::id() const
{
	return QStringLiteral("ffmpeg");
}

FootageDescription FFmpegDecoder::Probe(const QString &filename,
										CancelAtom *cancelled) const
{
	// Return value
	FootageDescription desc(id());

	// Variable for receiving errors from the bridge
	int error_code;

	// Convert QString to a C string
	QByteArray filename_c = filename.toUtf8();

	// Open file in the bridge library
	FBProbe *probe = fb_probe_create();
	error_code = fb_probe_open(probe, filename_c);

	// Handle open error
	if (error_code == 0) {
		int64_t footage_duration = fb_probe_get_duration(probe);
		TimecodeMetadata::SourceTime source_start_time = ExtractSourceStartTime(
			probe, -1, rational(1, FB_TIME_BASE), 0);

		bool duration_guessed_from_bitrate =
			fb_probe_duration_from_bitrate(probe) != 0;
		if (duration_guessed_from_bitrate) {
			qWarning()
				<< "Unreliable duration detected - we will manually determine it ourselves (this may take some time)";
		}

		// Dump it into the Footage object
		int video_streams = 0, audio_streams = 0, still_streams = 0;

		int stream_count = fb_probe_get_stream_count(probe);
		for (int i = 0; i < stream_count; i++) {
			FBStreamInfo info;
			if (fb_probe_get_stream_info(probe, i, &info) != 0) {
				continue;
			}

			rational stream_tb(info.time_base_num, info.time_base_den);

			if (!source_start_time.valid) {
				source_start_time = ExtractSourceStartTime(probe, i, stream_tb,
														   info.sample_rate);
			}

			// Only proceed if a decoder exists for this stream
			if (!info.has_decoder) {
				continue;
			}

			if (info.codec_type == FB_MEDIA_TYPE_VIDEO) {
				// Read at least two frames to get more information about this video stream
				VideoParams::Interlacing interlacing =
					VideoParams::kInterlaceNone;
				rational pixel_aspect_ratio(1, 1);
				rational frame_rate(info.avg_frame_rate_num,
									info.avg_frame_rate_den);
				int compatible_pix_fmt =
					FFmpegUtils::GetCompatibleBridgePixelFormat(info.pixel_format);
				bool image_is_still = false;
				int64_t stream_duration = info.duration;

				int decode_full_duration =
					(info.duration == FB_NOPTS_VALUE ||
					 duration_guessed_from_bitrate) ?
						1 :
						0;

				FBVideoStreamDetails details;
				if (fb_probe_video_stream_details(filename_c, i, &details,
												  decode_full_duration,
												  CancelThunk,
												  cancelled) == 0) {
					interlacing = FFmpegFieldOrderToOlive(details.field_order);
					if (details.pixel_aspect_den != 0) {
						pixel_aspect_ratio = rational(details.pixel_aspect_num,
													  details.pixel_aspect_den);
					}
					if (details.frame_rate_num != 0 &&
						details.frame_rate_den != 0) {
						frame_rate = rational(details.frame_rate_num,
											  details.frame_rate_den);
					}
					image_is_still = details.is_still != 0;
					if (details.decoded_duration != FB_NOPTS_VALUE) {
						stream_duration = details.decoded_duration;
					}
				}

				VideoParams stream;
				stream.set_stream_index(i);
				stream.set_width(info.width);
				stream.set_height(info.height);
				stream.set_video_type(image_is_still ?
										  VideoParams::kVideoTypeStill :
										  VideoParams::kVideoTypeVideo);
				stream.set_format(GetNativePixelFormat(compatible_pix_fmt));
				stream.set_channel_count(
					GetNativeChannelCount(compatible_pix_fmt));
				stream.set_interlacing(interlacing);
				stream.set_pixel_aspect_ratio(pixel_aspect_ratio);
				stream.set_frame_rate(frame_rate);
				stream.set_start_time(info.start_time);
				stream.set_time_base(stream_tb);
				stream.set_duration(stream_duration);
				stream.set_color_range(info.color_range == FB_COLOR_RANGE_JPEG ?
										   VideoParams::kColorRangeFull :
										   VideoParams::kColorRangeLimited);
				stream.set_premultiplied_alpha(false);

				desc.AddVideoStream(stream);
				image_is_still ? still_streams++ : video_streams++;

			} else if (info.codec_type == FB_MEDIA_TYPE_AUDIO) {
				int64_t stream_duration = info.duration;

				if (stream_duration == FB_NOPTS_VALUE ||
					duration_guessed_from_bitrate) {
					// Loop through stream until we get the whole duration
					if (footage_duration == FB_NOPTS_VALUE ||
						duration_guessed_from_bitrate) {
						int64_t decoded_duration = FB_NOPTS_VALUE;
						if (fb_probe_audio_stream_duration(
								filename_c, i, &decoded_duration, CancelThunk,
								cancelled) == 0) {
							stream_duration = decoded_duration;
						}
					} else {
						stream_duration = Timecode::rescale_timestamp_ceil(
							footage_duration, rational(1, FB_TIME_BASE),
							stream_tb);
					}
				}

				AudioParams stream;
				stream.set_stream_index(i);
				stream.set_channel_layout(info.channel_layout_mask);
				stream.set_sample_rate(info.sample_rate);
				stream.set_format(
					FFmpegUtils::GetNativeSampleFormat(info.sample_format));
				stream.set_time_base(stream_tb);
				stream.set_duration(stream_duration);
				desc.AddAudioStream(stream);

				audio_streams++;

			} else if (info.codec_type == FB_MEDIA_TYPE_SUBTITLE) {
				// The bridge limits this to SRT, matching our historical behavior
				SubtitleParams sub;
				SubtitleReadContext ctx = { &sub, stream_tb };

				if (fb_probe_read_subtitle_stream(filename_c, i,
												  SubtitleReadThunk,
												  &ctx) == 0) {
					desc.AddSubtitleStream(sub);
				}
			}
		}

		desc.SetStreamCount(stream_count);
		if (source_start_time.valid) {
			desc.SetSourceStartTime(source_start_time.time,
									source_start_time.source);
		}
		if (video_streams > 0 && still_streams > 0) {
			// 同时存在普通视频流和静态图片流（如 DJI 缩略图），只禁用静态图片流
			for (VideoParams &vp : desc.GetVideoStreams()) {
				if (vp.video_type() == VideoParams::kVideoTypeStill) {
					vp.set_enabled(false);
				}
			}
		} else if (video_streams == 0 && audio_streams > 0 && still_streams > 0) {
			// This footage has no video streams, but has audio and image streams. We've probably
			// imported a song with embedded album art that most people don't care about. We'll keep the
			// stills referenced in case users do, but we'll default them to disabled so they're
			// easier to work with.
			for (VideoParams &vp : desc.GetVideoStreams()) {
				vp.set_enabled(false);
			}
		}
	}

	// Free all memory
	fb_probe_free(&probe);

	return desc;
}

QString FFmpegDecoder::FFmpegError(int error_code)
{
	char err[1024];
	fb_error_string(error_code, err, 512);
	return QStringLiteral("%1 %2").arg(QString::number(error_code), err);
}

bool FFmpegDecoder::ConformAudioInternal(const QVector<QString> &filenames,
										 const AudioParams &params,
										 CancelAtom *cancelled)
{
	// Iterate through each audio frame and extract the PCM data

	// Seek to starting point
	fb_decoder_seek(instance_, 0);

	// The channel layout was validated by the bridge when the stream info was read
	if (!input_channel_layout_mask_) {
		qCritical()
			<< "Failed to determine channel layout of audio file, could not conform";
		return false;
	}

	// Create resampler
	FBResampler *resampler = fb_resampler_create(
		params.channel_layout(),
		FFmpegUtils::GetFFmpegSampleFormat(params.format()),
		params.sample_rate(), input_channel_layout_mask_, input_sample_format_,
		input_sample_rate_);
	if (!resampler) {
		qCritical() << "Failed to create resampler, could not conform";
		return false;
	}

	FBPacket *pkt = fb_packet_alloc();
	FBFrame *frame = fb_frame_alloc();
	int ret;

	bool success = false;

	int64_t duration = stream_duration_;
	if (duration == 0 || duration == FB_NOPTS_VALUE) {
		duration = fb_decoder_get_format_duration(instance_);
		if (!(duration == 0 || duration == FB_NOPTS_VALUE)) {
			// Rescale from format timebase to stream timebase
			duration = Timecode::rescale_timestamp_ceil(
				duration, rational(1, FB_TIME_BASE), stream_time_base_);
		}
	}

	PlanarFileDevice wave_out;
	if (wave_out.open(filenames, QFile::WriteOnly)) {
		int nb_channels = params.channel_count();
		SampleBuffer data;
		data.set_audio_params(params);

		while (true) {
			// Check if we have a `cancelled` ptr and its value
			if (cancelled && cancelled->IsCancelled()) {
				break;
			}

			ret = fb_decoder_get_frame(instance_, pkt, frame);

			if (ret < 0) {
				if (ret == FB_ERROR_EOF) {
					success = true;
				} else {
					char err_str[512];
					fb_error_string(ret, err_str, 512);
					qWarning() << "Failed to conform:" << ret << err_str;
				}
				break;
			}

			// Allocate buffers
			int nb_samples =
				fb_resampler_get_out_samples(resampler,
											 fb_frame_get_nb_samples(frame));
			int nb_bytes_per_channel =
				params.samples_to_bytes(nb_samples) / nb_channels;
			data.set_sample_count(nb_bytes_per_channel);
			data.allocate();

			// Resample audio to our destination parameters
			nb_samples = fb_resampler_convert_frame(
				resampler,
				reinterpret_cast<uint8_t **>(data.to_raw_ptrs().data()),
				nb_samples, frame);

			// If no error, write to files
			if (nb_samples > 0) {
				// Update byte count for the number of samples we actually received
				nb_bytes_per_channel =
					params.samples_to_bytes(nb_samples) / nb_channels;

				// Write to files
				wave_out.write(
					const_cast<const char **>(
						reinterpret_cast<char **>(data.to_raw_ptrs().data())),
					nb_bytes_per_channel);
			}

			// Free buffer
			data.destroy();

			// Handle error now after freeing
			if (nb_samples < 0) {
				char err_str[512];
				fb_error_string(nb_samples, err_str, 512);
				qWarning() << "libswresample failed with error:" << nb_samples
						   << err_str;
				break;
			}

			SignalProcessingProgress(fb_frame_get_best_effort_timestamp(frame),
									 duration);
		}

		wave_out.close();
	} else {
		qWarning() << "Failed to open WAVE output for indexing";
	}

	fb_resampler_free(&resampler);

	fb_frame_free(&frame);
	fb_packet_free(&pkt);

	return success;
}

PixelFormat FFmpegDecoder::GetNativePixelFormat(int pix_fmt)
{
	switch (pix_fmt) {
	case FB_PIX_FMT_RGB24:
	case FB_PIX_FMT_RGBA:
		return PixelFormat::U8;
	case FB_PIX_FMT_RGB48LE:
	case FB_PIX_FMT_RGBA64LE:
		return PixelFormat::U16;
	case FB_PIX_FMT_RGBF32LE:
	case FB_PIX_FMT_RGBAF32LE:
		return PixelFormat::F32;
	default:
		return PixelFormat::INVALID;
	}
}

int FFmpegDecoder::GetNativeChannelCount(int pix_fmt)
{
	switch (pix_fmt) {
	case FB_PIX_FMT_RGB24:
	case FB_PIX_FMT_RGB48LE:
	case FB_PIX_FMT_RGBF32LE:
		return VideoParams::kRGBChannelCount;
	case FB_PIX_FMT_RGBA:
	case FB_PIX_FMT_RGBA64LE:
	case FB_PIX_FMT_RGBAF32LE:
		return VideoParams::kRGBAChannelCount;
	default:
		return 0;
	}
}

bool FFmpegDecoder::IsPixelFormatGLSLCompatible(int f)
{
	// NOTE: We don't include RGB24 or RGB48 here because those are slow on the GPU and performance
	//       should be better if we convert to RGBA on the CPU beforehand
	switch (f) {
	case FB_PIX_FMT_YUV420P:
	case FB_PIX_FMT_YUV422P:
	case FB_PIX_FMT_YUV444P:
	case FB_PIX_FMT_YUV420P10LE:
	case FB_PIX_FMT_YUV422P10LE:
	case FB_PIX_FMT_YUV444P10LE:
	case FB_PIX_FMT_YUV420P12LE:
	case FB_PIX_FMT_YUV422P12LE:
	case FB_PIX_FMT_YUV444P12LE:
	case FB_PIX_FMT_RGBA:
	case FB_PIX_FMT_RGBA64LE:
	case FB_PIX_FMT_RGBAF32LE:
		return true;
	default:
		return false;
	}
}

void FFmpegDecoder::ClearFrameCache()
{
	if (!cached_frames_.empty()) {
		cached_frames_.clear();
		cache_at_eof_ = false;
		cache_at_zero_ = false;
	}
}

AVFramePtr FFmpegDecoder::PreProcessFrame(AVFramePtr f,
										  const RetrieveVideoParams &p)
{
	// In pre-processing, we try to achieve the following:
	//   - If a divider is being used, scale down the image
	//   - If a pixel format is not compatible with the GLSL shader, convert it to RGBA ourselves

	if (p.divider == 1 && IsPixelFormatGLSLCompatible(f->format())) {
		// No CPU processing required, the user wants this in full resolution and the pixel format can
		// be converted on the GPU
		return f;
	}

	// Some scaling and/or format conversion needs to be done
	AVFramePtr dest = CreateAVFramePtr();

	dest->set_width(f->width());
	dest->set_height(f->height());
	dest->set_format(f->format());
	dest->set_color_range(f->color_range());
	dest->set_colorspace(f->colorspace());
	if (p.divider > 1) {
		dest->set_width(
			VideoParams::GetScaledDimension(dest->width(), p.divider));
		dest->set_height(
			VideoParams::GetScaledDimension(dest->height(), p.divider));
	}

	if (!IsPixelFormatGLSLCompatible(dest->format())) {
		dest->set_format(FFmpegUtils::GetCompatibleBridgePixelFormat(dest->format(),
															   p.maximum_format));
	}

	// swscale does not support RGBAF32 as output, fallback to RGBA64
	if (dest->format() == FB_PIX_FMT_RGBAF32LE) {
		dest->set_format(FB_PIX_FMT_RGBA64LE);
	}

	int r = dest->get_buffer(0);
	if (r < 0) {
		FFmpegError(r);
		return nullptr;
	}

	if (!scaler_ || scaler_src_width_ != f->width() ||
		scaler_src_height_ != f->height() ||
		scaler_src_format_ != f->format() ||
		scaler_dst_width_ != dest->width() ||
		scaler_dst_height_ != dest->height() ||
		scaler_dst_format_ != dest->format() ||
		scaler_colrange_ != dest->color_range() ||
		scaler_colspace_ != dest->colorspace()) {
		// Scaler must be recreated, destroy current if it exists
		FreeScaler();

		// Cache info
		scaler_src_width_ = f->width();
		scaler_src_height_ = f->height();
		scaler_src_format_ = f->format();
		scaler_dst_width_ = dest->width();
		scaler_dst_height_ = dest->height();
		scaler_dst_format_ = dest->format();
		scaler_colrange_ = dest->color_range();
		scaler_colspace_ = dest->colorspace();

		// Create new scaler
		scaler_ = fb_scaler_create(scaler_src_width_, scaler_src_height_,
								   scaler_src_format_, scaler_dst_width_,
								   scaler_dst_height_, scaler_dst_format_,
								   FB_SCALER_POINT);

		// Set the scaler's colorspace details
		fb_scaler_set_colorspace(
			scaler_, scaler_colspace_,
			scaler_colrange_ == FB_COLOR_RANGE_JPEG);
	}

	r = fb_scaler_scale_frame(scaler_, dest->handle(), f->handle());

	if (r < 0) {
		FFmpegError(r);
		return nullptr;
	}

	return dest;
}

AVFramePtr FFmpegDecoder::RetrieveFrame(const rational &time,
										CancelAtom *cancelled)
{
	int64_t target_ts = Timecode::time_to_timestamp(time, stream_time_base_);

	if (format_start_time_ != FB_NOPTS_VALUE) {
		target_ts += Timecode::rescale_timestamp(format_start_time_,
												 rational(1, FB_TIME_BASE),
												 stream_time_base_);
	}

	const int64_t min_seek = 0;
	int64_t seek_ts = std::max(min_seek, target_ts - MaximumQueueSize());
	bool still_seeking = false;

	if (time != kAnyTimecode) {
		// If the frame wasn't in the frame cache, see if this frame cache is too old to use
		if (cached_frames_.empty() ||
			(target_ts < cached_frames_.front()->pts() ||
			 target_ts > cached_frames_.back()->pts() + 2 * second_ts_)) {
			ClearFrameCache();

			fb_decoder_seek(instance_, seek_ts);
			if (seek_ts == min_seek) {
				cache_at_zero_ = true;
			}

			still_seeking = true;
		} else {
			// Search cache for frame
			AVFramePtr cached_frame = GetFrameFromCache(target_ts);
			if (cached_frame) {
				return cached_frame;
			}
		}
	}

	int ret;
	AVFramePtr return_frame = nullptr;
	AVFramePtr filtered = nullptr;
	bool retried_after_eof = false;

	while (true) {
		// Break out of loop if we've cancelled
		if (cancelled && cancelled->IsCancelled()) {
			break;
		}

		if (!filtered) {
			filtered = CreateAVFramePtr();
		}

		// Pull from the decoder
		ret = fb_decoder_get_frame(instance_, working_packet_,
								   filtered->handle());

		if (cancelled && cancelled->IsCancelled()) {
			break;
		}

		// Handle any errors that aren't EOF (EOF is handled later on)
		if (ret < 0 && ret != FB_ERROR_EOF) {
			qCritical() << "Failed to retrieve frame:" << ret;
			break;
		}

		if (still_seeking) {
			// Handle a failure to seek (occurs on some media)
			// We'll only be here if the frame cache was emptied earlier
			if (!cache_at_zero_ &&
				(ret == FB_ERROR_EOF ||
				 filtered->best_effort_timestamp() > target_ts)) {
				seek_ts = qMax(min_seek, seek_ts - second_ts_);
				fb_decoder_seek(instance_, seek_ts);
				if (seek_ts == min_seek) {
					cache_at_zero_ = true;
				}
				continue;

			} else {
				still_seeking = false;
			}
		}

		if (ret == FB_ERROR_EOF) {
			// Handle an "expected" EOF by using the last frame of our cache
			cache_at_eof_ = true;

			if (cached_frames_.empty()) {
				if (!retried_after_eof) {
					retried_after_eof = true;
					ClearFrameCache();
					fb_decoder_seek(instance_, min_seek);
					cache_at_zero_ = true;
					still_seeking = true;
					continue;
				}

				qCritical()
					<< "Unexpected codec EOF - unable to retrieve frame";
			} else {
				return_frame = cached_frames_.back();
			}

			break;

		} else {
			// Cut down to thread count - 1 before we acquire a new frame
			if (cached_frames_.size() > size_t(MaximumQueueSize())) {
				RemoveFirstFrame();
			}

			// Store frame before just in case
			AVFramePtr previous;
			if (cached_frames_.empty()) {
				previous = nullptr;
			} else {
				previous = cached_frames_.back();
			}

			// Transfer hardware decoded frames to system memory before caching.
			filtered = TransferHardwareFrame(filtered);

			// Append this frame and signal to other threads that a new frame has arrived
			cached_frames_.push_back(filtered);

			// If this is a valid frame, see if this or the frame before it are the one we need
			if (filtered->pts() == target_ts || time == kAnyTimecode) {
				return_frame = filtered;
				break;
			} else if (filtered->pts() > target_ts) {
				if (!previous && cache_at_zero_) {
					return_frame = filtered;
					break;
				} else {
					return_frame = previous;
					break;
				}
			}
		}

		filtered = nullptr;
	}

	fb_packet_unref(working_packet_);

	return return_frame;
}

AVFramePtr FFmpegDecoder::TransferHardwareFrame(AVFramePtr f)
{
	if (!fb_decoder_hwaccel_enabled(instance_) ||
		!fb_frame_is_hw(f->handle())) {
		return f;
	}

	FBFrame *sw_frame = fb_frame_alloc();
	if (!sw_frame) {
		qCritical()
			<< "Failed to allocate software frame for hardware transfer";
		return nullptr;
	}

	int ret = fb_frame_hw_transfer_data(sw_frame, f->handle());
	if (ret < 0) {
		qWarning() << "Failed to transfer hardware frame to system memory:"
				   << FFmpegError(ret);
		fb_frame_free(&sw_frame);
		return nullptr;
	}

	ret = fb_frame_copy_props(sw_frame, f->handle());
	if (ret < 0) {
		qWarning()
			<< "Failed to copy frame properties during hardware transfer:"
			<< FFmpegError(ret);
	}

	return CreateAVFramePtr(sw_frame);
}

void FFmpegDecoder::FreeScaler()
{
	if (scaler_) {
		fb_scaler_free(&scaler_);
	}
}

AVFramePtr FFmpegDecoder::GetFrameFromCache(const int64_t &t) const
{
	if (t < cached_frames_.front()->pts()) {
		if (cache_at_zero_) {
			return cached_frames_.front();
		}

	} else if (t > cached_frames_.back()->pts()) {
		if (cache_at_eof_) {
			return cached_frames_.back();
		}

	} else {
		// We already have this frame in the cache, find it
		for (auto it = cached_frames_.cbegin(); it != cached_frames_.cend();
			 it++) {
			AVFramePtr this_frame = *it;

			auto next = it;
			next++;

			if (this_frame->pts() == t // Test for an exact match
				||
				(next != cached_frames_.cend() &&
				 (*next)->pts() > t)) { // Or for this frame to be the "closest"

				return this_frame;
			}
		}
	}

	return nullptr;
}

void FFmpegDecoder::RemoveFirstFrame()
{
	cached_frames_.pop_front();
	cache_at_zero_ = false;
}

int FFmpegDecoder::MaximumQueueSize()
{
	// Fairly arbitrary size. This used to need to be the number of current threads to ensure any
	// thread that arrived would have its frame available, but if we only have one render thread,
	// that's no longer a concern. Now, this value could technically be 1, but some memory cache
	// may be useful for reversing. This value may be tweaked over time.
	return 2;
}

}
