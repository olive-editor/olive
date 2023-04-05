/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "math.h"

#include "mathfunctions.h"

namespace olive {

const QString MathNode::kMethodIn = QStringLiteral("method_in");
const QString MathNode::kParamAIn = QStringLiteral("param_a_in");
const QString MathNode::kParamBIn = QStringLiteral("param_b_in");
const QString MathNode::kParamCIn = QStringLiteral("param_c_in");

#define super Node

std::map<MathNode::Operation, std::map< type_t, std::map<type_t, MathNode::operation_t> > > MathNode::operations_;

MathNode::MathNode()
{
  AddInput(kMethodIn, TYPE_COMBO, kInputFlagNotConnectable | kInputFlagNotKeyframable);

  AddInput(kParamAIn, TYPE_DOUBLE, 0.0);
  SetInputProperty(kParamAIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamAIn, QStringLiteral("autotrim"), true);

  AddInput(kParamBIn, TYPE_DOUBLE, 0.0);
  SetInputProperty(kParamBIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamBIn, QStringLiteral("autotrim"), true);

  if (operations_.empty()) {
    PopulateOperations();
  }
}

QString MathNode::Name() const
{
  // Default to naming after the operation
  if (parent()) {
    QString name = GetOperationName(static_cast<Operation>(GetStandardValue(kMethodIn).toInt()));
    if (!name.isEmpty()) {
      return name;
    }
  }

  return tr("Math");
}

QString MathNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.math");
}

QVector<Node::CategoryID> MathNode::Category() const
{
  return {kCategoryMath};
}

QString MathNode::Description() const
{
  return tr("Perform a mathematical operation between two values.");
}

void MathNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kMethodIn, tr("Method"));
  SetInputName(kParamAIn, tr("Value"));
  SetInputName(kParamBIn, tr("Value"));

  QStringList operations = {GetOperationName(kOpAdd),
                            GetOperationName(kOpSubtract),
                            GetOperationName(kOpMultiply),
                            GetOperationName(kOpDivide),
                            QString(),
                            GetOperationName(kOpPower)};

  SetComboBoxStrings(kMethodIn, operations);
}

void ProcessSamplesSamples(const void *context, const SampleJob &job, SampleBuffer &mixed_samples)
{
  const SampleBuffer samples_a = job.Get(QStringLiteral("a")).toSamples();
  const SampleBuffer samples_b = job.Get(QStringLiteral("b")).toSamples();
  const MathNode::Operation operation = static_cast<MathNode::Operation>(job.Get(QStringLiteral("operation")).toInt());

  size_t max_samples = qMax(samples_a.sample_count(), samples_b.sample_count());
  size_t min_samples = qMin(samples_a.sample_count(), samples_b.sample_count());

  for (int i=0;i<mixed_samples.audio_params().channel_count();i++) {
    // Mix samples that are in both buffers
    for (size_t j=0;j<min_samples;j++) {
      mixed_samples.data(i)[j] = PerformAll<float, float>(operation, samples_a.data(i)[j], samples_b.data(i)[j]);
    }
  }

  if (max_samples > min_samples) {
    size_t remainder = max_samples - min_samples;

    const SampleBuffer &larger_buffer = (max_samples == samples_a.sample_count()) ? samples_a : samples_b;

    for (int i=0;i<mixed_samples.audio_params().channel_count();i++) {
      memcpy(&mixed_samples.data(i)[min_samples],
             &larger_buffer.data(i)[min_samples],
             remainder * sizeof(float));
    }
  }
}

value_t MathNode::Value(const ValueParams &p) const
{
  // Auto-detect what values to operate with
  auto aval = GetInputValue(p, kParamAIn);
  auto bval = GetInputValue(p, kParamBIn);

  if (!bval.isValid()) {
    return aval;
  }
  if (!aval.isValid()) {
    return bval;
  }

  Operation operation = static_cast<Operation>(GetInputValue(p, kMethodIn).toInt());

  if (aval.type() == TYPE_SAMPLES || bval.type() == TYPE_SAMPLES) {
    if (aval.type() == TYPE_SAMPLES && bval.type() == TYPE_SAMPLES) {
      SampleJob job(p);

      job.Insert(QStringLiteral("a"), aval);
      job.Insert(QStringLiteral("b"), bval);
      job.Insert(QStringLiteral("operation"), int(operation));

      job.set_function(ProcessSamplesSamples, this);

      return job;
    }


  } else if (aval.type() == TYPE_TEXTURE || bval.type() == TYPE_TEXTURE) {
    // FIXME: Requires a more complex shader job
  } else {
    // Operation can be done entirely here
    operation_t func = operations_[operation][aval.type()][bval.type()];
    if (func) {
      return func(aval, bval);
    }
  }

  return aval;
}

QString MathNode::GetOperationName(Operation o)
{
  switch (o) {
  case kOpAdd: return tr("Add");
  case kOpSubtract: return tr("Subtract");
  case kOpMultiply: return tr("Multiply");
  case kOpDivide: return tr("Divide");
  case kOpPower: return tr("Power");
  }

  return QString();
}

void MathNode::PopulateOperations()
{
  operations_[kOpAdd][TYPE_INTEGER][TYPE_INTEGER] = Math::AddIntegerInteger;

  operations_[kOpAdd][TYPE_DOUBLE][TYPE_DOUBLE] = Math::AddDoubleDouble;
  operations_[kOpSubtract][TYPE_DOUBLE][TYPE_DOUBLE] = Math::SubtractDoubleDouble;
  operations_[kOpMultiply][TYPE_DOUBLE][TYPE_DOUBLE] = Math::MultiplyDoubleDouble;
  operations_[kOpDivide][TYPE_DOUBLE][TYPE_DOUBLE] = Math::DivideDoubleDouble;
  operations_[kOpPower][TYPE_DOUBLE][TYPE_DOUBLE] = Math::PowerDoubleDouble;

  operations_[kOpAdd][TYPE_MATRIX][TYPE_MATRIX] = Math::AddMatrixMatrix;
  operations_[kOpSubtract][TYPE_MATRIX][TYPE_MATRIX] = Math::SubtractMatrixMatrix;
  operations_[kOpMultiply][TYPE_MATRIX][TYPE_MATRIX] = Math::MultiplyMatrixMatrix;
}

}
