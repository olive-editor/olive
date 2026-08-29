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

#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <inttypes.h>

#include <ffmpeg_bridge/ffmpeg_bridge.h>

#include <QTimer>
#include <QVector>
#include <QWaitCondition>

#include "codec/decoder.h"
#include "common/ffmpegutils.h"

namespace olive
{

/**
 * @brief A Decoder derivative that uses the ffmpeg_bridge library as an Olive decoder
 *
 * All media access goes through the pure C API of the ffmpeg_bridge shared
 * library; this class never sees an FFmpeg structure or function.
 */
class FFmpegDecoder : public Decoder {
	Q_OBJECT
public:
	// Constructor
	FFmpegDecoder();

	// Destructor
	DECODER_DEFAULT_DESTRUCTOR(FFmpegDecoder)

	virtual QString id() const override;

	virtual bool SupportsVideo() override
	{
		return true;
	}
	virtual bool SupportsAudio() override
	{
		return true;
	}

	virtual FootageDescription Probe(const QString &filename,
									 CancelAtom *cancelled) const override;

protected:
	virtual bool OpenInternal() override;
	virtual TexturePtr
	RetrieveVideoInternal(const RetrieveVideoParams &p) override;
	virtual FramePtr
	RetrieveVideoFrameInternal(const RetrieveVideoParams &p) override;
	virtual bool ConformAudioInternal(const QVector<QString> &filenames,
									  const AudioParams &params,
									  CancelAtom *cancelled) override;
	virtual void CloseInternal() override;

	virtual rational GetAudioStartOffset() const override;

private:
	/**
   * @brief Handle a bridge error code
   *
   * Uses the bridge API to retrieve a descriptive string for this error code and sends it to Error(). As such, this
   * function also automatically closes the Decoder.
   *
   * @param error_code
   */
	static QString FFmpegError(int error_code);

	void FreeScaler();

	AVFramePtr TransferHardwareFrame(AVFramePtr f);

	static PixelFormat GetNativePixelFormat(int pix_fmt);
	static int GetNativeChannelCount(int pix_fmt);

	static bool IsPixelFormatGLSLCompatible(int f);

	AVFramePtr GetFrameFromCache(const int64_t &t) const;

	void ClearFrameCache();

	AVFramePtr PreProcessFrame(AVFramePtr f, const RetrieveVideoParams &p);

	TexturePtr ProcessFrameIntoTexture(AVFramePtr f,
									   const RetrieveVideoParams &p,
									   const AVFramePtr original);

	AVFramePtr RetrieveFrame(const rational &time, CancelAtom *cancelled);

	void RemoveFirstFrame();

	static int MaximumQueueSize();

	FBScaler *scaler_;
	int scaler_src_width_;
	int scaler_src_height_;
	int scaler_src_format_;
	int scaler_dst_width_;
	int scaler_dst_height_;
	int scaler_dst_format_;
	int scaler_colrange_;
	int scaler_colspace_;

	FBPacket *working_packet_;

	int64_t second_ts_;

	std::list<AVFramePtr> cached_frames_;

	bool cache_at_zero_;
	bool cache_at_eof_;

	FBDecoder *instance_;

	// Stream parameters cached on open (the stream object itself lives
	// inside the bridge library)
	rational stream_time_base_;
	int64_t stream_start_time_;
	int64_t stream_duration_;
	int64_t format_start_time_;
	int input_sample_format_;
	int input_sample_rate_;
	uint64_t input_channel_layout_mask_;
};

}

#endif // FFMPEGDECODER_H
