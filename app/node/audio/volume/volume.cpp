/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "widget/slider/floatslider.h"

namespace olive {

const QString VolumeNode::kSamplesInput = QStringLiteral("samples_in");
const QString VolumeNode::kVolumeInput = QStringLiteral("volume_in");

VolumeNode::VolumeNode()
{
  AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));

  AddInput(kVolumeInput, NodeValue::kFloat, 1.0);
  SetInputProperty(kVolumeInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kVolumeInput, QStringLiteral("view"), FloatSlider::kDecibel);
}

Node *VolumeNode::copy() const
{
  return new VolumeNode();
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

NodeValueTable VolumeNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  return ValueInternal(value,
                       kOpMultiply,
                       kPairSampleNumber,
                       kSamplesInput,
                       value[kSamplesInput].TakeWithMeta(NodeValue::kSamples),
                       kVolumeInput,
                       value[kVolumeInput].TakeWithMeta(NodeValue::kFloat));
}

void VolumeNode::ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  return ProcessSamplesInternal(values, kOpMultiply, kSamplesInput, kVolumeInput, input, output, index);
}

void VolumeNode::Retranslate()
{
  SetInputName(kSamplesInput, tr("Samples"));
  SetInputName(kVolumeInput, tr("Volume"));
}

}
