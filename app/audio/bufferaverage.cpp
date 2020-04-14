/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "bufferaverage.h"

OLIVE_NAMESPACE_ENTER

QVector<double> AudioBufferAverage::ProcessAverages(const char *data, int length, int channel_count)
{
  // FIXME: Assumes float and stereo
  const float* samples = reinterpret_cast<const float*>(data);
  int sample_count = static_cast<int>(length / static_cast<int>(sizeof(float)));

  // Create array of samples to send
  QVector<double> averages(channel_count);
  averages.fill(0);

  // Add all samples together
  for (int i=0;i<sample_count;i++) {
    averages[i%channel_count] = qMax(averages[i%channel_count], static_cast<double>(qAbs(samples[i])));
  }

  return averages;
}

OLIVE_NAMESPACE_EXIT
