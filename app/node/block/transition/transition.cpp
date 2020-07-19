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

#include "transition.h"

#include "common/clamp.h"

OLIVE_NAMESPACE_ENTER

TransitionBlock::TransitionBlock() :
  connected_out_block_(nullptr),
  connected_in_block_(nullptr)
{
  out_block_input_ = new NodeInput(QStringLiteral("out_block_in"), NodeParam::kBuffer);
  out_block_input_->set_is_keyframable(false);
  connect(out_block_input_, &NodeParam::EdgeAdded, this, &TransitionBlock::BlockConnected);
  connect(out_block_input_, &NodeParam::EdgeRemoved, this, &TransitionBlock::BlockDisconnected);
  AddInput(out_block_input_);

  in_block_input_ = new NodeInput(QStringLiteral("in_block_in"), NodeParam::kBuffer);
  in_block_input_->set_is_keyframable(false);
  connect(in_block_input_, &NodeParam::EdgeAdded, this, &TransitionBlock::BlockConnected);
  connect(in_block_input_, &NodeParam::EdgeRemoved, this, &TransitionBlock::BlockDisconnected);
  AddInput(in_block_input_);

  curve_input_ = new NodeInput(QStringLiteral("curve_in"), NodeParam::kCombo);
  curve_input_->set_is_keyframable(false);
  curve_input_->set_connectable(false);
  AddInput(curve_input_);
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
  return time - in().toDouble();
}

void TransitionBlock::InsertTransitionTimes(AcceleratedJob *job, const double &time) const
{
  // Provides total transition progress from 0.0 (start) - 1.0 (end)
  job->InsertValue(QStringLiteral("ove_tprog_all"),
                   NodeValue(NodeParam::kFloat, GetTotalProgress(time), this));

  // Provides progress of out section from 1.0 (start) - 0.0 (end)
  job->InsertValue(QStringLiteral("ove_tprog_out"),
                   NodeValue(NodeParam::kFloat, GetOutProgress(time), this));

  // Provides progress of in section from 0.0 (start) - 1.0 (end)
  job->InsertValue(QStringLiteral("ove_tprog_in"),
                   NodeValue(NodeParam::kFloat, GetInProgress(time), this));
}

void TransitionBlock::BlockConnected(NodeEdgePtr edge)
{
  if (!edge->output()->parentNode()->IsBlock()) {
    return;
  }

  Block* block = static_cast<Block*>(edge->output()->parentNode());

  if (edge->input() == out_block_input_) {
    connected_out_block_ = block;
  } else {
    connected_in_block_ = block;
  }
}

void TransitionBlock::BlockDisconnected(NodeEdgePtr edge)
{
  if (edge->input() == out_block_input_) {
    connected_out_block_ = nullptr;
  } else {
    connected_in_block_ = nullptr;
  }
}

NodeValueTable TransitionBlock::Value(NodeValueDatabase &value) const
{
  NodeParam::DataType data_type;

  if (out_block_input()->is_connected()) {
    data_type = value[out_block_input()].GetWithMeta(NodeParam::kBuffer).type();
  } else if (in_block_input()->is_connected()) {
    data_type = value[in_block_input()].GetWithMeta(NodeParam::kBuffer).type();
  } else {
    data_type = NodeParam::kNone;
  }

  NodeParam::DataType job_type;
  QVariant push_job;

  if (data_type == NodeParam::kTexture) {
    // This must be a visual transition
    ShaderJob job;

    job.InsertValue(out_block_input(), value);
    job.InsertValue(in_block_input(), value);
    job.InsertValue(curve_input_, value);

    double time = value[QStringLiteral("global")].Get(NodeParam::kFloat, QStringLiteral("time_in")).toDouble();
    InsertTransitionTimes(&job, time);

    ShaderJobEvent(value, job);

    job_type = NodeParam::kShaderJob;
    push_job = QVariant::fromValue(job);
  } else if (data_type == NodeParam::kSamples) {
    // This must be an audio transition
    SampleBufferPtr from_samples = value[out_block_input()].Take(NodeParam::kBuffer).value<SampleBufferPtr>();
    SampleBufferPtr to_samples = value[in_block_input()].Take(NodeParam::kBuffer).value<SampleBufferPtr>();

    if (from_samples || to_samples) {
      double time_in = value[QStringLiteral("global")].Get(NodeParam::kFloat, QStringLiteral("time_in")).toDouble();
      double time_out = value[QStringLiteral("global")].Get(NodeParam::kFloat, QStringLiteral("time_out")).toDouble();

      const AudioParams& params = (from_samples) ? from_samples->audio_params() : to_samples->audio_params();

      int nb_samples = params.time_to_samples(time_out - time_in);

      SampleBufferPtr out_samples = SampleBuffer::CreateAllocated(params, nb_samples);
      SampleJobEvent(from_samples, to_samples, out_samples, time_in);

      job_type = NodeParam::kSamples;
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
  switch (static_cast<CurveType>(curve_input_->get_standard_value().toInt())) {
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

OLIVE_NAMESPACE_EXIT
