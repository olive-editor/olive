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

#ifndef TIMECODEMETADATA_H
#define TIMECODEMETADATA_H

#include <QString>

#include "olive/core/util/rational.h"

namespace olive
{

class TimecodeMetadata {
public:
	struct SourceTime {
		core::rational time;
		QString source;
		bool valid = false;
	};

	static SourceTime FromTimecodeString(const QString &timecode,
										 const core::rational &timebase);

	static SourceTime FromBwfTimeReference(const QString &time_reference,
										   int sample_rate);
};

}

#endif // TIMECODEMETADATA_H
