/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

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

#include "audiolevelmeter.h"

#include <algorithm>
#include <cmath>

#include "common/decibel.h"

namespace olive
{

AudioLevelMeter::Stats
AudioLevelMeter::AnalyzeSampleBuffer(const core::SampleBuffer &samples)
{
	Stats stats;

	const int channel_count = samples.channel_count();
	const size_t sample_count = samples.sample_count();
	stats.channels.resize(channel_count);

	if (!channel_count || !sample_count) {
		return stats;
	}

	double total_square = 0.0;
	size_t total_samples = 0;

	for (int channel = 0; channel < channel_count; channel++) {
		const float *channel_data = samples.data(channel);
		double peak = 0.0;
		double square_sum = 0.0;

		for (size_t sample = 0; sample < sample_count; sample++) {
			const double value = channel_data[sample];
			const double abs_value = std::abs(value);

			peak = std::max(peak, abs_value);
			square_sum += value * value;
		}

		const double mean_square =
			square_sum / static_cast<double>(sample_count);
		const double rms = std::sqrt(mean_square);

		ChannelStats channel_stats;
		channel_stats.peak_linear = peak;
		channel_stats.peak_db = LinearToDb(peak);
		channel_stats.rms_linear = rms;
		channel_stats.rms_db = LinearToDb(rms);
		channel_stats.vu_db = channel_stats.rms_db;
		stats.channels[channel] = channel_stats;

		stats.max_peak_linear = std::max(stats.max_peak_linear, peak);
		total_square += square_sum;
		total_samples += sample_count;
	}

	stats.silence = qFuzzyIsNull(stats.max_peak_linear);
	stats.integrated_lufs =
		PowerToLufs(total_square / static_cast<double>(total_samples));

	return stats;
}

double AudioLevelMeter::LinearToDb(double linear)
{
	if (linear <= 0.0) {
		return Decibel::MINIMUM;
	}

	return Decibel::fromLinear(linear);
}

double AudioLevelMeter::PowerToLufs(double mean_square)
{
	if (mean_square <= 0.0) {
		return Decibel::MINIMUM;
	}

	// BS.1770 loudness uses K-weighted mean square. This first pass stores the
	// compatible unit and can be extended with K-weighting without changing UI.
	return -0.691 + 10.0 * std::log10(mean_square);
}

}
