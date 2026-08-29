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

#ifndef FFMPEGENCODER_H
#define FFMPEGENCODER_H

#include <ffmpeg_bridge/ffmpeg_bridge.h>

#include "codec/encoder.h"

namespace olive
{

/**
 * @brief An Encoder derivative that uses the ffmpeg_bridge library for encoding
 *
 * All encoding work happens inside the ffmpeg_bridge shared library through
 * its pure C API; this class only translates EncodingParams into a bridge
 * configuration and forwards calls.
 */
class FFmpegEncoder : public Encoder {
	Q_OBJECT
public:
	FFmpegEncoder(const EncodingParams &params);

	virtual QStringList
	GetPixelFormatsForCodec(ExportCodec::Codec c) const override;

	virtual std::vector<SampleFormat>
	GetSampleFormatsForCodec(ExportCodec::Codec c) const override;

	virtual bool Open() override;

	virtual bool WriteFrame(olive::FramePtr frame,
							olive::core::rational time) override;

	virtual bool WriteAudio(const olive::SampleBuffer &audio) override;

	bool WriteAudioData(const AudioParams &audio_params, const uint8_t **data,
						int input_sample_count);

	virtual bool WriteSubtitle(const SubtitleBlock *sub_block) override;

	virtual void Close() override;

	virtual PixelFormat GetDesiredPixelFormat() const override
	{
		return video_conversion_fmt_;
	}

private:
	/**
   * @brief Copy the last error message from the bridge into the encoder error state
   */
	void SetErrorFromBridge();

	static int ExportCodecToBridge(ExportCodec::Codec c);

	FBEncoder *encoder_;

	PixelFormat video_conversion_fmt_;

	bool open_;
};

}

#endif // FFMPEGENCODER_H
