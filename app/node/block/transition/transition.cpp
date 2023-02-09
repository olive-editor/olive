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

#include "transition.h"

#include "node/block/clip/clip.h"
#include "node/output/track/track.h"
#include "widget/slider/rationalslider.h"

namespace olive {

#define super Block

const QString TransitionBlock::kOutBlockInput = QStringLiteral("out_block_in");
const QString TransitionBlock::kInBlockInput = QStringLiteral("in_block_in");
const QString TransitionBlock::kCurveInput = QStringLiteral("curve_in");
const QString TransitionBlock::kCenterInput = QStringLiteral("center_in");

TransitionBlock::TransitionBlock() :
  connected_out_block_(nullptr),
  connected_in_block_(nullptr)
{
  AddInput(kOutBlockInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));

  AddInput(kInBlockInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));

  AddInput(kCurveInput, NodeValue::kCombo, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

  AddInput(kCenterInput, NodeValue::kRational, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
  SetInputProperty(kCenterInput, QStringLiteral("view"), RationalSlider::kTime);
  SetInputProperty(kCenterInput, QStringLiteral("viewlock"), true);

  SetFlags(GetFlags() & ~kDontShowInParamView);
}

void TransitionBlock::Retranslate()
{
  super::Retranslate();

  SetInputName(kOutBlockInput, tr("From"));
  SetInputName(kInBlockInput, tr("To"));
  SetInputName(kCurveInput, tr("Curve"));
  SetInputName(kCenterInput, tr("Center Offset"));

  // These must correspond to the CurveType enum
  SetComboBoxStrings(kCurveInput, { tr("Linear"), tr("Exponential"), tr("Logarithmic") });
}

rational TransitionBlock::in_offset() const
{
  if (is_dual_transition()) {
    return length()/2 + offset_center();
  } else if (connected_in_block()) {
    return length();
  } else {
    return 0;
  }
}

rational TransitionBlock::out_offset() const
{
  if (is_dual_transition()) {
    return length()/2 - offset_center();
  } else if (connected_out_block()) {
    return length();
  } else {
    return 0;
  }
}

rational TransitionBlock::offset_center() const
{
  return GetStandardValue(kCenterInput).value<rational>();
}

void TransitionBlock::set_offset_center(const rational &r)
{
  SetStandardValue(kCenterInput, QVariant::fromValue(r));
}

void TransitionBlock::set_offsets_and_length(const rational &in_offset, const rational &out_offset)
{
  rational len = in_offset + out_offset;
  rational center = len / 2 - in_offset;

  set_length_and_media_out(len);
  set_offset_center(center);
}

Block *TransitionBlock::connected_out_block() const
{
  return connected_out_block_;
}

Block *TransitionBlock::connected_in_block() const
{
  return connected_in_block_;
}

double TransitionBlock::GetTotalProgress(const double &time) const
{
  return GetInternalTransitionTime(time) / length().toDouble();
}

double TransitionBlock::GetOutProgress(const double &time) const
{
  if (out_offset() == 0) {
    return 0;
  }

  return std::clamp(1.0 - (GetInternalTransitionTime(time) / out_offset().toDouble()), 0.0, 1.0);
}

double TransitionBlock::GetInProgress(const double &time) const
{
  if (in_offset() == 0) {
    return 0;
  }

  return std::clamp((GetInternalTransitionTime(time) - out_offset().toDouble()) / in_offset().toDouble(), 0.0, 1.0);
}

double TransitionBlock::GetInternalTransitionTime(const double &time) const
{
  return time;
}

void TransitionBlock::InsertTransitionTimes(AcceleratedJob *job, const double &time) const
{
  // Provides total transition progress from 0.0 (start) - 1.0 (end)
  job->Insert(QStringLiteral("ove_tprog_all"),
                   NodeValue(NodeValue::kFloat, GetTotalProgress(time), this));

  // Provides progress of out section from 1.0 (start) - 0.0 (end)
  job->Insert(QStringLiteral("ove_tprog_out"),
                   NodeValue(NodeValue::kFloat, GetOutProgress(time), this));

  // Provides progress of in section from 0.0 (start) - 1.0 (end)
  job->Insert(QStringLiteral("ove_tprog_in"),
                   NodeValue(NodeValue::kFloat, GetInProgress(time), this));
}

void TransitionBlock::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  NodeValue out_buffer = value[kOutBlockInput];
  NodeValue in_buffer = value[kInBlockInput];
  NodeValue::Type data_type = (out_buffer.type() != NodeValue::kNone) ? out_buffer.type() : in_buffer.type();

  NodeValue::Type job_type = NodeValue::kNone;
  QVariant push_job;

  if (data_type == NodeValue::kTexture) {
    // This must be a visual transition
    ShaderJob job;

    if (out_buffer.type() != NodeValue::kNone) {
      job.Insert(kOutBlockInput, out_buffer);
    } else {
      job.Insert(kOutBlockInput, NodeValue(NodeValue::kTexture, nullptr));
    }

    if (in_buffer.type() != NodeValue::kNone) {
      job.Insert(kInBlockInput, in_buffer);
    } else {
      job.Insert(kInBlockInput, NodeValue(NodeValue::kTexture, nullptr));
    }

    job.Insert(kCurveInput, value);

    double time = globals.time().in().toDouble();
    InsertTransitionTimes(&job, time);

    ShaderJobEvent(value, &job);

    job_type = NodeValue::kTexture;
    push_job = QVariant::fromValue(Texture::Job(globals.vparams(), job));
  } else if (data_type == NodeValue::kSamples) {
    // This must be an audio transition
    SampleBuffer from_samples = out_buffer.toSamples();
    SampleBuffer to_samples = in_buffer.toSamples();

    if (from_samples.is_allocated() || to_samples.is_allocated()) {
      double time_in = globals.time().in().toDouble();
      double time_out = globals.time().out().toDouble();

      const AudioParams& params = (from_samples.is_allocated()) ? from_samples.audio_params() : to_samples.audio_params();

      SampleBuffer out_samples;

      if (params.is_valid()) {
        int nb_samples = params.time_to_samples(time_out - time_in);

        out_samples = SampleBuffer(params, nb_samples);
        SampleJobEvent(from_samples, to_samples, out_samples, time_in);
      }

      job_type = NodeValue::kSamples;
      push_job = QVariant::fromValue(out_samples);
    }
  }

  if (!push_job.isNull()) {
    table->Push(job_type, push_job, this);
  }
}

void TransitionBlock::InvalidateCache(const TimeRange &range, const QString &from, int element, InvalidateCacheOptions options)
{
  TimeRange r = range;

  if (from == kOutBlockInput || from == kInBlockInput) {
    Block *n = dynamic_cast<Block*>(GetConnectedOutput(from));
    if (n) {
      r = Track::TransformRangeFromBlock(n, r);
    }
  }

  super::InvalidateCache(r, from, element, options);
}

double TransitionBlock::TransformCurve(double linear) const
{
  switch (static_cast<CurveType>(GetStandardValue(kCurveInput).toInt())) {
  case kLinear:
    break;
  case kExponential:
    linear *= linear;
    break;
  case kLogarithmic:
    linear = std::sqrt(linear);
    break;
  }

  return linear;
}

void TransitionBlock::InputConnectedEvent(const QString &input, int element, Node *output)
{
  Q_UNUSED(element)

  if (input == kOutBlockInput) {
    // If node is not a block, this will just be null
    if ((connected_out_block_ = dynamic_cast<ClipBlock*>(output))) {
      connected_out_block_->set_out_transition(this);
    }
  } else if (input == kInBlockInput) {
    // If node is not a block, this will just be null
    if ((connected_in_block_ = dynamic_cast<ClipBlock*>(output))) {
      connected_in_block_->set_in_transition(this);
    }
  }
}

void TransitionBlock::InputDisconnectedEvent(const QString &input, int element, Node *output)
{
  Q_UNUSED(element)
  Q_UNUSED(output)

  if (input == kOutBlockInput) {
    if (connected_out_block_) {
      connected_out_block_->set_out_transition(nullptr);
      connected_out_block_ = nullptr;
    }
  } else if (input == kInBlockInput) {
    if (connected_in_block_) {
      connected_in_block_->set_in_transition(nullptr);
      connected_in_block_ = nullptr;
    }
  }
}

TimeRange TransitionBlock::InputTimeAdjustment(const QString &input, int element, const TimeRange &input_time, bool clamp) const
{
  if (input == kInBlockInput || input == kOutBlockInput) {
    Block* block = dynamic_cast<Block*>(GetConnectedOutput(input));
    if (block) {
      // Retransform time as if it came from the track
      return input_time + in() - block->in();
    }
  }

  return super::InputTimeAdjustment(input, element, input_time, clamp);
}

TimeRange TransitionBlock::OutputTimeAdjustment(const QString &input, int element, const TimeRange &input_time) const
{
  if (input == kInBlockInput || input == kOutBlockInput) {
    Block* block = dynamic_cast<Block*>(GetConnectedOutput(input));
    if (block) {
      return input_time + block->in() - in();
    }
  }

  return super::OutputTimeAdjustment(input, element, input_time);
}

}
