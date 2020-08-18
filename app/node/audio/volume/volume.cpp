/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

OLIVE_NAMESPACE_ENTER

VolumeNode::VolumeNode()
{
  samples_input_ = new NodeInput("samples_in", NodeParam::kSamples);
  AddInput(samples_input_);

  volume_input_ = new NodeInput("volume_in", NodeParam::kFloat, 1.0);
  volume_input_->set_property("min", 0.0);
  volume_input_->set_property("view", "db");
  AddInput(volume_input_);
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

QList<Node::CategoryID> VolumeNode::Category() const
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
                       value[samples_input_].TakeWithMeta(NodeParam::kSamples),
                       volume_input_,
                       value[volume_input_].TakeWithMeta(NodeParam::kFloat));
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

OLIVE_NAMESPACE_EXIT
