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

#include "crossdissolvetransition.h"

namespace olive {

CrossDissolveTransition::CrossDissolveTransition()
{
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

ShaderCode CrossDissolveTransition::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/crossdissolve.frag"));
}

void CrossDissolveTransition::ShaderJobEvent(const ValueParams &p, ShaderJob *job) const
{
  job->set_function(GetShaderCode);
}

void CrossDissolveTransition::ProcessSamples(const void *context, const SampleJob &job, SampleBuffer &out_samples)
{
  const CrossDissolveTransition *t = static_cast<const CrossDissolveTransition *>(context);
  SampleBuffer from_samples = job.Get(TransitionBlock::kOutBlockInput).toSamples();
  SampleBuffer to_samples = job.Get(TransitionBlock::kInBlockInput).toSamples();

  double time_in = job.value_params().time().in().toDouble();

  for (size_t i=0; i<out_samples.sample_count(); i++) {
    double this_sample_time = out_samples.audio_params().samples_to_time(i).toDouble() + time_in;
    double progress = t->GetTotalProgress(this_sample_time);

    for (int j=0; j<out_samples.audio_params().channel_count(); j++) {
      out_samples.data(j)[i] = 0;

      if (from_samples.is_allocated()) {
        if (i < from_samples.sample_count()) {
          out_samples.data(j)[i] += from_samples.data(j)[i] * t->TransformCurve(1.0 - progress);
        }
      }

      if (to_samples.is_allocated()) {
        // Offset input samples from the end
        size_t remain = (out_samples.sample_count() - to_samples.sample_count());
        if (i >= remain) {
          qint64 in_index = i - remain;
          out_samples.data(j)[i] += to_samples.data(j)[in_index] * t->TransformCurve(progress);
        }
      }
    }
  }
}

void CrossDissolveTransition::SampleJobEvent(const ValueParams &p, SampleJob *job) const
{
  job->set_function(ProcessSamples, this);
}

}
