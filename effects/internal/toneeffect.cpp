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

#include "toneeffect.h"

#include <QtMath>

#define TONE_TYPE_SINE 0

#include "timeline/clip.h"
#include "timeline/sequence.h"

ToneEffect::ToneEffect(Clip* c, const EffectMeta *em) : Effect(c, em), sinX(INT_MIN) {
  type_val = new ComboInput(this, "type", tr("Type"));
  type_val->AddItem(tr("Sine"), TONE_TYPE_SINE);

  freq_val = new DoubleInput(this, "frequency", tr("Frequency"));
  freq_val->SetMinimum(20);
  freq_val->SetMaximum(20000);
  freq_val->SetDefault(1000);

  amount_val = new DoubleInput(this, "amount", tr("Amount"));
  amount_val->SetMinimum(0);
  amount_val->SetMaximum(100);
  amount_val->SetDefault(25);

  mix_val = new BoolInput(this, "mix", tr("Mix"));
  mix_val->SetValueAt(0, true);
}

void ToneEffect::process_audio(double timecode_start,
                               double timecode_end,
                               float **samples,
                               int nb_samples,
                               int channel_count,
                               int type) {

  Q_UNUSED(type)

  double interval = (timecode_end - timecode_start)/nb_samples;

  for (int i=0;i<nb_samples;i++) {

    double timecode = timecode_start+(interval*i);
    float tone_sample = qSin((2*M_PI*sinX*freq_val->GetDoubleAt(timecode))
                                           /parent_clip->track()->sequence()->audio_frequency)
                                      *log_volume(amount_val->GetDoubleAt(timecode)*0.01);

    for (int j=0;j<channel_count;j++) {

      if (mix_val->GetBoolAt(timecode)) {

        // mix with source audio
        samples[j][i] += tone_sample;

      } else {

        // replace source audio
        samples[j][i] = tone_sample;

      }

    }

    sinX++;
  }
}
