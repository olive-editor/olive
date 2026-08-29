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

#ifndef AUDIOLEVELMETER_H
#define AUDIOLEVELMETER_H

#include <QVector>

#include "olive/core/render/samplebuffer.h"

namespace olive
{

class AudioLevelMeter {
public:
	struct ChannelStats {
		double peak_linear = 0.0;
		double peak_db = -200.0;
		double rms_linear = 0.0;
		double rms_db = -200.0;
		double vu_db = -200.0;
	};

	struct Stats {
		QVector<ChannelStats> channels;
		double max_peak_linear = 0.0;
		double integrated_lufs = -200.0;
		bool silence = true;
	};

	static Stats AnalyzeSampleBuffer(const core::SampleBuffer &samples);

private:
	static double LinearToDb(double linear);
	static double PowerToLufs(double mean_square);
};

}

#endif // AUDIOLEVELMETER_H
