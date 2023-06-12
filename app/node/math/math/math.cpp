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

std::map<MathNode::Operation, std::map< type_t, std::map<type_t, std::map<type_t, MathNode::operation_t>> > > MathNode::operations_;

MathNode::MathNode()
{
  AddInput(kMethodIn, TYPE_COMBO, kInputFlagNotConnectable | kInputFlagNotKeyframable);

  AddInput(kParamAIn, TYPE_DOUBLE, 0.0, kInputFlagDontAutoConvert);
  SetInputProperty(kParamAIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamAIn, QStringLiteral("autotrim"), true);

  AddInput(kParamBIn, TYPE_DOUBLE, 0.0, kInputFlagDontAutoConvert);
  SetInputProperty(kParamBIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamBIn, QStringLiteral("autotrim"), true);

  AddInput(kParamCIn, TYPE_DOUBLE, 0.0, kInputFlagDontAutoConvert);
  SetInputProperty(kParamCIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamCIn, QStringLiteral("autotrim"), true);

  if (operations_.empty()) {
    PopulateOperations();
  }

  UpdateInputVisibility();
}

QString MathNode::Name() const
{
  // Default to naming after the operation
  if (parent()) {
    QString name = GetOperationName(GetOperation());
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
  if (GetOperation() == kOpClamp) {
    SetInputName(kParamBIn, tr("Minimum"));
    SetInputName(kParamCIn, tr("Maximum"));
  } else {
    SetInputName(kParamBIn, tr("Value"));
    SetInputName(kParamCIn, tr("Value"));
  }

  QStringList operations = {
    GetOperationName(kOpAdd),
    GetOperationName(kOpSubtract),
    GetOperationName(kOpMultiply),
    GetOperationName(kOpDivide),
    QString(),
    GetOperationName(kOpPower),
    QString(),
    GetOperationName(kOpSine),
    GetOperationName(kOpCosine),
    GetOperationName(kOpTangent),
    QString(),
    GetOperationName(kOpArcSine),
    GetOperationName(kOpArcCosine),
    GetOperationName(kOpArcTangent),
    QString(),
    GetOperationName(kOpHypSine),
    GetOperationName(kOpHypCosine),
    GetOperationName(kOpHypTangent),
    QString(),
    GetOperationName(kOpMin),
    GetOperationName(kOpMax),
    GetOperationName(kOpClamp),
    QString(),
    GetOperationName(kOpFloor),
    GetOperationName(kOpCeil),
    GetOperationName(kOpRound),
    GetOperationName(kOpAbs),
  };

  SetComboBoxStrings(kMethodIn, operations);
}

void MathNode::ProcessSamplesSamples(const void *context, const SampleJob &job, SampleBuffer &mixed_samples)
{
  const SampleBuffer samples_a = job.Get(QStringLiteral("a")).toSamples();
  const SampleBuffer samples_b = job.Get(QStringLiteral("b")).toSamples();
  const MathNode::Operation operation = static_cast<MathNode::Operation>(job.Get(QStringLiteral("operation")).toInt());

  if (!samples_a.is_allocated() || !samples_b.is_allocated()) {
    return;
  }

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
  case kOpMin:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_min_ps(_mm_loadu_ps(input + start + j), mult));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = std::min(input[start + j], b);
    }
    break;
  case kOpMax:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_max_ps(_mm_loadu_ps(input + start + j), mult));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = std::max(input[start + j], b);
    }
    break;

  // These operations use either more or less than 2 operands and are thus invalid for this function
  case kOpSine:
  case kOpCosine:
  case kOpTangent:
  case kOpArcSine:
  case kOpArcCosine:
  case kOpArcTangent:
  case kOpHypSine:
  case kOpHypCosine:
  case kOpHypTangent:
  case kOpClamp:
  case kOpFloor:
  case kOpCeil:
  case kOpRound:
  case kOpAbs:
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
  case kOpMin:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_min_ps(_mm_loadu_ps(input + start + j), _mm_loadu_ps(input2 + start + j)));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = std::min(input[start + j], input2[start + j]);
    }
    break;
  case kOpMax:
#ifdef USE_SSE
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_max_ps(_mm_loadu_ps(input + start + j), _mm_loadu_ps(input2 + start + j)));
    }
#endif
    for (size_t j = end_divisible_4; j < end; j++) {
      output[start + j] = std::max(input[start + j], input2[start + j]);
    }
    break;

    // These operations use either more or less than 2 operands and are thus invalid for this function
  case kOpSine:
  case kOpCosine:
  case kOpTangent:
  case kOpArcSine:
  case kOpArcCosine:
  case kOpArcTangent:
  case kOpHypSine:
  case kOpHypCosine:
  case kOpHypTangent:
  case kOpClamp:
  case kOpFloor:
  case kOpCeil:
  case kOpRound:
  case kOpAbs:
    break;
  }
}

int MathNode::GetNumberOfOperands(Operation o)
{
  switch (o) {
  case kOpSine:
  case kOpCosine:
  case kOpTangent:
  case kOpArcSine:
  case kOpArcCosine:
  case kOpArcTangent:
  case kOpHypSine:
  case kOpHypCosine:
  case kOpHypTangent:
  case kOpFloor:
  case kOpCeil:
  case kOpRound:
  case kOpAbs:
    return 1;
  case kOpAdd:
  case kOpSubtract:
  case kOpMultiply:
  case kOpDivide:
  case kOpPower:
  case kOpMin:
  case kOpMax:
    return 2;
  case kOpClamp:
    return 3;
  }

  return 0;
}

MathNode::Operation MathNode::GetOperation() const
{
  return static_cast<Operation>(GetStandardValue(kMethodIn).toInt());
}

void MathNode::UpdateInputVisibility()
{
  int count = GetNumberOfOperands(GetOperation());

  SetInputFlag(kParamAIn, kInputFlagHidden, 0 >= count);
  SetInputFlag(kParamBIn, kInputFlagHidden, 1 >= count);
  SetInputFlag(kParamCIn, kInputFlagHidden, 2 >= count);

  Retranslate();
}

void MathNode::ProcessSamplesDouble(const void *context, const SampleJob &job, SampleBuffer &output)
{
  const Node *n = static_cast<const Node *>(context);

  const ValueParams &p = job.value_params();
  const SampleBuffer input = job.Get(QStringLiteral("samples")).toSamples();
  const QString number_in = job.Get(QStringLiteral("number")).toString();
  const Operation operation = static_cast<Operation>(job.Get(QStringLiteral("operation")).toInt());

  if (!input.is_allocated()) {
    return;
  }

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

void MathNode::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kMethodIn) {
    UpdateInputVisibility();
  }

  super::InputValueChangedEvent(input, element);
}

QString GetShaderUniformType(const type_t &type)
{
  if (type == TYPE_TEXTURE) {
    return QStringLiteral("sampler2D");
  } else {
    return QStringLiteral("float");
  }
}

QString GetShaderVariableCall(const QString &input_id, const type_t &type, const QString& coord_op = QString())
{
  if (type == TYPE_TEXTURE) {
    return QStringLiteral("texture(%1, ove_texcoord%2)").arg(input_id, coord_op);
  }

  return input_id;
}

ShaderCode MathNode::GetShaderCode(const QString &id)
{
  QStringList j = id.split(':');
  if (j.size() != 3) {
    return ShaderCode();
  }

  Operation op = static_cast<Operation>(j.at(0).toInt());
  type_t atype = type_t::fromString(j.at(1));
  type_t btype = type_t::fromString(j.at(2));
  type_t ctype = type_t::fromString(j.at(3));

  QString line;

  switch (op) {
  case kOpAdd: line = QStringLiteral("%1 + %2"); break;
  case kOpSubtract: line = QStringLiteral("%1 - %2"); break;
  case kOpMultiply: line = QStringLiteral("%1 * %2"); break;
  case kOpDivide: line = QStringLiteral("%1 / %2"); break;
  case kOpPower:
  case kOpMin:
  case kOpMax:
    if (op == kOpPower) {
      line = QStringLiteral("pow");
    } else if (op == kOpMin) {
      line = QStringLiteral("min");
    } else if (op == kOpMax) {
      line = QStringLiteral("max");
    }

    if (atype == TYPE_DOUBLE) {
      // The "number" in this operation has to be declared a vec4
      line.append("(%2, vec4(%1))");
    } else if (btype == TYPE_DOUBLE) {
      line.append("(%1, vec4(%2))");
    } else {
      line.append("(%1, %2)");
    }
    break;
  case kOpSine: line = QStringLiteral("sin(%1)"); break;
  case kOpCosine: line = QStringLiteral("cos(%1)"); break;
  case kOpTangent: line = QStringLiteral("tan(%1)"); break;
  case kOpArcSine: line = QStringLiteral("asin(%1)"); break;
  case kOpArcCosine: line = QStringLiteral("acos(%1)"); break;
  case kOpArcTangent: line = QStringLiteral("atan(%1)"); break;
  case kOpHypSine: line = QStringLiteral("sinh(%1)"); break;
  case kOpHypCosine: line = QStringLiteral("cosh(%1)"); break;
  case kOpHypTangent: line = QStringLiteral("tanh(%1)"); break;
  case kOpFloor: line = QStringLiteral("floor(%1)"); break;
  case kOpCeil: line = QStringLiteral("ceil(%1)"); break;
  case kOpRound: line = QStringLiteral("round(%1)"); break;
  case kOpAbs: line = QStringLiteral("abs(%1)"); break;
  case kOpClamp: line = QStringLiteral("clamp(%1, %2, %3)"); break;
    break;
  }

  line = line.arg(GetShaderVariableCall(kParamAIn, atype),
                  GetShaderVariableCall(kParamBIn, btype),
                  GetShaderVariableCall(kParamCIn, ctype));

  static const QString stub =
      QStringLiteral("uniform %1 %4;\n"
                     "uniform %2 %5;\n"
                     "uniform %3 %6;\n"
                     "\n"
                     "in vec2 ove_texcoord;\n"
                     "out vec4 frag_color;\n"
                     "\n"
                     "void main(void) {\n"
                     "    vec4 c = %7;\n"
                     "    c.a = clamp(c.a, 0.0, 1.0);\n" // Ensure alpha is between 0.0 and 1.0
                     "    frag_color = c;\n"
                     "}\n");

  return stub.arg(GetShaderUniformType(atype),
                  GetShaderUniformType(btype),
                  GetShaderUniformType(ctype),
                  kParamAIn,
                  kParamBIn,
                  kParamCIn,
                  line);
}

void NormalizeNumber(value_t &in)
{
  if (in.type() == TYPE_RATIONAL || in.type() == TYPE_INTEGER) {
    in = in.converted(TYPE_DOUBLE);
  }
}

value_t MathNode::Value(const ValueParams &p) const
{
  // Auto-detect what values to operate with
  auto aval = GetInputValue(p, kParamAIn);
  auto bval = GetInputValue(p, kParamBIn);
  auto cval = GetInputValue(p, kParamCIn);

  Operation operation = static_cast<Operation>(GetInputValue(p, kMethodIn).toInt());
  int count = GetNumberOfOperands(operation);

  // Null values if we aren't listening so the function-lookup is more reliable
  if (count < 1) { aval = value_t(); }
  if (count < 2) { bval = value_t(); }
  if (count < 3) { cval = value_t(); }

  // Treat integers and rationals as doubles
  NormalizeNumber(aval);
  NormalizeNumber(bval);
  NormalizeNumber(cval);

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
    // Produce shader job
    ShaderJob job = CreateShaderJob(p, GetShaderCode);
    job.SetShaderID(QStringLiteral("%1:%2:%3:%4").arg(QString::number(operation), aval.type().toString(), bval.type().toString(), cval.type().toString()));
    return Texture::Job(aval.type() == TYPE_TEXTURE ? aval.toTexture()->params() : bval.toTexture()->params(), job);
  } else {
    // Operation can be done entirely here
    operation_t func = operations_[operation][aval.type()][bval.type()][cval.type()];
    if (func) {
      return func(aval, bval, cval);
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
  case kOpSine: return tr("Sine");
  case kOpCosine: return tr("Cosine");
  case kOpTangent: return tr("Tangent");
  case kOpArcSine: return tr("Inverse Sine");
  case kOpArcCosine: return tr("Inverse Cosine");
  case kOpArcTangent: return tr("Inverse Tangent");
  case kOpHypSine: return tr("Hyperbolic Sine");
  case kOpHypCosine: return tr("Hyperbolic Cosine");
  case kOpHypTangent: return tr("Hyperbolic Tangent");
  case kOpMin: return tr("Minimum");
  case kOpMax: return tr("Maximum");
  case kOpClamp: return tr("Clamp");
  case kOpFloor: return tr("Floor");
  case kOpCeil: return tr("Ceil");
  case kOpRound: return tr("Round");
  case kOpAbs: return tr("Absolute");
  }

  return QString();
}

void MathNode::PopulateOperations()
{
  operations_[kOpAdd][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::AddDoubleDouble;
  operations_[kOpSubtract][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::SubtractDoubleDouble;
  operations_[kOpMultiply][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::MultiplyDoubleDouble;
  operations_[kOpDivide][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::DivideDoubleDouble;
  operations_[kOpPower][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::PowerDoubleDouble;

  operations_[kOpSine][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::SineDouble;
  operations_[kOpCosine][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::CosineDouble;
  operations_[kOpTangent][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::TangentDouble;

  operations_[kOpArcSine][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::ArcSineDouble;
  operations_[kOpArcCosine][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::ArcCosineDouble;
  operations_[kOpArcTangent][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::ArcTangentDouble;

  operations_[kOpHypSine][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::HypSineDouble;
  operations_[kOpHypCosine][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::HypCosineDouble;
  operations_[kOpHypTangent][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::HypTangentDouble;

  operations_[kOpMin][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::MinDoubleDouble;
  operations_[kOpMax][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_NONE] = Math::MaxDoubleDouble;
  operations_[kOpClamp][TYPE_DOUBLE][TYPE_DOUBLE][TYPE_DOUBLE] = Math::ClampDoubleDoubleDouble;

  operations_[kOpFloor][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::FloorDouble;
  operations_[kOpCeil][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::CeilDouble;
  operations_[kOpRound][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::RoundDouble;
  operations_[kOpAbs][TYPE_DOUBLE][TYPE_NONE][TYPE_NONE] = Math::AbsDouble;

  operations_[kOpAdd][TYPE_MATRIX][TYPE_MATRIX][TYPE_NONE] = Math::AddMatrixMatrix;
  operations_[kOpSubtract][TYPE_MATRIX][TYPE_MATRIX][TYPE_NONE] = Math::SubtractMatrixMatrix;
  operations_[kOpMultiply][TYPE_MATRIX][TYPE_MATRIX][TYPE_NONE] = Math::MultiplyMatrixMatrix;
}

}
