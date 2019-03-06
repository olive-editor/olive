/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "audionoiseeffect.h"

#include <QDateTime>
#include <QtMath>

AudioNoiseEffect::AudioNoiseEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
    amount_val = add_row(tr("Amount"))->add_field(EFFECT_FIELD_DOUBLE, "amount");
  amount_val->set_double_minimum_value(0);
  amount_val->set_double_maximum_value(100);
  amount_val->set_double_default_value(20);

    mix_val = add_row(tr("Mix"))->add_field(EFFECT_FIELD_BOOL, "mix");
  mix_val->set_bool_value(true);

}

void AudioNoiseEffect::process_audio(double timecode_start, double timecode_end, quint8 *samples, int nb_bytes, int) {
  double interval = (timecode_end - timecode_start)/nb_bytes;
  for (int i=0;i<nb_bytes;i+=4) {
    double timecode = timecode_start+(interval*i);

    qint16 left_noise_sample = this->randomNumber<qint16>();
    qint16 right_noise_sample = this->randomNumber<qint16>();

    // set noise volume
    double vol = log_volume( amount_val->get_double_value(timecode, true)*0.01 );
    left_noise_sample *= vol;
    right_noise_sample *= vol;

    // mix with source audio
    if (mix_val->get_bool_value(timecode, true)) {
      qint16 left_sample = static_cast<qint16> (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));
      qint16 right_sample = static_cast<qint16> (((samples[i+3] & 0xFF) << 8) | (samples[i+2] & 0xFF));
      left_noise_sample = mix_audio_sample(left_noise_sample, left_sample);
      right_noise_sample = mix_audio_sample(right_noise_sample, right_sample);
    }

    samples[i+3] = static_cast<quint8> (right_noise_sample >> 8);
    samples[i+2] = static_cast<quint8> (right_noise_sample);
    samples[i+1] = static_cast<quint8> (left_noise_sample >> 8);
    samples[i] = static_cast<quint8> (left_noise_sample);
  }
}
