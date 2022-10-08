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

  SetFlags(kAudioEffect);
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

void VolumeNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // Create a sample job
  SampleBuffer buffer = value[kSamplesInput].toSamples();

  if (buffer.is_allocated()) {
    // If the input is static, we can just do it now which will be faster
    if (IsInputStatic(kVolumeInput)) {
      auto volume = value[kVolumeInput].toDouble();

      if (!qFuzzyCompare(volume, 1.0)) {
        buffer.transform_volume(volume);
      }

      table->Push(NodeValue::kSamples, QVariant::fromValue(buffer), this);
    } else {
      // Requires job
      SampleJob job(globals.time(), kSamplesInput, value);
      job.Insert(kVolumeInput, value);
      table->Push(NodeValue::kSamples, QVariant::fromValue(job), this);
    }
  }
}

void VolumeNode::ProcessSamples(const NodeValueRow &values, const SampleBuffer &input, SampleBuffer &output, int index) const
{
  return ProcessSamplesInternal(values, kOpMultiply, kSamplesInput, kVolumeInput, input, output, index);
}

void VolumeNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kSamplesInput, tr("Samples"));
  SetInputName(kVolumeInput, tr("Volume"));
}

}
