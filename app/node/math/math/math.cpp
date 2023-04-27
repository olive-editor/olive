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

void MathNode::ProcessSamplesSamples(const void *context, const SampleJob &job, SampleBuffer &mixed_samples)
{
  const SampleBuffer samples_a = job.Get(QStringLiteral("a")).toSamples();
  const SampleBuffer samples_b = job.Get(QStringLiteral("b")).toSamples();
  const MathNode::Operation operation = static_cast<MathNode::Operation>(job.Get(QStringLiteral("operation")).toInt());

  size_t max_samples = qMax(samples_a.sample_count(), samples_b.sample_count());
  size_t min_samples = qMin(samples_a.sample_count(), samples_b.sample_count());

  for (int i=0;i<mixed_samples.audio_params().channel_count();i++) {
    OperateSampleSample(operation, samples_a.data(i), mixed_samples.data(i), samples_b.data(i), 0, std::min(samples_a.sample_count(), samples_b.sample_count()));
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

#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
#define USE_SSE
#endif

void MathNode::OperateSampleNumber(Operation operation, const float *input, float *output, float b, size_t start, size_t end)
{
  size_t end_divisible_4 =
#ifdef USE_SSE
      (end / 4) * 4
#else
      0
#endif
  ;

  // Load number to multiply by into buffer
  __m128 mult = _mm_load1_ps(&b);

  switch (operation) {
  case kOpAdd:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_add_ps(_mm_loadu_ps(input + start + j), mult));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] + b;
    }
    break;
  case kOpSubtract:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_sub_ps(_mm_loadu_ps(input + start + j), mult));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] - b;
    }
    break;
  case kOpMultiply:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_mul_ps(_mm_loadu_ps(input + start + j), mult));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] * b;
    }
    break;
  case kOpDivide:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_div_ps(_mm_loadu_ps(input + start + j), mult));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] / b;
    }
    break;
  case kOpPower:
    // Can't do pow in SSE, do it manually here instead
    end_divisible_4 = 0;
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = std::pow(input[start + j], b);
    }
    break;
  }
}

void MathNode::OperateSampleSample(Operation operation, const float *input, float *output, const float *input2, size_t start, size_t end)
{
  size_t end_divisible_4 =
#ifdef USE_SSE
      (end / 4) * 4
#else
      0
#endif
  ;

  switch (operation) {
  case kOpAdd:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_add_ps(_mm_loadu_ps(input + start + j), _mm_loadu_ps(input2 + start + j)));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] + input2[start + j];
    }
    break;
  case kOpSubtract:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_sub_ps(_mm_loadu_ps(input + start + j), _mm_loadu_ps(input2 + start + j)));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] - input2[start + j];
    }
    break;
  case kOpMultiply:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_mul_ps(_mm_loadu_ps(input + start + j), _mm_loadu_ps(input2 + start + j)));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] * input2[start + j];
    }
    break;
  case kOpDivide:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_div_ps(_mm_loadu_ps(input + start + j), _mm_loadu_ps(input2 + start + j)));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = input[start + j] / input2[start + j];
    }
    break;
  case kOpPower:
    // Can't do pow in SSE, do it manually here instead
    end_divisible_4 = 0;
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = std::pow(input[start + j], input2[start + j]);
    }
    break;
  }
}

void MathNode::ProcessSamplesDouble(const void *context, const SampleJob &job, SampleBuffer &output)
{
  const Node *n = static_cast<const Node *>(context);

  const ValueParams &p = job.value_params();
  const SampleBuffer input = job.Get(QStringLiteral("samples")).toSamples();
  const QString number_in = job.Get(QStringLiteral("number")).toString();
  const Operation operation = static_cast<Operation>(job.Get(QStringLiteral("operation")).toInt());

  if (n->IsInputStatic(number_in)) {
    auto f = n->GetStandardValue(number_in).toDouble();

    for (int i=0;i<output.audio_params().channel_count();i++) {
      OperateSampleNumber(operation, input.data(i), output.data(i), f, 0, output.sample_count());
    }
  } else {
    std::vector<float> values(input.sample_count());

    for (size_t i = 0; i < values.size(); i++) {
      rational this_sample_time = p.time().in() + rational(i, input.audio_params().sample_rate());
      TimeRange this_sample_range(this_sample_time, this_sample_time + input.audio_params().sample_rate_as_time_base());
      auto v = n->GetInputValue(p.time_transformed(this_sample_range), number_in).toDouble();
      values[i] = v;
    }

    for (int i=0;i<output.audio_params().channel_count();i++) {
      OperateSampleSample(operation, input.data(i), output.data(i), values.data(), 0, input.sample_count());
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
    } else if (aval.type() == TYPE_DOUBLE || bval.type() == TYPE_DOUBLE) {
      SampleJob job(p);

      job.Insert(QStringLiteral("samples"), aval.type() == TYPE_SAMPLES ? aval : bval);
      job.Insert(QStringLiteral("number"), aval.type() == TYPE_SAMPLES ? kParamBIn : kParamAIn);
      job.Insert(QStringLiteral("operation"), int(operation));

      job.set_function(ProcessSamplesDouble, this);

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
