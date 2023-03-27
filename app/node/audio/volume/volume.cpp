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

#include "widget/slider/floatslider.h"

namespace olive {

const QString VolumeNode::kSamplesInput = QStringLiteral("samples_in");
const QString VolumeNode::kVolumeInput = QStringLiteral("volume_in");

#define super MathNodeBase

VolumeNode::VolumeNode()
{
  AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));

  AddInput(kVolumeInput, NodeValue::kFloat, 1.0);
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

NodeValue VolumeNode::Value(const ValueParams &p) const
{
  // Create a sample job
  NodeValue meta = GetInputValue(p, kSamplesInput);

  if (!IsInputStatic(kVolumeInput) || !qFuzzyCompare(GetInputValue(p, kVolumeInput).toDouble(), 1.0)) {
    SampleJob job(p);

    job.Insert(kSamplesInput, GetInputValue(p, kSamplesInput));

    return NodeValue(NodeValue::kSamples, job, this);
  }

  return meta;
}

void VolumeNode::ProcessSamples(const SampleJob &job, SampleBuffer &output) const
{
  SampleBuffer buffer = job.Get(kSamplesInput).toSamples();
  const ValueParams &p = job.value_params();

  if (IsInputStatic(kVolumeInput)) {
    auto volume = GetInputValue(p, kVolumeInput).toDouble();

    if (!qFuzzyCompare(volume, 1.0)) {
      SampleBuffer::transform_volume(volume, &buffer, &output);
    }
  } else {
    return ProcessSamplesNumberInternal(p, kOpMultiply, kVolumeInput, buffer, output);
  }
}

void VolumeNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kSamplesInput, tr("Samples"));
  SetInputName(kVolumeInput, tr("Volume"));
}

}
