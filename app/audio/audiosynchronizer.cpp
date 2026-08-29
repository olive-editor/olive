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

#include "audiosynchronizer.h"

namespace olive
{

AudioSynchronizer::Placement AudioSynchronizer::PlaceBySourceTime(
	const SourceClip &reference, const SourceClip &candidate,
	const core::rational &reference_timeline_in)
{
	Placement placement;
	if (!reference.has_source_start_time || !candidate.has_source_start_time ||
		reference.source_start_time.isNaN() ||
		candidate.source_start_time.isNaN()) {
		return placement;
	}

	const core::rational reference_head_source =
		reference.source_start_time + reference.media_in;
	const core::rational candidate_head_source =
		candidate.source_start_time + candidate.media_in;

	placement.timeline_in =
		reference_timeline_in + candidate_head_source - reference_head_source;
	placement.valid = !placement.timeline_in.isNaN();
	return placement;
}

AudioSynchronizer::Placement AudioSynchronizer::PlaceByWaveformOffset(
	const core::rational &reference_timeline_in,
	int64_t candidate_offset_samples, int sample_rate)
{
	Placement placement;
	if (sample_rate <= 0) {
		return placement;
	}

	placement.timeline_in = reference_timeline_in +
							core::rational::fromDouble(
								static_cast<double>(candidate_offset_samples) /
								static_cast<double>(sample_rate));
	placement.valid = !placement.timeline_in.isNaN();
	return placement;
}

}
