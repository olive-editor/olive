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

#include "transition.h"

#include "common/clamp.h"

namespace olive {

TransitionBlock::TransitionBlock() :
  connected_out_block_(nullptr),
  connected_in_block_(nullptr)
{
  out_block_input_ = new NodeInput(this, QStringLiteral("out_block_in"), NodeValue::kNone);
  out_block_input_->SetKeyframable(false);
  connect(out_block_input_, &NodeInput::InputConnected, this, &TransitionBlock::InBlockConnected);
  connect(out_block_input_, &NodeInput::InputDisconnected, this, &TransitionBlock::InBlockDisconnected);

  in_block_input_ = new NodeInput(this, QStringLiteral("in_block_in"), NodeValue::kNone);
  in_block_input_->SetKeyframable(false);
  connect(in_block_input_, &NodeInput::InputConnected, this, &TransitionBlock::OutBlockConnected);
  connect(in_block_input_, &NodeInput::InputDisconnected, this, &TransitionBlock::OutBlockDisconnected);

  curve_input_ = new NodeInput(this, QStringLiteral("curve_in"), NodeValue::kCombo);
  curve_input_->SetKeyframable(false);
  curve_input_->SetConnectable(false);
}

Block::Type TransitionBlock::type() const
{
  return kTransition;
}

NodeInput *TransitionBlock::out_block_input() const
{
  return out_block_input_;
}

NodeInput *TransitionBlock::in_block_input() const
{
  return in_block_input_;
}

void TransitionBlock::Retranslate()
{
  Block::Retranslate();

  out_block_input_->set_name(tr("From"));
  in_block_input_->set_name(tr("To"));
  curve_input_->set_name(tr("Curve"));

  // These must correspond to the CurveType enum
  curve_input_->set_combobox_strings({ tr("Linear"), tr("Exponential"), tr("Logarithmic") });
}

rational TransitionBlock::in_offset() const
{
  // If no in block is connected, there's no in offset
  if (!connected_in_block()) {
    return 0;
  }

  if (!connected_out_block()) {
    // Assume only an in block is connected, in which case this entire transition length
    return length();
  }

  // Assume both are connected
  return length() + media_in();
}

rational TransitionBlock::out_offset() const
{
  // If no in block is connected, there's no in offset
  if (!connected_out_block()) {
    return 0;
  }

  if (!connected_in_block()) {
    // Assume only an in block is connected, in which case this entire transition length
    return length();
  }

  // Assume both are connected
  return -media_in();
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

  return clamp(1.0 - (GetInternalTransitionTime(time) / out_offset().toDouble()), 0.0, 1.0);
}

double TransitionBlock::GetInProgress(const double &time) const
{
  if (in_offset() == 0) {
    return 0;
  }

  return clamp((GetInternalTransitionTime(time) - out_offset().toDouble()) / in_offset().toDouble(), 0.0, 1.0);
}

void TransitionBlock::Hash(QCryptographicHash &hash, const rational &time) const
{
  Node::Hash(hash, time);

  double time_dbl = time.toDouble();
  double all_prog = GetTotalProgress(time_dbl);
  double in_prog = GetInProgress(time_dbl);
  double out_prog = GetOutProgress(time_dbl);

  hash.addData(reinterpret_cast<const char*>(&all_prog), sizeof(double));
  hash.addData(reinterpret_cast<const char*>(&in_prog), sizeof(double));
  hash.addData(reinterpret_cast<const char*>(&out_prog), sizeof(double));
}

double TransitionBlock::GetInternalTransitionTime(const double &time) const
{
  return time;
}

void TransitionBlock::InsertTransitionTimes(AcceleratedJob *job, const double &time) const
{
  // Provides total transition progress from 0.0 (start) - 1.0 (end)
  job->InsertValue(QStringLiteral("ove_tprog_all"),
                   NodeValue(NodeValue::kFloat, GetTotalProgress(time), this));

  // Provides progress of out section from 1.0 (start) - 0.0 (end)
  job->InsertValue(QStringLiteral("ove_tprog_out"),
                   NodeValue(NodeValue::kFloat, GetOutProgress(time), this));

  // Provides progress of in section from 0.0 (start) - 1.0 (end)
  job->InsertValue(QStringLiteral("ove_tprog_in"),
                   NodeValue(NodeValue::kFloat, GetInProgress(time), this));
}

void TransitionBlock::OutBlockConnected(Node *node)
{
  // If node is not a block, this will just be null
  if ((connected_out_block_ = dynamic_cast<Block*>(node))) {

    Q_ASSERT(connected_out_block_->type() != Block::kTransition
        && !connected_out_block_->out_transition()
        && connected_out_block_ == this->previous());

    connected_out_block_->set_out_transition(this);
  }
}

void TransitionBlock::OutBlockDisconnected()
{
  if (connected_out_block_) {
    connected_out_block_->set_in_transition(nullptr);
    connected_out_block_ = nullptr;
  }
}

void TransitionBlock::InBlockConnected(Node *node)
{
  // If node is not a block, this will just be null
  if ((connected_in_block_ = dynamic_cast<Block*>(node))) {

    Q_ASSERT(connected_in_block_->type() != Block::kTransition
        && !connected_in_block_->in_transition()
        && connected_in_block_ == this->next());

    connected_in_block_->set_in_transition(this);
  }
}

void TransitionBlock::InBlockDisconnected()
{
  if (connected_in_block_) {
    connected_in_block_->set_in_transition(nullptr);
    connected_in_block_ = nullptr;
  }
}

NodeValueTable TransitionBlock::Value(NodeValueDatabase &value) const
{
  NodeValue::Type data_type;

  if (out_block_input()->IsConnected()) {
    data_type = value[out_block_input()].GetWithMeta(NodeValue::kBuffer).type();
  } else if (in_block_input()->IsConnected()) {
    data_type = value[in_block_input()].GetWithMeta(NodeValue::kBuffer).type();
  } else {
    data_type = NodeValue::kNone;
  }

  NodeValue::Type job_type = NodeValue::kNone;
  QVariant push_job;

  if (data_type == NodeValue::kTexture) {
    // This must be a visual transition
    ShaderJob job;

    job.InsertValue(out_block_input(), value);
    job.InsertValue(in_block_input(), value);
    job.InsertValue(curve_input_, value);

    double time = value[QStringLiteral("global")].Get(NodeValue::kFloat, QStringLiteral("time_in")).toDouble();
    InsertTransitionTimes(&job, time);

    ShaderJobEvent(value, job);

    job_type = NodeValue::kShaderJob;
    push_job = QVariant::fromValue(job);
  } else if (data_type == NodeValue::kSamples) {
    // This must be an audio transition
    SampleBufferPtr from_samples = value[out_block_input()].Take(NodeValue::kSamples).value<SampleBufferPtr>();
    SampleBufferPtr to_samples = value[in_block_input()].Take(NodeValue::kSamples).value<SampleBufferPtr>();

    if (from_samples || to_samples) {
      double time_in = value[QStringLiteral("global")].Get(NodeValue::kFloat, QStringLiteral("time_in")).toDouble();
      double time_out = value[QStringLiteral("global")].Get(NodeValue::kFloat, QStringLiteral("time_out")).toDouble();

      const AudioParams& params = (from_samples) ? from_samples->audio_params() : to_samples->audio_params();

      int nb_samples = params.time_to_samples(time_out - time_in);

      SampleBufferPtr out_samples = SampleBuffer::CreateAllocated(params, nb_samples);
      SampleJobEvent(from_samples, to_samples, out_samples, time_in);

      job_type = NodeValue::kSamples;
      push_job = QVariant::fromValue(out_samples);
    }
  }

  NodeValueTable table = value.Merge();

  if (!push_job.isNull()) {
    table.Push(job_type, push_job, this);
  }

  return table;
}

void TransitionBlock::ShaderJobEvent(NodeValueDatabase &value, ShaderJob &job) const
{
  Q_UNUSED(value)
  Q_UNUSED(job)
}

void TransitionBlock::SampleJobEvent(SampleBufferPtr from_samples, SampleBufferPtr to_samples, SampleBufferPtr out_samples, double time_in) const
{
  Q_UNUSED(from_samples)
  Q_UNUSED(to_samples)
  Q_UNUSED(out_samples)
  Q_UNUSED(time_in)
}

double TransitionBlock::TransformCurve(double linear) const
{
  switch (static_cast<CurveType>(curve_input_->GetStandardValue().toInt())) {
  case kLinear:
    break;
  case kExponential:
    linear *= linear;
    break;
  case kLogarithmic:
    linear = qSqrt(linear);
    break;
  }

  return linear;
}

}
