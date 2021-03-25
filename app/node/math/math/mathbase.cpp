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

#include "math.h"

#include <QMatrix4x4>
#include <QVector2D>

#include "common/tohex.h"
#include "node/distort/transform/transformdistortnode.h"
#include "render/color.h"

namespace olive {

ShaderCode MathNodeBase::GetShaderCodeInternal(const QString &shader_id, const QString& param_a_in, const QString& param_b_in) const
{
  QStringList code_id = shader_id.split('.');

  Operation op = static_cast<Operation>(code_id.at(0).toInt());
  Pairing pairing = static_cast<Pairing>(code_id.at(1).toInt());
  NodeValue::Type type_a = static_cast<NodeValue::Type>(code_id.at(2).toInt());
  NodeValue::Type type_b = static_cast<NodeValue::Type>(code_id.at(3).toInt());

  QString operation, frag, vert;

  if (pairing == kPairTextureMatrix && op == kOpMultiply) {

    // Override the operation for this operation since we multiply texture COORDS by the matrix rather than
    const QString& tex_in = (type_a == NodeValue::kTexture) ? param_a_in : param_b_in;
    const QString& mat_in = (type_a == NodeValue::kTexture) ? param_b_in : param_a_in;

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
        if (NodeValue::type_is_numeric(type_a)) {
          operation = QStringLiteral("pow(%2, vec4(%1))");
        } else {
          operation = QStringLiteral("pow(%1, vec4(%2))");
        }
      } else {
        operation = QStringLiteral("pow(%1, %2)");
      }
      break;
    }

    operation = operation.arg(GetShaderVariableCall(param_a_in, type_a),
                              GetShaderVariableCall(param_b_in, type_b));
  }

  frag = QStringLiteral("uniform %1 %3;\n"
                        "uniform %2 %4;\n"
                        "\n"
                        "in vec2 ove_texcoord;\n"
                        "\n"
                        "out vec4 fragColor;\n"
                        "\n"
                        "void main(void) {\n"
                        "    fragColor = %5;\n"
                        "}\n").arg(GetShaderUniformType(type_a),
                                   GetShaderUniformType(type_b),
                                   param_a_in,
                                   param_b_in,
                                   operation);

  return ShaderCode(frag, vert);
}

QString MathNodeBase::GetShaderUniformType(const olive::NodeValue::Type &type)
{
  switch (type) {
  case NodeValue::kTexture:
    return QStringLiteral("sampler2D");
  case NodeValue::kColor:
    return QStringLiteral("vec4");
  case NodeValue::kMatrix:
    return QStringLiteral("mat4");
  default:
    return QStringLiteral("float");
  }
}

QString MathNodeBase::GetShaderVariableCall(const QString &input_id, const NodeValue::Type &type, const QString& coord_op)
{
  if (type == NodeValue::kTexture) {
    return QStringLiteral("texture(%1, ove_texcoord%2)").arg(input_id, coord_op);
  }

  return input_id;
}

QVector4D MathNodeBase::RetrieveVector(const NodeValue &val)
{
  // QVariant doesn't know that QVector*D can convert themselves so we do it here
  switch (val.type()) {
  case NodeValue::kVec2:
    return val.data().value<QVector2D>();
  case NodeValue::kVec3:
    return val.data().value<QVector3D>();
  case NodeValue::kVec4:
  default:
    return val.data().value<QVector4D>();
  }
}

void MathNodeBase::PushVector(NodeValueTable *output, olive::NodeValue::Type type, const QVector4D &vec) const
{
  switch (type) {
  case NodeValue::kVec2:
    output->Push(type, QVector2D(vec), this);
    break;
  case NodeValue::kVec3:
    output->Push(type, QVector3D(vec), this);
    break;
  case NodeValue::kVec4:
    output->Push(type, vec, this);
    break;
  default:
    break;
  }
}

NodeValueTable MathNodeBase::ValueInternal(NodeValueDatabase &value, Operation operation, Pairing pairing, const QString& param_a_in, const NodeValue& val_a, const QString& param_b_in, const NodeValue& val_b) const
{
  NodeValueTable output = value.Merge();

  switch (pairing) {

  case kPairNumberNumber:
  {
    if (val_a.type() == NodeValue::kRational && val_b.type() == NodeValue::kRational && operation != kOpPower) {
      // Preserve rationals
      output.Push(NodeValue::kRational,
                  QVariant::fromValue(PerformAddSubMultDiv<rational, rational>(operation, val_a.data().value<rational>(), val_b.data().value<rational>())),
                  this);
    } else {
      output.Push(NodeValue::kFloat,
                  PerformAll<float, float>(operation, RetrieveNumber(val_a), RetrieveNumber(val_b)),
                  this);
    }
    break;
  }

  case kPairVecVec:
  {
    // We convert all vectors to QVector4D just for simplicity and exploit the fact that kVec4 is higher than kVec2 in
    // the enum to find the largest data type
    PushVector(&output,
               qMax(val_a.type(), val_b.type()),
               PerformAddSubMultDiv<QVector4D, QVector4D>(operation, RetrieveVector(val_a), RetrieveVector(val_b)));
    break;
  }

  case kPairMatrixVec:
  {
    QMatrix4x4 matrix = (val_a.type() == NodeValue::kMatrix) ? val_a.data().value<QMatrix4x4>() : val_b.data().value<QMatrix4x4>();
    QVector4D vec = (val_a.type() == NodeValue::kMatrix) ? RetrieveVector(val_b) : RetrieveVector(val_a);

    // Only valid operation is multiply
    PushVector(&output,
               qMax(val_a.type(), val_b.type()),
               PerformMult<QVector4D, QMatrix4x4>(operation, vec, matrix));
    break;
  }

  case kPairVecNumber:
  {
    QVector4D vec = (NodeValue::type_is_vector(val_a.type()) ? RetrieveVector(val_a) : RetrieveVector(val_b));
    float number = RetrieveNumber((val_a.type() & NodeValue::kMatrix) ? val_b : val_a);

    // Only multiply and divide are valid operations
    PushVector(&output, val_a.type(), PerformMultDiv<QVector4D, float>(operation, vec, number));
    break;
  }

  case kPairMatrixMatrix:
  {
    QMatrix4x4 mat_a = val_a.data().value<QMatrix4x4>();
    QMatrix4x4 mat_b = val_b.data().value<QMatrix4x4>();
    output.Push(NodeValue::kMatrix, PerformAddSubMult<QMatrix4x4, QMatrix4x4>(operation, mat_a, mat_b), this);
    break;
  }

  case kPairColorColor:
  {
    Color col_a = val_a.data().value<Color>();
    Color col_b = val_b.data().value<Color>();

    // Only add and subtract are valid operations
    output.Push(NodeValue::kColor, QVariant::fromValue(PerformAddSub<Color, Color>(operation, col_a, col_b)), this);
    break;
  }


  case kPairNumberColor:
  {
    Color col = (val_a.type() == NodeValue::kColor) ? val_a.data().value<Color>() : val_b.data().value<Color>();
    float num = (val_a.type() == NodeValue::kColor) ? val_b.data().toFloat() : val_a.data().toFloat();

    // Only multiply and divide are valid operations
    output.Push(NodeValue::kColor, QVariant::fromValue(PerformMult<Color, float>(operation, col, num)), this);
    break;
  }

  case kPairSampleSample:
  {
    SampleBufferPtr samples_a = val_a.data().value<SampleBufferPtr>();
    SampleBufferPtr samples_b = val_b.data().value<SampleBufferPtr>();

    int max_samples = qMax(samples_a->sample_count(), samples_b->sample_count());
    int min_samples = qMin(samples_a->sample_count(), samples_b->sample_count());

    SampleBufferPtr mixed_samples = SampleBuffer::CreateAllocated(samples_a->audio_params(), max_samples);

    for (int i=0;i<mixed_samples->audio_params().channel_count();i++) {
      // Mix samples that are in both buffers
      for (int j=0;j<min_samples;j++) {
        mixed_samples->data(i)[j] = PerformAll<float, float>(operation, samples_a->data(i)[j], samples_b->data(i)[j]);
      }
    }

    if (max_samples > min_samples) {
      // Fill in remainder space with 0s
      int remainder = max_samples - min_samples;

      SampleBufferPtr larger_buffer = (max_samples == samples_a->sample_count()) ? samples_a : samples_b;

      for (int i=0;i<mixed_samples->audio_params().channel_count();i++) {
        memcpy(&mixed_samples->data(i)[min_samples],
               &larger_buffer->data(i)[min_samples],
               remainder * sizeof(float));
      }
    }

    output.Push(NodeValue::kSamples, QVariant::fromValue(mixed_samples), this);
    break;
  }

  case kPairTextureColor:
  case kPairTextureNumber:
  case kPairTextureTexture:
  case kPairTextureMatrix:
  {
    ShaderJob job;
    job.SetShaderID(QStringLiteral("%1.%2.%3.%4").arg(QString::number(operation),
                                                      QString::number(pairing),
                                                      QString::number(val_a.type()),
                                                      QString::number(val_b.type())));

    job.InsertValue(param_a_in, val_a);
    job.InsertValue(param_b_in, val_b);

    bool operation_is_noop = false;

    const NodeValue& number_val = val_a.type() == NodeValue::kTexture ? val_b : val_a;
    const NodeValue& texture_val = val_a.type() == NodeValue::kTexture ? val_a : val_b;
    TexturePtr texture = texture_val.data().value<TexturePtr>();

    if (!texture) {
      operation_is_noop = true;
    } else if (pairing == kPairTextureNumber) {
      if (NumberIsNoOp(operation, RetrieveNumber(number_val))) {
        operation_is_noop = true;
      }
    } else if (pairing == kPairTextureMatrix) {
      // Only allow matrix multiplication
      QVector2D sequence_res = value[QStringLiteral("global")].Get(NodeValue::kVec2, QStringLiteral("resolution")).value<QVector2D>();
      QVector2D texture_res(texture->params().width() * texture->pixel_aspect_ratio().toDouble(), texture->params().height());

      QMatrix4x4 adjusted_matrix = TransformDistortNode::AdjustMatrixByResolutions(number_val.data().value<QMatrix4x4>(),
                                                                                   sequence_res,
                                                                                   texture_res);

      if (operation != kOpMultiply || adjusted_matrix.isIdentity()) {
        operation_is_noop = true;
      } else {
        // Replace with adjusted matrix
        job.InsertValue(val_a.type() == NodeValue::kTexture ? param_b_in : param_a_in,
                        NodeValue(NodeValue::kMatrix, adjusted_matrix, this));

        // It's likely an alpha channel will result from this operation
        job.SetAlphaChannelRequired(true);
      }
    }

    if (operation_is_noop) {
      // Just push texture as-is
      output.Push(texture_val);
    } else {
      // Push shader job
      output.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    }
    break;
  }

  case kPairSampleNumber:
  {
    // Queue a sample job
    const NodeValue& number_val = val_a.type() == NodeValue::kSamples ? val_b : val_a;
    const QString& number_param = val_a.type() == NodeValue::kSamples ? param_b_in : param_a_in;

    float number = RetrieveNumber(number_val);

    SampleJob job(val_a.type() == NodeValue::kSamples ? val_a : val_b);
    job.InsertValue(number_param, NodeValue(NodeValue::kFloat, number, this));

    if (job.HasSamples()) {
      if (IsInputStatic(number_param)) {
        if (!NumberIsNoOp(operation, number)) {
          for (int i=0;i<job.samples()->audio_params().channel_count();i++) {
            for (int j=0;j<job.samples()->sample_count();j++) {
              job.samples()->data(i)[j] = PerformAll(operation, job.samples()->data(i)[j], number);
            }
          }
        }

        output.Push(NodeValue::kSamples, QVariant::fromValue(job.samples()), this);
      } else {
        output.Push(NodeValue::kSampleJob, QVariant::fromValue(job), this);
      }
    }
    break;
  }

  case kPairNone:
  case kPairCount:
    break;
  }

  return output;
}

void MathNodeBase::ProcessSamplesInternal(NodeValueDatabase &values, MathNodeBase::Operation operation, const QString &param_a_in, const QString &param_b_in, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  // This function is only used for sample+number pairing
  NodeValue number_val = values[param_a_in].GetWithMeta(NodeValue::kNumber);

  if (number_val.type() == NodeValue::kNone) {
    number_val = values[param_b_in].GetWithMeta(NodeValue::kNumber);

    if (number_val.type() == NodeValue::kNone) {
      return;
    }
  }

  float number_flt = RetrieveNumber(number_val);

  for (int i=0;i<output->audio_params().channel_count();i++) {
    output->data(i)[index] = PerformAll<float, float>(operation, input->data(i)[index], number_flt);
  }
}

float MathNodeBase::RetrieveNumber(const NodeValue &val)
{
  if (val.type() == NodeValue::kRational) {
    return val.data().value<rational>().toDouble();
  } else {
    return val.data().toFloat();
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

MathNodeBase::PairingCalculator::PairingCalculator(const NodeValueTable &table_a, const NodeValueTable &table_b)
{
  QVector<int> pair_likelihood_a = GetPairLikelihood(table_a);
  QVector<int> pair_likelihood_b = GetPairLikelihood(table_b);

  int weight_a = qMax(0, table_b.Count() - table_a.Count());
  int weight_b = qMax(0, table_a.Count() - table_b.Count());

  QVector<int> likelihoods(kPairCount);

  for (int i=0;i<kPairCount;i++) {
    if (pair_likelihood_a.at(i) == -1 || pair_likelihood_b.at(i) == -1) {
      likelihoods.replace(i, -1);
    } else {
      likelihoods.replace(i, pair_likelihood_a.at(i) + weight_a + pair_likelihood_b.at(i) + weight_b);
    }
  }

  most_likely_pairing_ = kPairNone;

  for (int i=0;i<likelihoods.size();i++) {
    if (likelihoods.at(i) > -1) {
      if (most_likely_pairing_ == kPairNone
          || likelihoods.at(i) > likelihoods.at(most_likely_pairing_)) {
        most_likely_pairing_ = static_cast<Pairing>(i);
      }
    }
  }

  if (most_likely_pairing_ != kPairNone) {
    most_likely_value_a_ = table_a.at(pair_likelihood_a.at(most_likely_pairing_));
    most_likely_value_b_ = table_b.at(pair_likelihood_b.at(most_likely_pairing_));
  }
}

QVector<int> MathNodeBase::PairingCalculator::GetPairLikelihood(const NodeValueTable &table)
{
  // FIXME: When we introduce a manual override, placing it here would be the least problematic

  QVector<int> likelihood(kPairCount, -1);

  for (int i=0;i<table.Count();i++) {
    NodeValue::Type type = table.at(i).type();

    int weight = i;

    if (NodeValue::type_is_vector(type)) {
      likelihood.replace(kPairVecVec, weight);
      likelihood.replace(kPairVecNumber, weight);
      likelihood.replace(kPairMatrixVec, weight);
    } else if (type == NodeValue::kMatrix) {
      likelihood.replace(kPairMatrixMatrix, weight);
      likelihood.replace(kPairMatrixVec, weight);
      likelihood.replace(kPairTextureMatrix, weight);
    } else if (type == NodeValue::kColor) {
      likelihood.replace(kPairColorColor, weight);
      likelihood.replace(kPairNumberColor, weight);
      likelihood.replace(kPairTextureColor, weight);
    } else if (NodeValue::type_is_numeric(type)) {
      likelihood.replace(kPairNumberNumber, weight);
      likelihood.replace(kPairVecNumber, weight);
      likelihood.replace(kPairNumberColor, weight);
      likelihood.replace(kPairTextureNumber, weight);
      likelihood.replace(kPairSampleNumber, weight);
    } else if (type == NodeValue::kSamples) {
      likelihood.replace(kPairSampleSample, weight);
      likelihood.replace(kPairSampleNumber, weight);
    } else if (type == NodeValue::kTexture) {
      likelihood.replace(kPairTextureTexture, weight);
      likelihood.replace(kPairTextureNumber, weight);
      likelihood.replace(kPairTextureColor, weight);
      likelihood.replace(kPairTextureMatrix, weight);
    }
  }

  return likelihood;
}

bool MathNodeBase::PairingCalculator::FoundMostLikelyPairing() const
{
  return (most_likely_pairing_ > kPairNone && most_likely_pairing_ < kPairCount);
}

MathNodeBase::Pairing MathNodeBase::PairingCalculator::GetMostLikelyPairing() const
{
  return most_likely_pairing_;
}

const NodeValue &MathNodeBase::PairingCalculator::GetMostLikelyValueA() const
{
  return most_likely_value_a_;
}

const NodeValue &MathNodeBase::PairingCalculator::GetMostLikelyValueB() const
{
  return most_likely_value_b_;
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
    return qPow(a, b);
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
