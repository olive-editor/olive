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

#include "shakeeffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <QOpenGLFunctions>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "panels/timeline.h"
#include "global/debug.h"

ShakeEffect::ShakeEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
  SetFlags(Effect::CoordsFlag);

  EffectRow* intensity_row = new EffectRow(this, tr("Intensity"));
  intensity_val = new DoubleField(intensity_row, "intensity");
  intensity_val->SetMinimum(0);
  intensity_val->SetDefault(25);

  EffectRow* rotation_row = new EffectRow(this, tr("Rotation"));
  rotation_val = new DoubleField(rotation_row, "rotation");
  rotation_val->SetMinimum(0);
  rotation_val->SetDefault(10);

  EffectRow* frequency_row = new EffectRow(this, tr("Frequency"));
  frequency_val = new DoubleField(frequency_row, "frequency");
  frequency_val->SetMinimum(0);
  frequency_val->SetDefault(5);

  const auto limit = std::numeric_limits<int32_t>::max();
  for (int i=0;i<RANDOM_VAL_SIZE;i++) {
    random_vals[i] = static_cast<double>(this->randomNumber<int32_t>()) / limit;
  }
}

void ShakeEffect::process_coords(double timecode, GLTextureCoords& coords, int) {
  int lim = RANDOM_VAL_SIZE/6;

  double multiplier = intensity_val->GetDoubleAt(timecode)/lim;
  double rotmult = rotation_val->GetDoubleAt(timecode)/lim/10;
  double x = timecode * frequency_val->GetDoubleAt(timecode);

  double xoff = 0;
  double yoff = 0;
  double rotoff = 0;

  for (int i=0;i<lim;i++) {
    int offset = 6*i;
    xoff += qSin((x+random_vals[offset])*random_vals[offset+1]);
    yoff += qSin((x+random_vals[offset+2])*random_vals[offset+3]);
    rotoff += qSin((x+random_vals[offset+4])*random_vals[offset+5]);
  }

  xoff *= multiplier;
  yoff *= multiplier;
  rotoff *= rotmult;

  coords.vertexTopLeftX += xoff;
  coords.vertexTopRightX += xoff;
  coords.vertexBottomLeftX += xoff;
  coords.vertexBottomRightX += xoff;

  coords.vertexTopLeftY += yoff;
  coords.vertexTopRightY += yoff;
  coords.vertexBottomLeftY += yoff;
  coords.vertexBottomRightY += yoff;

  glRotatef(rotoff, 0, 0, 1);
}
