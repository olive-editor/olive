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

#include "timelinewidgetwaveformsync.h"

#include <cmath>

#include "node/block/clip/clip.h"
#include "render/audiowaveformcache.h"

namespace olive
{

namespace TimelineWaveformSync
{

bool GetWaveformSyncClip(Block *block, WaveformSyncClip *out)
{
	ClipBlock *clip = dynamic_cast<ClipBlock *>(block);
	if (!clip || !clip->waveform()) {
		return false;
	}

	const TimeRange media_range = clip->media_range();
	if (media_range.length().isNull()) {
		return false;
	}

	const AudioWaveformCache *waveform = clip->waveform();
	if (waveform->GetParameters().sample_rate() <= 0) {
		return false;
	}

	// Allow waveform sync as long as at least some portion of the clip's
	// media range has been validated. Requiring the entire range to be
	// validated makes the menu item stay disabled for long clips and gives
	// the appearance that "nothing happens" when the user tries to sync.
	const TimeRangeList validated_ranges =
		waveform->GetValidatedRanges().Intersects(media_range);
	if (validated_ranges.isEmpty()) {
		return false;
	}

	out->clip = clip;
	out->waveform = waveform;
	out->media_range = media_range;
	out->sample_rate = waveform->GetParameters().sample_rate();
	return true;
}

QVector<WaveformSyncClip>
GetSelectedWaveformSyncClips(const QVector<Block *> &blocks)
{
	QVector<WaveformSyncClip> clips;
	for (Block *block : blocks) {
		WaveformSyncClip sync_clip;
		if (GetWaveformSyncClip(block, &sync_clip)) {
			clips.append(sync_clip);
		}
	}
	return clips;
}

QVector<double> ExtractWaveformCacheEnvelope(const WaveformSyncClip &clip,
											 int sample_rate,
											 size_t window_samples,
											 QVector<bool> *valid_mask)
{
	QVector<double> envelope;
	if (sample_rate <= 0 || !window_samples) {
		return envelope;
	}

	if (valid_mask) {
		valid_mask->clear();
	}

	const rational window_time(static_cast<int>(window_samples), sample_rate);

	// Only trust regions that have actually been validated. Unvalidated cache
	// returns zero samples, which both drags the correlation score down and
	// can produce false peaks if one clip happens to have more cached data
	// than another. Zero placeholders keep every envelope aligned to the same
	// absolute timeline, while the validity mask lets the correlation skip
	// those placeholders entirely.
	const TimeRangeList validated_ranges =
		clip.waveform->GetValidatedRanges().Intersects(clip.media_range);

	for (rational t = clip.media_range.in(); t < clip.media_range.out();
		 t += window_time) {
		const rational length = qMin(window_time, clip.media_range.out() - t);
		const TimeRange window(t, t + length);

		const bool window_valid = validated_ranges.contains(window);

		double peak = 0.0;
		if (window_valid) {
			const AudioVisualWaveform::Sample summary =
				clip.waveform->GetSummaryFromTime(t, length);

			for (const AudioVisualWaveform::SamplePerChannel &channel :
				 summary) {
				const double channel_peak =
					std::max(std::abs(static_cast<double>(channel.min)),
							 std::abs(static_cast<double>(channel.max)));
				peak = std::max(peak, channel_peak);
			}
		}

		envelope.append(peak);
		if (valid_mask) {
			valid_mask->append(window_valid);
		}
	}
	return envelope;
}

} // namespace TimelineWaveformSync

} // namespace olive
