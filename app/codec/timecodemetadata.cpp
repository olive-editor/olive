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

#include "timecodemetadata.h"

#include <limits>
#include <numeric>

#include "olive/core/util/timecodefunctions.h"

namespace olive
{

TimecodeMetadata::SourceTime
TimecodeMetadata::FromTimecodeString(const QString &timecode,
									 const core::rational &timebase)
{
	SourceTime result;
	const QString trimmed = timecode.trimmed();
	if (trimmed.isEmpty()) {
		return result;
	}

	bool ok = false;
	const core::Timecode::Display display =
		trimmed.contains(';') ? core::Timecode::kTimecodeDropFrame :
								core::Timecode::kTimecodeNonDropFrame;
	result.time = core::Timecode::timecode_to_time(trimmed.toStdString(),
												   timebase, display, &ok);
	result.valid = ok;
	if (ok) {
		result.source = QStringLiteral("timecode");
	}
	return result;
}

TimecodeMetadata::SourceTime
TimecodeMetadata::FromBwfTimeReference(const QString &time_reference,
									   int sample_rate)
{
	SourceTime result;
	if (sample_rate <= 0) {
		return result;
	}

	bool ok = false;
	const qulonglong samples = time_reference.trimmed().toULongLong(&ok);
	if (!ok) {
		return result;
	}

	qulonglong numerator = samples;
	qulonglong denominator = static_cast<qulonglong>(sample_rate);
	const qulonglong divisor = std::gcd(numerator, denominator);
	numerator /= divisor;
	denominator /= divisor;

	const qulonglong rational_limit =
		static_cast<qulonglong>(std::numeric_limits<int>::max());
	if (numerator <= rational_limit && denominator <= rational_limit) {
		result.time = core::rational(static_cast<int>(numerator),
									 static_cast<int>(denominator));
	} else {
		result.time = core::rational::fromDouble(
			static_cast<double>(samples) / static_cast<double>(sample_rate));
	}
	result.source = QStringLiteral("bwf_time_reference");
	result.valid = true;
	return result;
}

}
