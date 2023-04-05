/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "volume.h"

#include "node/math/math/mathfunctions.h"
#include "widget/slider/floatslider.h"

namespace olive {

const QString VolumeNode::kSamplesInput = QStringLiteral("samples_in");
const QString VolumeNode::kVolumeInput = QStringLiteral("volume_in");

#define super Node

VolumeNode::VolumeNode()
{
  AddInput(kSamplesInput, TYPE_SAMPLES, kInputFlagNotKeyframable);

  AddInput(kVolumeInput, TYPE_DOUBLE, 1.0);
  SetInputProperty(kVolumeInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kVolumeInput, QStringLiteral("view"), FloatSlider::kDecibel);

  SetFlag(kAudioEffect);
  SetEffectInput(kSamplesInput);
}

QString VolumeNode::Name() const
{
  return tr("Volume");
}

QString VolumeNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.volume");
}

QVector<Node::CategoryID> VolumeNode::Category() const
{
  return {kCategoryFilter};
}

QString VolumeNode::Description() const
{
  return tr("Adjusts the volume of an audio source.");
}

value_t VolumeNode::Value(const ValueParams &p) const
{
  return Math::MultiplySamplesDouble(GetInputValue(p, kSamplesInput), GetInputValue(p, kVolumeInput));
}

void VolumeNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kSamplesInput, tr("Samples"));
  SetInputName(kVolumeInput, tr("Volume"));
}

}
