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

#include "audioprocessor.h"

#include <cstring>

#include <QDebug>

#include "common/ffmpegutils.h"

namespace olive
{

/**
 * @brief Ensure an AudioParams has a usable channel layout mask.
 *
 * The bridge's abuffer/aformat filters reject a channel layout mask of 0
 * (e.g. when the user config or a source stream reports a mask of 0).
 * If the mask is zero, fall back to a default layout derived from the
 * channel count (stereo when unknown).
 */
static AudioParams FixChannelLayout(const AudioParams &params)
{
	AudioParams result = params;

	if (params.channel_layout() == 0) {
		int channels = params.channel_count();
		if (channels <= 0) {
			channels = 2;
		}

		qWarning() << "AudioProcessor: fixing unspecified channel layout"
				   << "(channels=" << params.channel_count() << ") -> default"
				   << channels << "channel layout";

		result.set_channel_layout(fb_channel_layout_default(channels));
	}

	return result;
}

AudioProcessor::AudioProcessor()
{
	graph_ = nullptr;
	out_frame_ = nullptr;
}

AudioProcessor::~AudioProcessor()
{
	Close();
}

bool AudioProcessor::Open(const AudioParams &from, const AudioParams &to,
						  double tempo)
{
	if (graph_) {
		qWarning() << "Tried to open a processor that was already open";
		return false;
	}

	AudioParams from_fixed = FixChannelLayout(from);
	AudioParams to_fixed = FixChannelLayout(to);

	qDebug() << "AudioProcessor::Open: from sample_rate="
			 << from_fixed.sample_rate() << "channels="
			 << from_fixed.channel_count() << "layout_mask=0x" << Qt::hex
			 << from_fixed.channel_layout() << "to sample_rate="
			 << to_fixed.sample_rate() << "channels=" << to_fixed.channel_count()
			 << "layout_mask=0x" << to_fixed.channel_layout() << Qt::dec;

	FBAudioGraphConfig config;
	memset(&config, 0, sizeof(config));
	config.in_sample_rate = from_fixed.sample_rate();
	config.in_channel_layout_mask = from_fixed.channel_layout();
	config.in_sample_format =
		FFmpegUtils::GetFFmpegSampleFormat(from_fixed.format());
	config.in_channels = from_fixed.channel_count();

	config.out_sample_rate = to_fixed.sample_rate();
	config.out_channel_layout_mask = to_fixed.channel_layout();
	config.out_sample_format =
		FFmpegUtils::GetFFmpegSampleFormat(to_fixed.format());
	config.out_channels = to_fixed.channel_count();
	config.out_is_planar = to_fixed.format().is_planar() ? 1 : 0;

	config.tempo = tempo;

	graph_ = fb_audio_graph_create(&config);
	if (!graph_) {
		qCritical() << "Failed to create audio filter graph";
		return false;
	}

	out_frame_ = fb_frame_alloc();
	if (!out_frame_) {
		qCritical() << "Failed to allocate output frame";
		Close();
		return false;
	}

	from_ = from_fixed;
	to_ = to_fixed;

	return true;
}

void AudioProcessor::Close()
{
	if (graph_) {
		fb_audio_graph_free(&graph_);
	}

	if (out_frame_) {
		fb_frame_free(&out_frame_);
	}
}

int AudioProcessor::Convert(float **in, int nb_in_samples,
							AudioProcessor::Buffer *output)
{
	if (!IsOpen()) {
		qCritical() << "Tried to convert on closed processor";
		return -1;
	}

	int r = 0;

	if (in && nb_in_samples) {
		r = fb_audio_graph_push(
			graph_, reinterpret_cast<const uint8_t *const *>(in),
			nb_in_samples);
		if (r < 0) {
			qCritical() << "Failed to add frame to buffersrc:" << r;
			return r;
		}
	}

	if (output) {
		int nb_channels = to_.channel_count();

		if (to_.format().is_packed()) {
			nb_channels = 1;
		}

		AudioProcessor::Buffer &result = *output;
		result.resize(nb_channels);

		int byte_offset = 0;

		while (true) {
			r = fb_audio_graph_pull(graph_, out_frame_);
			if (r <= 0) {
				if (r == 0) {
					// No more output available right now
					r = 0;
				} else {
					// Handle unexpected error
					qCritical() << "Failed to pull from buffersink:" << r;
				}
				break;
			}

			int nb_bytes = fb_frame_get_nb_samples(out_frame_) *
						   to_.bytes_per_sample_per_channel();
			if (to_.format().is_packed()) {
				nb_bytes *= to_.channel_count();
			}

			for (int i = 0; i < nb_channels; i++) {
				result[i].resize(byte_offset + nb_bytes);
				memcpy(result[i].data() + byte_offset,
					   fb_frame_get_data(out_frame_, i), nb_bytes);
			}
			byte_offset += nb_bytes;
		}
	}

	return r;
}

void AudioProcessor::Flush()
{
	int r = fb_audio_graph_push(graph_, nullptr, 0);
	if (r < 0) {
		qCritical() << "Failed to flush:" << r;
	}
}

}
