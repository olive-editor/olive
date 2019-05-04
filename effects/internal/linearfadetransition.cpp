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

#include "linearfadetransition.h"

LinearFadeTransition::LinearFadeTransition(Clip* c) : Transition(c) {}

QString LinearFadeTransition::name()
{
  return tr("Linear Fade");
}

QString LinearFadeTransition::id()
{
  return "org.olivevideoeditor.Olive.linearfade";
}

QString LinearFadeTransition::description()
{
  return tr("An linear audio fade that fades evenly at a constant rate.");
}

EffectType LinearFadeTransition::type()
{
  return EFFECT_TYPE_TRANSITION;
}

olive::TrackType LinearFadeTransition::subtype()
{
  return olive::kTypeAudio;
}

NodePtr LinearFadeTransition::Create(Clip *c)
{
  return std::make_shared<LinearFadeTransition>(c);
}

void LinearFadeTransition::process_audio(double timecode_start,
                                         double timecode_end,
                                         float **samples,
                                         int nb_samples,
                                         int channel_count,
                                         int type) {
  double interval = (timecode_end-timecode_start)/nb_samples;

  for (int i=0;i<nb_samples;i++) {

    float multi = timecode_start + (interval * i);

    for (int j=0;j<channel_count;j++) {

      switch (type) {
      case kTransitionOpening:
        samples[j][i] *= multi;
        break;
      case kTransitionClosing:
        samples[j][i] *= 1.0f - multi;
        break;
      }
    }
  }
}
