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

#include "pan.h"

#include "widget/slider/floatslider.h"

namespace olive {

const QString PanNode::kSamplesInput = QStringLiteral("samples_in");
const QString PanNode::kPanningInput = QStringLiteral("panning_in");

#define super Node

PanNode::PanNode()
{
  AddInput(kSamplesInput, TYPE_SAMPLES, kInputFlagNotKeyframable);

  AddInput(kPanningInput, TYPE_DOUBLE, 0.0);
  SetInputProperty(kPanningInput, QStringLiteral("min"), -1.0);
  SetInputProperty(kPanningInput, QStringLiteral("max"), 1.0);
  SetInputProperty(kPanningInput, QStringLiteral("view"), FloatSlider::kPercentage);

  SetFlag(kAudioEffect);
  SetEffectInput(kSamplesInput);
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
  return {kCategoryFilter};
}

QString PanNode::Description() const
{
  return tr("Adjust the stereo panning of an audio source.");
}

void PanNode::ProcessSamples(const void *context, const SampleJob &job, SampleBuffer &output)
{
  const PanNode *n = static_cast<const PanNode *>(context);
  const ValueParams &p = job.value_params();

  const SampleBuffer input = job.Get(PanNode::kSamplesInput).toSamples();
  if (!input.is_allocated()) {
    return;
  }

  // This node is only compatible with stereo audio
  if (job.audio_params().channel_count() == 2) {
    if (n->IsInputStatic(kPanningInput)) {
      float pan_volume = n->GetInputValue(p, kPanningInput).toDouble();
      if (!qIsNull(pan_volume)) {
        if (pan_volume > 0) {
          SampleBuffer::transform_volume_for_channel(0, 1.0f - pan_volume, &input, &output);
          output.set(1, input.data(1), input.sample_count());
        } else {
          output.set(0, input.data(0), input.sample_count());
          SampleBuffer::transform_volume_for_channel(1, 1.0f + pan_volume, &input, &output);
        }
      }
    } else {
      for (size_t index = 0; index < output.sample_count(); index++) {
        rational this_sample_time = p.time().in() + rational(index, job.audio_params().sample_rate());
        TimeRange this_sample_range(this_sample_time, this_sample_time + job.audio_params().sample_rate_as_time_base());
        auto pan_val = n->GetInputValue(p.time_transformed(this_sample_range), kPanningInput).toDouble();

        if (pan_val > 0) {
          output.data(0)[index] = input.data(0)[index] * (1.0F - pan_val);
          output.data(1)[index] = input.data(1)[index];
        } else if (pan_val < 0) {
          output.data(0)[index] = input.data(0)[index];
          output.data(1)[index] = input.data(1)[index] * (1.0F - qAbs(pan_val));
        }
      }
    }
  }
}

value_t PanNode::Value(const ValueParams &p) const
{
  // Create a sample job
  value_t samples_original = GetInputValue(p, kSamplesInput);

  if (!IsInputStatic(kPanningInput) || !qIsNull(GetInputValue(p, kPanningInput).toDouble())) {
    // Requires sample job
    SampleJob job(p);

    job.Insert(kSamplesInput, GetInputValue(p, kSamplesInput));
    job.set_function(ProcessSamples, this);

    return job;
  }

  return samples_original;
}

void PanNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kSamplesInput, tr("Samples"));
  SetInputName(kPanningInput, tr("Pan"));
}

}
