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

#include "logarithmicfadetransition.h"

#include <QtMath>

LogarithmicFadeTransition::LogarithmicFadeTransition(Clip* c) :
  Transition(c)
{
}

QString LogarithmicFadeTransition::name()
{
  return tr("Logarithmic Fade");
}

QString LogarithmicFadeTransition::id()
{
  return "org.olivevideoeditor.Olive.logarithmicfade";
}

QString LogarithmicFadeTransition::description()
{
  return tr("An logarithmic audio fade that starts fast and ends slow.");
}

EffectType LogarithmicFadeTransition::type()
{
  return EFFECT_TYPE_TRANSITION;
}

olive::TrackType LogarithmicFadeTransition::subtype()
{
  return olive::kTypeAudio;
}

NodePtr LogarithmicFadeTransition::Create(Clip *c)
{
  return std::make_shared<LogarithmicFadeTransition>(c);
}

void LogarithmicFadeTransition::process_audio(double timecode_start,
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
        samples[j][i] *= qSqrt(multi);
        break;
      case kTransitionClosing:
        samples[j][i] *= qSqrt(1.0f - multi);
        break;
      }
    }
  }
}
