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

#ifndef LIBOLIVECORE_AUDIOPARAMS_H
#define LIBOLIVECORE_AUDIOPARAMS_H
#include <cstring>

#include <assert.h>
#include <vector>

#include "channellayout.h"
#include "sampleformat.h"
#include "../util/rational.h"

namespace olive::core
{

/**
 * @brief Audio parameters class managing audio stream configuration
 *
 * Channel layouts are stored as plain 64-bit masks (see channellayout.h).
 * Because the mask is a simple value type, AudioParams has value semantics
 * and can be copied freely.
 */
class AudioParams {
public:
	/**
	 * @brief Default constructor creates invalid AudioParams
	 * sample_rate=0, channel_layout empty, format=INVALID
	 */
	AudioParams()
		: sample_rate_(0)
		, channel_layout_mask_(0)
		, channel_count_(0)
		, format_(SampleFormat::INVALID)
	{
		set_default_footage_parameters();
	}

	/**
	 * @brief Constructor from channel layout mask
	 * @param sample_rate Audio sample rate
	 * @param channel_layout Channel layout mask (e.g., kChannelLayoutStereo)
	 * @param format Sample format
	 */
	AudioParams(const int &sample_rate, uint64_t channel_layout,
				const SampleFormat &format)
		: sample_rate_(sample_rate)
		, channel_layout_mask_(channel_layout)
		, channel_count_(0)
		, format_(format)
	{
		set_default_footage_parameters();
		timebase_ = sample_rate_as_time_base();
		calculate_channel_count();
	}
	int sample_rate() const
	{
		return sample_rate_;
	}

	void set_sample_rate(int sample_rate)
	{
		sample_rate_ = sample_rate;
	}

	/**
	 * @brief Channel layout as a 64-bit mask (0 if unspecified)
	 */
	const uint64_t &channel_layout() const
	{
		return channel_layout_mask_;
	}

	/**
	 * @brief Set channel layout from mask
	 * @param mask Channel layout mask (e.g., kChannelLayoutStereo)
	 */
	void set_channel_layout(uint64_t mask)
	{
		channel_layout_mask_ = mask;
		calculate_channel_count();
	}
	rational time_base() const
	{
		return timebase_;
	}

	void set_time_base(const rational &timebase)
	{
		timebase_ = timebase;
	}

	rational sample_rate_as_time_base() const
	{
		return rational(1, sample_rate());
	}

	SampleFormat format() const
	{
		return format_;
	}

	void set_format(SampleFormat format)
	{
		format_ = format;
	}

	bool enabled() const
	{
		return enabled_;
	}

	void set_enabled(bool e)
	{
		enabled_ = e;
	}

	int stream_index() const
	{
		return stream_index_;
	}

	void set_stream_index(int s)
	{
		stream_index_ = s;
	}

	int64_t duration() const
	{
		return duration_;
	}

	void set_duration(int64_t duration)
	{
		duration_ = duration;
	}

	int64_t time_to_bytes(const double &time) const;
	int64_t time_to_bytes(const rational &time) const;
	int64_t time_to_bytes_per_channel(const double &time) const;
	int64_t time_to_bytes_per_channel(const rational &time) const;
	int64_t time_to_samples(const double &time) const;
	int64_t time_to_samples(const rational &time) const;
	int64_t samples_to_bytes(const int64_t &samples) const;
	int64_t samples_to_bytes_per_channel(const int64_t &samples) const;
	rational samples_to_time(const int64_t &samples) const;
	int64_t bytes_to_samples(const int64_t &bytes) const;
	rational bytes_to_time(const int64_t &bytes) const;
	rational bytes_per_channel_to_time(const int64_t &bytes) const;
	int channel_count() const;
	int bytes_per_sample_per_channel() const;
	int bits_per_sample() const;
	bool is_valid() const;

	bool operator==(const AudioParams &other) const;
	bool operator!=(const AudioParams &other) const;

	static const std::vector<uint64_t> kSupportedChannelLayouts;
	static const std::vector<int> kSupportedSampleRates;

private:
	void set_default_footage_parameters()
	{
		enabled_ = true;
		stream_index_ = 0;
		duration_ = 0;
	}

	/**
	 * @brief Updates channel_count_ from the current channel_layout_mask_
	 * Called after any channel layout modification.
	 */
	void calculate_channel_count();

	int sample_rate_;                    ///< Audio sample rate in Hz (e.g., 48000)

	/**
	 * @brief Channel layout mask (0 if unspecified)
	 *
	 * Plain value type mirroring FFmpeg's AV_CH_LAYOUT_* masks; no dynamic
	 * memory is involved, so copies are trivially safe.
	 */
	uint64_t channel_layout_mask_;

	int channel_count_;                  ///< Cached channel count from layout

	SampleFormat format_;                ///< Audio sample format

	// Footage-specific parameters (serialized with footage metadata)
	int enabled_; // Using int instead of bool fixes GCC 11 stringop-overflow issue (byte alignment)
	int stream_index_;                   ///< Index in the source file's stream list
	int64_t duration_;                   ///< Stream duration in timebase units
	rational timebase_;                  ///< Timebase for this audio stream
};

}

#endif // LIBOLIVECORE_AUDIOPARAMS_H
