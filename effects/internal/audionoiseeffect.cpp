/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "audionoiseeffect.h"

#include <QDateTime>
#include <QtMath>

AudioNoiseEffect::AudioNoiseEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
  amount_val = new DoubleInput(this, "amount", tr("Amount"));
  amount_val->SetMinimum(0);
  amount_val->SetDefault(20);
  amount_val->SetMaximum(100);

  mix_val = new BoolInput(this, "mix", tr("Mix"));
  mix_val->SetValueAt(0, true);
}

void AudioNoiseEffect::process_audio(double timecode_start,
                                     double timecode_end,
                                     float **samples,
                                     int nb_samples,
                                     int channel_count,
                                     int type) {

  Q_UNUSED(type)

  double interval = (timecode_end - timecode_start)/nb_samples;
  for (int i=0;i<nb_samples;i+=4) {
    double timecode = timecode_start+(interval*i);

    // set noise volume
    float vol = log_volume( amount_val->GetDoubleAt(timecode)*0.01 );

    for (int j=0;j<channel_count;j++) {
      // Generate noise sample
      float noise_sample = this->randomFloat<float>() * vol;

      // mix with source audio
      if (mix_val->GetBoolAt(timecode)) {
        samples[j][i] += noise_sample;
      } else {
        samples[j][i] = noise_sample;
      }
    }
  }
}
