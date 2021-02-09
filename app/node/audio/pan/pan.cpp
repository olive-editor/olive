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

#include "pan.h"

#include "widget/slider/floatslider.h"

namespace olive {

const QString PanNode::kSamplesInput = QStringLiteral("samples_in");
const QString PanNode::kPanningInput = QStringLiteral("panning_in");

PanNode::PanNode()
{
  AddInput(kSamplesInput, NodeValue::kSamples, InputFlags(kInputFlagNotKeyframable));

  AddInput(kPanningInput, NodeValue::kFloat, 0.0);
  SetInputProperty(kPanningInput, QStringLiteral("min"), -1.0);
  SetInputProperty(kPanningInput, QStringLiteral("max"), 1.0);
  SetInputProperty(kPanningInput, QStringLiteral("view"), FloatSlider::kPercentage);
}

Node *PanNode::copy() const
{
  return new PanNode();
}

QString PanNode::Name() const
{
  return tr("Pan");
}

QString PanNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.pan");
}

QVector<Node::CategoryID> PanNode::Category() const
{
  return {kCategoryChannels};
}

QString PanNode::Description() const
{
  return tr("Adjust the stereo panning of an audio source.");
}

NodeValueTable PanNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  // Create a sample job
  SampleJob job(kSamplesInput, value);
  job.InsertValue(this, kPanningInput, value);

  // Push it to our table
  NodeValueTable table = value.Merge();

  if (job.HasSamples()) {
    float pan_volume = job.GetValue(kPanningInput).data().toFloat();
    if (IsInputStatic(kPanningInput)) {
      if (!qIsNull(pan_volume) && job.samples()->audio_params().channel_count() == 2) {
        if (pan_volume > 0) {
          job.samples()->transform_volume_for_channel(0, 1.0f - pan_volume);
        } else {
          job.samples()->transform_volume_for_channel(1, 1.0f + pan_volume);
        }
      }

      table.Push(NodeValue::kSamples, QVariant::fromValue(job.samples()), this);
    } else {
      table.Push(NodeValue::kSampleJob, QVariant::fromValue(job), this);
    }
  }

  return table;
}

void PanNode::ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  if (input->audio_params().channel_count() != 2) {
    // This node currently only works for stereo audio
    return;
  }

  float pan_val = values[kPanningInput].Get(NodeValue::kFloat).toFloat();

  for (int i=0;i<input->audio_params().channel_count();i++) {
    output->data()[i][index] = input->data()[i][index];
  }

  if (pan_val > 0) {
    output->data()[0][index] *= (1.0F - pan_val);
  } else if (pan_val < 0) {
    output->data()[1][index] *= (1.0F - qAbs(pan_val));
  }
}

void PanNode::Retranslate()
{
  SetInputName(kSamplesInput, tr("Samples"));
  SetInputName(kPanningInput, tr("Pan"));
}

}
