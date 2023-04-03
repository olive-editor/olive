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

#include "math.h"

#include <QMatrix4x4>
#include <QVector2D>

#include "common/tohex.h"
#include "node/distort/transform/transformdistortnode.h"

namespace olive {

static const QString param_a = QStringLiteral("param_a");
static const QString param_b = QStringLiteral("param_b");

ShaderCode MathNodeBase::GetShaderCode(const QString &shader_id)
{
  QStringList code_id = shader_id.split('.');

  Operation op = static_cast<Operation>(code_id.at(0).toInt());
  Pairing pairing = static_cast<Pairing>(code_id.at(1).toInt());
  type_t type_a = code_id.at(2).toUtf8().constData();
  type_t type_b = code_id.at(3).toUtf8().constData();
  size_t size_a = code_id.at(4).toULongLong();
  size_t size_b = code_id.at(5).toULongLong();

  QString operation, frag, vert;

  if (pairing == kPairTextureMatrix && op == kOpMultiply) {

    // Override the operation for this operation since we multiply texture COORDS by the matrix rather than
    const QString& tex_in = (type_a == TYPE_TEXTURE) ? param_a : param_b;
    const QString& mat_in = (type_a == TYPE_TEXTURE) ? param_b : param_a;

    // No-op frag shader (can we return QString() instead?)
    operation = QStringLiteral("texture(%1, ove_texcoord)").arg(tex_in);

    vert = QStringLiteral("uniform mat4 %1;\n"
                          "\n"
                          "in vec4 a_position;\n"
                          "in vec2 a_texcoord;\n"
                          "\n"
                          "out vec2 ove_texcoord;\n"
                          "\n"
                          "void main() {\n"
                          "    gl_Position = %1 * a_position;\n"
                          "    ove_texcoord = a_texcoord;\n"
                          "}\n").arg(mat_in);

  } else {
    switch (op) {
    case kOpAdd:
      operation = QStringLiteral("%1 + %2");
      break;
    case kOpSubtract:
      operation = QStringLiteral("%1 - %2");
      break;
    case kOpMultiply:
      operation = QStringLiteral("%1 * %2");
      break;
    case kOpDivide:
      operation = QStringLiteral("%1 / %2");
      break;
    case kOpPower:
      if (pairing == kPairTextureNumber) {
        // The "number" in this operation has to be declared a vec4
        if (type_a == TYPE_INTEGER || type_a == TYPE_DOUBLE || type_a == TYPE_RATIONAL) {
          operation = QStringLiteral("pow(%2, vec4(%1))");
        } else {
          operation = QStringLiteral("pow(%1, vec4(%2))");
        }
      } else {
        operation = QStringLiteral("pow(%1, %2)");
      }
      break;
    }

    operation = operation.arg(GetShaderVariableCall(param_a, type_a),
                              GetShaderVariableCall(param_b, type_b));
  }

  frag = QStringLiteral("uniform %1 %3;\n"
                        "uniform %2 %4;\n"
                        "\n"
                        "in vec2 ove_texcoord;\n"
                        "out vec4 frag_color;\n"
                        "\n"
                        "void main(void) {\n"
                        "    vec4 c = %5;\n"
                        "    c.a = clamp(c.a, 0.0, 1.0);\n" // Ensure alpha is between 0.0 and 1.0
                        "    frag_color = c;\n"
                        "}\n").arg(GetShaderUniformType(type_a, size_a),
                                   GetShaderUniformType(type_b, size_b),
                                   param_a,
                                   param_b,
                                   operation);

  return ShaderCode(frag, vert);
}

QString MathNodeBase::GetShaderUniformType(const olive::type_t &type, size_t channels)
{
  if (type == TYPE_TEXTURE) {
    return QStringLiteral("sampler2D");
  } else if (type == TYPE_MATRIX) {
    return QStringLiteral("mat4");
  } else if (type == TYPE_DOUBLE) {
    switch (channels) {
    case 1: return QStringLiteral("float");
    case 2: return QStringLiteral("vec2");
    case 3: return QStringLiteral("vec3");
    case 4: return QStringLiteral("vec4");
    }
  }

  // Fallback (shouldn't ever get here)
  return QString();
}

QString MathNodeBase::GetShaderVariableCall(const QString &input_id, const type_t &type, const QString& coord_op)
{
  if (type == TYPE_TEXTURE) {
    return QStringLiteral("texture(%1, ove_texcoord%2)").arg(input_id, coord_op);
  }

  return input_id;
}

QVector4D MathNodeBase::RetrieveVector(const value_t &val)
{
  return val.toVec4();
}

value_t MathNodeBase::PushVector(size_t channels, const QVector4D &vec) const
{
  switch (channels) {
  case 2: return vec.toVector2D();
  case 3: return vec.toVector3D();
  case 4: return vec;
  }

  return value_t();
}

QString MathNodeBase::GetOperationName(Operation o)
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

void MathNodeBase::PerformAllOnFloatBuffer(Operation operation, const float *input, float *output, float b, size_t start, size_t end)
{
  for (size_t j=start;j<end;j++) {
    output[j] = PerformAll(operation, input[j], b);
  }
}

#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
void MathNodeBase::PerformAllOnFloatBufferSSE(Operation operation, const float *input, float *output, float b, size_t start, size_t end)
{
  size_t end_divisible_4 = (end / 4) * 4;

  // Load number to multiply by into buffer
  __m128 mult = _mm_load1_ps(&b);

  switch (operation) {
  case kOpAdd:
    // Loop all values
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_add_ps(_mm_loadu_ps(input + start + j), mult));
    }
    break;
  case kOpSubtract:
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_sub_ps(_mm_loadu_ps(input + start + j), mult));
    }
    break;
  case kOpMultiply:
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_mul_ps(_mm_loadu_ps(input + start + j), mult));
    }
    break;
  case kOpDivide:
    for(size_t j = 0; j < end_divisible_4; j+=4) {
      _mm_storeu_ps(output + start + j, _mm_div_ps(_mm_loadu_ps(input + start + j), mult));
    }
    break;
  case kOpPower:
    // Fallback for operations we can't support here
    end_divisible_4 = 0;
    break;
  }

  // Handle last 1-3 bytes if necessary, or all bytes if we couldn't
  // support this op on SSE
  PerformAllOnFloatBuffer(operation, input, output, b, end_divisible_4, end);
}
#endif

void MathNodeBase::ProcessSamplesNumber(const void *context, const SampleJob &job, SampleBuffer &output)
{
  const MathNodeBase *n = static_cast<const MathNodeBase *>(context);

  const ValueParams &p = job.value_params();
  const SampleBuffer input = job.Get(QStringLiteral("samples")).toSamples();
  const QString number_in = job.Get(QStringLiteral("number")).toString();
  const Operation operation = static_cast<Operation>(job.Get(QStringLiteral("operation")).toInt());

  if (n->IsInputStatic(number_in)) {
    auto f = n->GetStandardValue(number_in).toDouble();

    for (int i=0;i<output.audio_params().channel_count();i++) {
#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
      // Use SSE instructions for optimization
      PerformAllOnFloatBufferSSE(operation, input.data(i), output.data(i), f, 0, output.sample_count());
#else
      PerformAllOnFloatBuffer(operation, input.data(i), output.data(i), f, 0, output.sample_count());
#endif
    }
  } else {
    for (size_t j = 0; j < input.sample_count(); j++) {
      rational this_sample_time = p.time().in() + rational(j, input.audio_params().sample_rate());
      TimeRange this_sample_range(this_sample_time, this_sample_time + input.audio_params().sample_rate_as_time_base());
      auto v = n->GetInputValue(p.time_transformed(this_sample_range), number_in).toDouble();

      for (int i=0;i<output.audio_params().channel_count();i++) {
        output.data(i)[j] = PerformAll<float, float>(operation, input.data(i)[j], v);
      }
    }
  }
}

value_t MathNodeBase::ValueInternal(Operation operation, Pairing pairing, const QString& param_a_in, const value_t& val_a, const QString& param_b_in, const value_t& val_b, const ValueParams &p) const
{
  /*
  switch (pairing) {

  case kPairNumberNumber:
  {
    if (val_a.type() == TYPE_RATIONAL && val_b.type() == TYPE_RATIONAL && operation != kOpPower) {
      // Preserve rationals
      return PerformAddSubMultDiv<rational, rational>(operation, val_a.toRational(), val_b.toRational());
    } else {
      return PerformAll<float, float>(operation, RetrieveNumber(val_a), RetrieveNumber(val_b));
    }
  }

  case kPairVecVec:
  {
    // We convert all vectors to QVector4D just for simplicity and exploit the fact that kVec4 is higher than kVec2 in
    // the enum to find the largest data type
    return PushVector(qMax(val_a.type(), val_b.type()),
                      PerformAddSubMultDiv<QVector4D, QVector4D>(operation, RetrieveVector(val_a), RetrieveVector(val_b)));
  }

  case kPairMatrixVec:
  {
    QMatrix4x4 matrix = (val_a.type() == TYPE_MATRIX) ? val_a.toMatrix() : val_b.toMatrix();
    QVector4D vec = (val_a.type() == TYPE_MATRIX) ? RetrieveVector(val_b) : RetrieveVector(val_a);

    // Only valid operation is multiply
    return PushVector(qMax(val_a.type(), val_b.type()),
                      PerformMult<QVector4D, QMatrix4x4>(operation, vec, matrix));
  }

  case kPairVecNumber:
  {
    QVector4D vec = (NodeValue::type_is_vector(val_a.type()) ? RetrieveVector(val_a) : RetrieveVector(val_b));
    float number = RetrieveNumber((val_a.type() & TYPE_MATRIX) ? val_b : val_a);

    // Only multiply and divide are valid operations
    return PushVector(val_a.type(), PerformMultDiv<QVector4D, float>(operation, vec, number));
  }

  case kPairMatrixMatrix:
  {
    QMatrix4x4 mat_a = val_a.toMatrix();
    QMatrix4x4 mat_b = val_b.toMatrix();
    return PerformAddSubMult<QMatrix4x4, QMatrix4x4>(operation, mat_a, mat_b);
  }

  case kPairColorColor:
  {
    Color col_a = val_a.toColor();
    Color col_b = val_b.toColor();

    // Only add and subtract are valid operations
    return PerformAddSub<Color, Color>(operation, col_a, col_b);
  }


  case kPairNumberColor:
  {
    Color col = (val_a.type() == NodeValue::kColor) ? val_a.toColor() : val_b.toColor();
    float num = (val_a.type() == NodeValue::kColor) ? val_b.toDouble() : val_a.toDouble();

    // Only multiply and divide are valid operations
    return PerformMult<Color, float>(operation, col, num);
  }

  case kPairSampleSample:
  {
    SampleJob job(p);

    job.Insert(QStringLiteral("a"), val_a);
    job.Insert(QStringLiteral("b"), val_b);
    job.Insert(QStringLiteral("operation"), int(operation));

    job.set_function(MathNodeBase::ProcessSamplesSamples, this);

    return job;
  }

  case kPairTextureColor:
  case kPairTextureNumber:
  case kPairTextureTexture:
  case kPairTextureMatrix:
  {
    ShaderJob job;
    job.set_function(MathNodeBase::GetShaderCode);
    job.SetShaderID(QStringLiteral("%1.%2.%3.%4.%5.%6").arg(QString::number(operation),
                                                            QString::number(pairing),
                                                            val_a.type(),
                                                            val_b.type(),
                                                            QString::number(val_a.size()),
                                                            QString::number(val_b.size())));

    job.Insert(param_a, val_a);
    job.Insert(param_b, val_b);

    bool operation_is_noop = false;

    const Value& number_val = val_a.type() == NodeValue::TYPE_TEXTURE ? val_b : val_a;
    const Value& texture_val = val_a.type() == NodeValue::TYPE_TEXTURE ? val_a : val_b;
    TexturePtr texture = texture_val.toTexture();

    if (!texture) {
      operation_is_noop = true;
    } else if (pairing == kPairTextureNumber) {
      if (NumberIsNoOp(operation, RetrieveNumber(number_val))) {
        operation_is_noop = true;
      }
    } else if (pairing == kPairTextureMatrix) {
      // Only allow matrix multiplication
      const QVector2D &sequence_res = p.nonsquare_resolution();
      QVector2D texture_res(texture->params().width() * texture->pixel_aspect_ratio().toDouble(), texture->params().height());

      QMatrix4x4 adjusted_matrix = TransformDistortNode::AdjustMatrixByResolutions(number_val.toMatrix(),
                                                                                   sequence_res,
                                                                                   texture->params().offset(),
                                                                                   texture_res);

      if (operation != kOpMultiply || adjusted_matrix.isIdentity()) {
        operation_is_noop = true;
      } else {
        // Replace with adjusted matrix
        job.Insert(val_a.type() == NodeValue::kTexture ? param_b_in : param_a_in, adjusted_matrix);
      }
    }

    if (operation_is_noop) {
      // Just push texture as-is
      return texture_val;
    } else {
      // Push shader job
      return Texture::Job(p.vparams(), job);
    }
    break;
  }

  case kPairSampleNumber:
  {
    // Queue a sample job
    const Value& number_val = val_a.type() == NodeValue::kSamples ? val_b : val_a;
    const QString& number_param = val_a.type() == NodeValue::kSamples ? param_b_in : param_a_in;

    float number = RetrieveNumber(number_val);

    const Value &sample_val = val_a.type() == NodeValue::kSamples ? val_a : val_b;

    if (IsInputStatic(number_param) && NumberIsNoOp(operation, number)) {
      return sample_val;
    } else {
      SampleJob job(p);

      job.Insert(QStringLiteral("samples"), sample_val);
      job.Insert(QStringLiteral("number"), val_a.type() == NodeValue::kSamples ? param_b_in : param_a_in);
      job.Insert(QStringLiteral("operation"), int(operation));

      job.set_function(MathNodeBase::ProcessSamplesNumber, this);

      return job;
    }
    break;
  }

  case kPairNone:
  case kPairCount:
    break;
  }

  return Value();
  */
  return value_t();
}

void MathNodeBase::ProcessSamplesSamples(const void *context, const SampleJob &job, SampleBuffer &mixed_samples)
{
  const SampleBuffer samples_a = job.Get(QStringLiteral("a")).toSamples();
  const SampleBuffer samples_b = job.Get(QStringLiteral("b")).toSamples();
  const Operation operation = static_cast<Operation>(job.Get(QStringLiteral("operation")).toInt());

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

float MathNodeBase::RetrieveNumber(const value_t &val)
{
  if (val.type() == TYPE_RATIONAL) {
    return val.toRational().toDouble();
  } else {
    return val.toDouble();
  }
}

bool MathNodeBase::NumberIsNoOp(const MathNodeBase::Operation &op, const float &number)
{
  switch (op) {
  case kOpAdd:
  case kOpSubtract:
    if (qIsNull(number)) {
      return true;
    }
    break;
  case kOpMultiply:
  case kOpDivide:
  case kOpPower:
    if (qFuzzyCompare(number, 1.0f)) {
      return true;
    }
    break;
  }

  return false;
}

template<typename T, typename U>
T MathNodeBase::PerformAll(Operation operation, T a, U b)
{
  switch (operation) {
  case kOpAdd:
    return a + b;
  case kOpSubtract:
    return a - b;
  case kOpMultiply:
    return a * b;
  case kOpDivide:
    return a / b;
  case kOpPower:
    return std::pow(a, b);
  }

  return a;
}

template<typename T, typename U>
T MathNodeBase::PerformMultDiv(Operation operation, T a, U b)
{
  switch (operation) {
  case kOpMultiply:
    return a * b;
  case kOpDivide:
    return a / b;
  case kOpAdd:
  case kOpSubtract:
  case kOpPower:
    break;
  }

  return a;
}

template<typename T, typename U>
T MathNodeBase::PerformAddSub(Operation operation, T a, U b)
{
  switch (operation) {
  case kOpAdd:
    return a + b;
  case kOpSubtract:
    return a - b;
  case kOpMultiply:
  case kOpDivide:
  case kOpPower:
    break;
  }

  return a;
}

template<typename T, typename U>
T MathNodeBase::PerformMult(Operation operation, T a, U b)
{
  switch (operation) {
  case kOpMultiply:
    return a * b;
  case kOpAdd:
  case kOpSubtract:
  case kOpDivide:
  case kOpPower:
    break;
  }

  return a;
}

template<typename T, typename U>
T MathNodeBase::PerformAddSubMult(Operation operation, T a, U b)
{
  switch (operation) {
  case kOpAdd:
    return a + b;
  case kOpSubtract:
    return a - b;
  case kOpMultiply:
    return a * b;
  case kOpDivide:
  case kOpPower:
    break;
  }

  return a;
}

template<typename T, typename U>
T MathNodeBase::PerformAddSubMultDiv(Operation operation, T a, U b)
{
  switch (operation) {
  case kOpAdd:
    return a + b;
  case kOpSubtract:
    return a - b;
  case kOpMultiply:
    return a * b;
  case kOpDivide:
    return a / b;
  case kOpPower:
    break;
  }

  return a;
}

}
