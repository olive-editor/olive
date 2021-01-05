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

namespace olive {

VolumeNode::VolumeNode()
{
  samples_input_ = new NodeInput(this, QStringLiteral("samples_in"), NodeValue::kSamples);
  samples_input_->SetKeyframable(false);

  volume_input_ = new NodeInput(this, QStringLiteral("volume_in"), NodeValue::kFloat, 1.0);
  volume_input_->setProperty("min", 0.0);
  volume_input_->setProperty("view", QStringLiteral("db"));
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

NodeValueTable VolumeNode::Value(NodeValueDatabase &value) const
{
  return ValueInternal(value,
                       kOpMultiply,
                       kPairSampleNumber,
                       samples_input_,
                       value[samples_input_].TakeWithMeta(NodeValue::kSamples),
                       volume_input_,
                       value[volume_input_].TakeWithMeta(NodeValue::kFloat));
}

void VolumeNode::ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  return ProcessSamplesInternal(values, kOpMultiply, samples_input_, volume_input_, input, output, index);
}

void VolumeNode::Retranslate()
{
  samples_input_->set_name(tr("Samples"));
  volume_input_->set_name(tr("Volume"));
}

}
