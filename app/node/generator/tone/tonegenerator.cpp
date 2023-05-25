/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "tonegenerator.h"

#include <QElapsedTimer>

#include "widget/slider/floatslider.h"

namespace olive {

const QString ToneGenerator::kFrequencyInput = QStringLiteral("freq_in");

#define super Node

ToneGenerator::ToneGenerator()
{
  AddInput(kFrequencyInput, TYPE_DOUBLE, 1000.0, kInputFlagStatic);
}

QString ToneGenerator::Name() const
{
  return tr("Tone");
}

QString ToneGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.tonegenerator");
}

QVector<Node::CategoryID> ToneGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString ToneGenerator::Description() const
{
  return tr("Generates an audio tone.");
}

void ToneGenerator::Retranslate()
{
  super::Retranslate();

  SetInputName(kFrequencyInput, tr("Frequency"));
}

void ProcessSamples(const void *context, const SampleJob &job, SampleBuffer &output)
{
  const double freq = job.Get(ToneGenerator::kFrequencyInput).toDouble();

  const ValueParams &vp = job.value_params();
  const double sample_interval = vp.aparams().sample_rate_as_time_base().toDouble();
  double time = vp.time().in().toDouble();

  for (size_t i = 0; i < output.sample_count(); i++) {
    static const double PI = 3.14159265358979323846264;
    double val = std::sin(time * 2 * PI * freq);

    for (int j = 0; j < output.channel_count(); j++) {
      output.data(j)[i] = val;
    }

    time += sample_interval;
  }
}

value_t ToneGenerator::Value(const ValueParams &p) const
{
  SampleJob job(p);
  job.Insert(kFrequencyInput, GetStandardValue(kFrequencyInput));
  job.set_function(ProcessSamples, this);
  return job;
}

}
