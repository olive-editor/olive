/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "crossdissolvetransition.h"

namespace olive {

CrossDissolveTransition::CrossDissolveTransition()
{

}

Node *CrossDissolveTransition::copy() const
{
  return new CrossDissolveTransition();
}

QString CrossDissolveTransition::Name() const
{
  return tr("Cross Dissolve");
}

QString CrossDissolveTransition::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.crossdissolve");
}

QVector<Node::CategoryID> CrossDissolveTransition::Category() const
{
  return {kCategoryTransition};
}

QString CrossDissolveTransition::Description() const
{
  return tr("Smoothly transition between two clips.");
}

ShaderCode CrossDissolveTransition::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/crossdissolve.frag"), QString());
}

void CrossDissolveTransition::ShaderJobEvent(NodeValueDatabase &value, ShaderJob &job) const
{
  Q_UNUSED(value)

  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
}

void CrossDissolveTransition::SampleJobEvent(SampleBufferPtr from_samples, SampleBufferPtr to_samples, SampleBufferPtr out_samples, double time_in) const
{
  for (int i=0; i<out_samples->sample_count(); i++) {
    double this_sample_time = out_samples->audio_params().samples_to_time(i).toDouble() + time_in;
    double progress = GetTotalProgress(this_sample_time);

    for (int j=0; j<out_samples->audio_params().channel_count(); j++) {
      out_samples->data(j)[i] = 0;

      if (from_samples) {
        if (i < from_samples->sample_count()) {
          out_samples->data(j)[i] += from_samples->data(j)[i] * TransformCurve(1.0 - progress);
        }
      }

      if (to_samples) {
        // Offset input samples from the end
        int in_index = i - (out_samples->sample_count() - to_samples->sample_count());

        if (in_index >= 0) {
          out_samples->data(j)[i] += to_samples->data(j)[in_index] * TransformCurve(progress);
        }
      }
    }
  }
}

}
