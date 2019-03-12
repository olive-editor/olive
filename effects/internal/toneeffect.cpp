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

#include "project/clip.h"
#include "project/sequence.h"
#include "debug.h"

ToneEffect::ToneEffect(Clip* c, const EffectMeta *em) : Effect(c, em), sinX(INT_MIN) {
  EffectRow* type_row = add_row(tr("Type"));
  type_val = new ComboField(type_row, "type");
  type_val->AddItem(tr("Sine"), TONE_TYPE_SINE);

  EffectRow* frequency_row = add_row(tr("Frequency"));
  freq_val = new DoubleField(frequency_row, "frequency");
  freq_val->SetMinimum(20);
  freq_val->SetMaximum(20000);
  freq_val->SetDefault(1000);

  EffectRow* amount_row = add_row(tr("Amount"));
  amount_val = new DoubleField(amount_row, "amount");
  amount_val->SetMinimum(0);
  amount_val->SetMaximum(100);
  amount_val->SetDefault(25);

  EffectRow* mix_row = add_row(tr("Mix"));
  mix_val = new BoolField(mix_row, "mix");
  mix_val->SetValueAt(0, true);
}

void ToneEffect::process_audio(double timecode_start, double timecode_end, quint8 *samples, int nb_bytes, int) {
  double interval = (timecode_end - timecode_start)/nb_bytes;
  for (int i=0;i<nb_bytes;i+=4) {
    double timecode = timecode_start+(interval*i);

    qint16 left_tone_sample = qint16(qRound(qSin((2*M_PI*sinX*freq_val->GetDoubleAt(timecode))
                                                 /parent_clip->sequence->audio_frequency)
                                            *log_volume(amount_val->GetDoubleAt(timecode)*0.01)*INT16_MAX));
    qint16 right_tone_sample = left_tone_sample;

    // mix with source audio
    if (mix_val->GetBoolAt(timecode)) {
      qint16 left_sample = qint16(((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
      qint16 right_sample = qint16(((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));
      left_tone_sample = mix_audio_sample(left_tone_sample, left_sample);
      right_tone_sample = mix_audio_sample(right_tone_sample, right_sample);
    }

    samples[i+3] = quint8(right_tone_sample >> 8);
    samples[i+2] = quint8(right_tone_sample);
    samples[i+1] = quint8(left_tone_sample >> 8);
    samples[i] = quint8(left_tone_sample);

    sinX++;
  }
}
