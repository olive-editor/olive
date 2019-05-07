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

#include "exponentialfadetransition.h"

#include <QtMath>

ExponentialFadeTransition::ExponentialFadeTransition(Clip* c) :
  Transition(c)
{
}

QString ExponentialFadeTransition::name()
{
  return tr("Exponential Fade");
}

QString ExponentialFadeTransition::id()
{
  return "org.olivevideoeditor.Olive.exponentialfade";
}

QString ExponentialFadeTransition::description()
{
  return tr("An exponential audio fade that starts slow and ends fast.");
}

EffectType ExponentialFadeTransition::type()
{
  return EFFECT_TYPE_TRANSITION;
}

olive::TrackType ExponentialFadeTransition::subtype()
{
  return olive::kTypeAudio;
}

OldEffectNodePtr ExponentialFadeTransition::Create(Clip *c)
{
  return std::make_shared<ExponentialFadeTransition>(c);
}

void ExponentialFadeTransition::process_audio(double timecode_start,
                                              double timecode_end,
                                              float **samples,
                                              int nb_samples,
                                              int channel_count,
                                              int type) {
  double interval = (timecode_end-timecode_start)/nb_samples;

  for (int i=0;i<nb_samples;i++) {

    double multi = timecode_start + (interval * i);

    for (int j=0;j<channel_count;j++) {
      switch (type) {
      case kTransitionOpening:
        samples[j][i] *= qPow(multi, 2);
        break;
      case kTransitionClosing:
        samples[j][i] *= qPow(1.0 - multi, 2);
        break;
      }
    }
  }
}
