/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC
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

#include "render/audioparams.h"

#include <cmath>

namespace olive::core
{

const std::vector<int> AudioParams::kSupportedSampleRates = {
	8000, // 8000 Hz
	11025, // 11025 Hz
	16000, // 16000 Hz
	22050, // 22050 Hz
	24000, // 24000 Hz
	32000, // 32000 Hz
	44100, // 44100 Hz
	48000, // 48000 Hz
	88200, // 88200 Hz
	96000 // 96000 Hz
};

const std::vector<uint64_t> AudioParams::kSupportedChannelLayouts = {
	kChannelLayoutMono, kChannelLayoutStereo, kChannelLayout2_1,
	kChannelLayout5Point1, kChannelLayout7Point1
};

bool AudioParams::operator==(const AudioParams &other) const
{
	return format() == other.format() && sample_rate() == other.sample_rate() &&
		   time_base() == other.time_base() &&
		   channel_layout_mask_ == other.channel_layout_mask_;
}

bool AudioParams::operator!=(const AudioParams &other) const
{
	return !(*this == other);
}

int64_t AudioParams::time_to_bytes(const double &time) const
{
	return time_to_bytes_per_channel(time) * channel_count();
}

int64_t AudioParams::time_to_bytes(const rational &time) const
{
	return time_to_bytes(time.toDouble());
}

int64_t AudioParams::time_to_bytes_per_channel(const double &time) const
{
	assert(is_valid());

	return int64_t(time_to_samples(time)) * bytes_per_sample_per_channel();
}

int64_t AudioParams::time_to_bytes_per_channel(const rational &time) const
{
	return time_to_bytes_per_channel(time.toDouble());
}

int64_t AudioParams::time_to_samples(const double &time) const
{
	assert(is_valid());

	return std::round(double(sample_rate()) * time);
}

int64_t AudioParams::time_to_samples(const rational &time) const
{
	return time_to_samples(time.toDouble());
}

int64_t AudioParams::samples_to_bytes(const int64_t &samples) const
{
	assert(is_valid());

	return samples_to_bytes_per_channel(samples) * channel_count();
}

int64_t AudioParams::samples_to_bytes_per_channel(const int64_t &samples) const
{
	assert(is_valid());

	return samples * bytes_per_sample_per_channel();
}

rational AudioParams::samples_to_time(const int64_t &samples) const
{
	return sample_rate_as_time_base() * samples;
}

int64_t AudioParams::bytes_to_samples(const int64_t &bytes) const
{
	assert(is_valid());

	return bytes / (channel_count() * bytes_per_sample_per_channel());
}

rational AudioParams::bytes_to_time(const int64_t &bytes) const
{
	return samples_to_time(bytes_to_samples(bytes));
}

rational AudioParams::bytes_per_channel_to_time(const int64_t &bytes) const
{
	return samples_to_time(bytes_to_samples(bytes * channel_count()));
}

int AudioParams::channel_count() const
{
	return channel_count_;
}

int AudioParams::bytes_per_sample_per_channel() const
{
	return format_.byte_count();
}

int AudioParams::bits_per_sample() const
{
	return bytes_per_sample_per_channel() * 8;
}

bool AudioParams::is_valid() const
{
	return (!time_base().isNull() && channel_layout_mask_ != 0 &&
			format_ > SampleFormat::INVALID && format_ < SampleFormat::COUNT);
}

void AudioParams::calculate_channel_count()
{
	channel_count_ = ChannelLayoutMaskChannelCount(channel_layout_mask_);
}

}
