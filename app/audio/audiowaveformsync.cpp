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

#include "audiowaveformsync.h"

#include <algorithm>
#include <cmath>

namespace olive
{

QVector<double>
AudioWaveformSync::ExtractRmsEnvelope(const core::SampleBuffer &samples,
									  size_t window_samples)
{
	QVector<double> envelope;

	const int channel_count = samples.channel_count();
	const size_t sample_count = samples.sample_count();
	if (!channel_count || !sample_count || !window_samples) {
		return envelope;
	}

	const size_t window_count =
		(sample_count + window_samples - 1) / window_samples;
	envelope.resize(static_cast<int>(window_count));

	for (size_t window = 0; window < window_count; window++) {
		const size_t start = window * window_samples;
		const size_t end = std::min(start + window_samples, sample_count);
		double square_sum = 0.0;
		size_t total = 0;

		for (int channel = 0; channel < channel_count; channel++) {
			const float *data = samples.data(channel);
			for (size_t sample = start; sample < end; sample++) {
				const double value = data[sample];
				square_sum += value * value;
				total++;
			}
		}

		envelope[static_cast<int>(window)] =
			total ? std::sqrt(square_sum / static_cast<double>(total)) : 0.0;
	}

	return envelope;
}

AudioWaveformSync::OffsetResult AudioWaveformSync::EstimateOffset(
	const core::SampleBuffer &reference, const core::SampleBuffer &candidate,
	size_t window_samples, int64_t max_offset_samples)
{
	if (!window_samples) {
		return OffsetResult();
	}

	const QVector<double> reference_envelope =
		ExtractRmsEnvelope(reference, window_samples);
	const QVector<double> candidate_envelope =
		ExtractRmsEnvelope(candidate, window_samples);
	const int64_t max_offset_windows =
		max_offset_samples / static_cast<int64_t>(window_samples);

	return EstimateEnvelopeOffset(reference_envelope, candidate_envelope,
								  window_samples, max_offset_windows);
}

AudioWaveformSync::OffsetResult AudioWaveformSync::EstimateEnvelopeOffset(
	const QVector<double> &reference, const QVector<double> &candidate,
	size_t window_samples, int64_t max_offset_windows)
{
	return EstimateEnvelopeOffset(reference, candidate, QVector<bool>(),
								  QVector<bool>(), window_samples,
								  max_offset_windows);
}

AudioWaveformSync::OffsetResult AudioWaveformSync::EstimateEnvelopeOffset(
	const QVector<double> &reference, const QVector<double> &candidate,
	const QVector<bool> &reference_valid, const QVector<bool> &candidate_valid,
	size_t window_samples, int64_t max_offset_windows)
{
	OffsetResult result;
	if (reference.isEmpty() || candidate.isEmpty() || !window_samples) {
		return result;
	}

	const auto is_valid = [](const QVector<bool> &mask, int size, int index) {
		return mask.size() != size || mask.at(index);
	};

	double best_score = -2.0;
	int64_t best_lag = 0;

	for (int64_t lag = -max_offset_windows; lag <= max_offset_windows; lag++) {
		const int reference_start =
			static_cast<int>(std::max<int64_t>(0, -lag));
		const int candidate_start = static_cast<int>(std::max<int64_t>(0, lag));
		const int overlap = std::min(reference.size() - reference_start,
									 candidate.size() - candidate_start);

		if (overlap < 2) {
			continue;
		}

		// Only windows marked valid on both sides participate in the score
		double reference_mean = 0.0;
		double candidate_mean = 0.0;
		int valid_count = 0;
		for (int i = 0; i < overlap; i++) {
			const int reference_index = reference_start + i;
			const int candidate_index = candidate_start + i;
			if (!is_valid(reference_valid, reference.size(),
						  reference_index) ||
				!is_valid(candidate_valid, candidate.size(), candidate_index)) {
				continue;
			}
			reference_mean += reference.at(reference_index);
			candidate_mean += candidate.at(candidate_index);
			valid_count++;
		}

		if (valid_count < 2) {
			continue;
		}

		reference_mean /= static_cast<double>(valid_count);
		candidate_mean /= static_cast<double>(valid_count);

		double numerator = 0.0;
		double reference_energy = 0.0;
		double candidate_energy = 0.0;
		for (int i = 0; i < overlap; i++) {
			const int reference_index = reference_start + i;
			const int candidate_index = candidate_start + i;
			if (!is_valid(reference_valid, reference.size(),
						  reference_index) ||
				!is_valid(candidate_valid, candidate.size(), candidate_index)) {
				continue;
			}
			const double reference_value =
				reference.at(reference_index) - reference_mean;
			const double candidate_value =
				candidate.at(candidate_index) - candidate_mean;
			numerator += reference_value * candidate_value;
			reference_energy += reference_value * reference_value;
			candidate_energy += candidate_value * candidate_value;
		}

		if (qFuzzyIsNull(reference_energy) || qFuzzyIsNull(candidate_energy)) {
			continue;
		}

		const double score =
			numerator / std::sqrt(reference_energy * candidate_energy);
		if (score > best_score) {
			best_score = score;
			best_lag = lag;
		}
	}

	if (best_score > -2.0) {
		result.valid = true;
		result.confidence = std::max(0.0, best_score);
		result.offset_samples = best_lag * static_cast<int64_t>(window_samples);
	}

	return result;
}

AudioWaveformSync::StretchOffsetResult AudioWaveformSync::EstimateStretchAndOffset(
	const QVector<double> &reference, const QVector<double> &candidate,
	const QVector<bool> &reference_valid, const QVector<bool> &candidate_valid,
	size_t window_samples, int64_t max_offset_windows, double min_rate,
	double max_rate, double rate_step)
{
	StretchOffsetResult result;
	if (reference.isEmpty() || candidate.isEmpty() || !window_samples ||
		min_rate <= 0.0 || max_rate < min_rate || rate_step <= 0.0) {
		return result;
	}

	double best_confidence = -2.0;

	for (double rate = min_rate; rate <= max_rate + rate_step * 0.5;
		 rate += rate_step) {
		// Resample the candidate envelope so that window i of the resampled
		// envelope corresponds to window i*rate of the original
		const int resampled_size =
			static_cast<int>(candidate.size() / rate);
		if (resampled_size < 2) {
			continue;
		}

		QVector<double> resampled(resampled_size);
		QVector<bool> resampled_valid(resampled_size);
		for (int i = 0; i < resampled_size; i++) {
			const double position = i * rate;
			const int lower = static_cast<int>(position);
			const int upper =
				std::min(lower + 1, static_cast<int>(candidate.size()) - 1);
			const double fraction = position - lower;

			resampled[i] = candidate.at(lower) * (1.0 - fraction) +
						   candidate.at(upper) * fraction;

			resampled_valid[i] =
				(candidate_valid.size() != candidate.size() ||
				 (candidate_valid.at(lower) && candidate_valid.at(upper)));
		}

		const OffsetResult offset = EstimateEnvelopeOffset(
			reference, resampled, reference_valid, resampled_valid,
			window_samples, max_offset_windows);

		if (offset.valid && offset.confidence > best_confidence) {
			best_confidence = offset.confidence;
			result.valid = true;
			result.rate = rate;
			result.confidence = offset.confidence;
			result.offset_samples = offset.offset_samples;
		}
	}

	return result;
}

}
