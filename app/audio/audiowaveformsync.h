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

#ifndef AUDIOWAVEFORMSYNC_H
#define AUDIOWAVEFORMSYNC_H

#include <cstdint>

#include <QVector>

#include "olive/core/render/samplebuffer.h"

namespace olive
{

class AudioWaveformSync {
public:
	struct OffsetResult {
		int64_t offset_samples = 0;
		double confidence = 0.0;
		bool valid = false;
	};

	struct StretchOffsetResult {
		// Playback rate the candidate must be played at to align with the
		// reference (e.g. 2.0 = candidate runs at half speed and needs to be
		// sped up 2x)
		double rate = 1.0;
		int64_t offset_samples = 0;
		double confidence = 0.0;
		bool valid = false;
	};

	static QVector<double> ExtractRmsEnvelope(const core::SampleBuffer &samples,
											  size_t window_samples);

	static OffsetResult EstimateOffset(const core::SampleBuffer &reference,
									   const core::SampleBuffer &candidate,
									   size_t window_samples,
									   int64_t max_offset_samples);

	static OffsetResult EstimateEnvelopeOffset(const QVector<double> &reference,
											   const QVector<double> &candidate,
											   size_t window_samples,
											   int64_t max_offset_windows);

	/**
	 * @brief Offset estimation that ignores windows flagged as invalid
	 *
	 * @p reference_valid and @p candidate_valid mark which envelope windows
	 * contain real data (e.g. actually cached waveform regions). Windows
	 * flagged false on either side are excluded from the correlation instead
	 * of being treated as silence, which improves accuracy when parts of the
	 * waveform cache have not been generated yet. Empty masks are treated as
	 * "all windows valid".
	 */
	static OffsetResult EstimateEnvelopeOffset(const QVector<double> &reference,
											   const QVector<double> &candidate,
											   const QVector<bool> &reference_valid,
											   const QVector<bool> &candidate_valid,
											   size_t window_samples,
											   int64_t max_offset_windows);

	/**
	 * @brief Estimates a playback-rate change plus offset aligning the
	 * candidate to the reference
	 *
	 * The candidate envelope is resampled at each candidate rate in
	 * [min_rate, max_rate] (step rate_step) and correlated against the
	 * reference. rate > 1 means the candidate runs slower than the reference
	 * and must be sped up. The search is O(rates * lags * overlap), so
	 * callers should bound max_offset_windows to a sensible range.
	 */
	static StretchOffsetResult
	EstimateStretchAndOffset(const QVector<double> &reference,
							 const QVector<double> &candidate,
							 const QVector<bool> &reference_valid,
							 const QVector<bool> &candidate_valid, size_t window_samples,
							 int64_t max_offset_windows, double min_rate,
							 double max_rate, double rate_step);
};

}

#endif // AUDIOWAVEFORMSYNC_H
