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

#include "math.h"

#include <QMatrix4x4>
#include <QVector2D>

#include "common/tohex.h"
#include "render/color.h"

OLIVE_NAMESPACE_ENTER

MathNode::MathNode()
{
  method_in_ = new NodeInput(QStringLiteral("method_in"), NodeParam::kCombo);
  method_in_->SetConnectable(false);
  method_in_->set_is_keyframable(false);
  AddInput(method_in_);

  param_a_in_ = new NodeInput(QStringLiteral("param_a_in"), NodeParam::kFloat);
  param_a_in_->set_property(QStringLiteral("decimalplaces"), 8);
  param_a_in_->set_property(QStringLiteral("autotrim"), true);
  AddInput(param_a_in_);

  param_b_in_ = new NodeInput(QStringLiteral("param_b_in"), NodeParam::kFloat);
  param_b_in_->set_property(QStringLiteral("decimalplaces"), 8);
  param_b_in_->set_property(QStringLiteral("autotrim"), true);
  AddInput(param_b_in_);
}

Node *MathNode::copy() const
{
  return new MathNode();
}

QString MathNode::Name() const
{
  return tr("Math");
}

QString MathNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.math");
}

QString MathNode::Category() const
{
  return tr("Math");
}

QString MathNode::Description() const
{
  return tr("Perform a mathematical operation between two values.");
}

void MathNode::Retranslate()
{
  Node::Retranslate();

  method_in_->set_name(tr("Method"));
  param_a_in_->set_name(tr("Value"));
  param_b_in_->set_name(tr("Value"));

  QStringList operations = {tr("Add"),
                            tr("Subtract"),
                            tr("Multiply"),
                            tr("Divide"),
                            QString(),
                            tr("Power")};

  method_in_->set_combobox_strings(operations);
}

Node::Capabilities MathNode::GetCapabilities(const NodeValueDatabase &input) const
{
  PairingCalculator calc(input[param_a_in_], input[param_b_in_]);

  switch (calc.GetMostLikelyPairing()) {
  case kPairTextureColor:
  case kPairTextureNumber:
  case kPairTextureTexture:
  case kPairTextureMatrix:
    return kShader;
  case kPairSampleNumber:
    return kSampleProcessor;
  default:
    return kNormal;
  }
}

QString MathNode::ShaderID(const NodeValueDatabase &input) const
{
  QString method = QString::number(GetOperation());

  PairingCalculator calc(input[param_a_in_], input[param_b_in_]);

  QString type_a = QString::number(calc.GetMostLikelyValueA().type());
  QString type_b = QString::number(calc.GetMostLikelyValueB().type());

  return id().append(method).append(type_a).append(type_b);
}

QString MathNode::ShaderFragmentCode(const NodeValueDatabase &input) const
{
  PairingCalculator calc(input[param_a_in_], input[param_b_in_]);

  NodeParam::DataType type_a = calc.GetMostLikelyValueA().type();
  NodeParam::DataType type_b = calc.GetMostLikelyValueB().type();

  QString operation;

  if (calc.GetMostLikelyPairing() == kPairTextureMatrix && GetOperation() == kOpMultiply) {

    // Override the operation for this operation since we multiply texture COORDS by the matrix rather than
    NodeParam* tex_in = (type_a == NodeParam::kTexture) ? param_a_in_ : param_b_in_;
    NodeParam* mat_in = (type_a == NodeParam::kTexture) ? param_b_in_ : param_a_in_;

    operation = QStringLiteral("texture2D(%1, (vec4(ove_texcoord, 0.0, 1.0) * %2).xy)").arg(tex_in->id(), mat_in->id());

  } else {
    switch (GetOperation()) {
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
      operation = QStringLiteral("pow(%1, %2)");
      break;
    }

    operation = operation.arg(GetShaderVariableCall(param_a_in_->id(), type_a),
                              GetShaderVariableCall(param_b_in_->id(), type_b));
  }

  return QStringLiteral("#version 110\n"
                        "\n"
                        "varying vec2 ove_texcoord;\n"
                        "\n"
                        "uniform %1 %3;\n"
                        "uniform %2 %4;\n"
                        "\n"
                        "void main(void) {\n"
                        "  gl_FragColor = %5;\n"
                        "}\n").arg(GetShaderUniformType(type_a),
                                   GetShaderUniformType(type_b),
                                   param_a_in_->id(),
                                   param_b_in_->id(),
                                   operation);
}

NodeValue MathNode::InputValueFromTable(NodeInput *input, NodeValueDatabase &db, bool take) const
{
  if (input == param_a_in_ || input == param_b_in_) {
    PairingCalculator calc(db[param_a_in_], db[param_b_in_]);

    NodeValue v = (input == param_a_in_)
        ? calc.GetMostLikelyValueA()
        : calc.GetMostLikelyValueB();

    if (take) {
      db[input].Remove(v);
    }

    return v;
  }

  return Node::InputValueFromTable(input, db, take);
}

NodeValueTable MathNode::Value(NodeValueDatabase &value) const
{
  // Auto-detect what values to operate with
  // FIXME: Add manual override for this
  PairingCalculator calc(value[param_a_in_], value[param_b_in_]);

  if (!calc.FoundMostLikelyPairing()
      || calc.GetMostLikelyPairing() == kPairSampleNumber
      || calc.GetMostLikelyPairing() == kPairTextureTexture
      || calc.GetMostLikelyPairing() == kPairTextureNumber
      || calc.GetMostLikelyPairing() == kPairTextureColor
      || calc.GetMostLikelyPairing() == kPairTextureMatrix) {
    return value.Merge();
  }

  NodeValue val_a = calc.GetMostLikelyValueA();
  value[param_a_in_].Remove(val_a);

  NodeValue val_b = calc.GetMostLikelyValueB();
  value[param_b_in_].Remove(val_b);

  NodeValueTable output = value.Merge();

  switch (calc.GetMostLikelyPairing()) {

  case kPairNumberNumber:
  {
    if (val_a.type() == NodeParam::kRational && val_b.type() == NodeParam::kRational && GetOperation() != kOpPower) {
      // Preserve rationals
      output.Push(NodeParam::kRational,
                  QVariant::fromValue(PerformAddSubMultDiv<rational, rational>(val_a.data().value<rational>(), val_b.data().value<rational>())));
    } else {
      output.Push(NodeParam::kFloat,
                  PerformAll<float, float>(RetrieveNumber(val_a), RetrieveNumber(val_b)));
    }
    break;
  }

  case kPairVecVec:
  {
    // We convert all vectors to QVector4D just for simplicity and exploit the fact that kVec4 is higher than kVec2 in
    // the enum to find the largest data type
    PushVector(&output,
               qMax(val_a.type(), val_b.type()),
               PerformAddSubMultDiv<QVector4D, QVector4D>(RetrieveVector(val_a), RetrieveVector(val_b)));
    break;
  }

  case kPairMatrixVec:
  {
    QMatrix4x4 matrix = (val_a.type() == NodeParam::kMatrix) ? val_a.data().value<QMatrix4x4>() : val_b.data().value<QMatrix4x4>();
    QVector4D vec = (val_a.type() == NodeParam::kMatrix) ? RetrieveVector(val_b) : RetrieveVector(val_a);

    // Only valid operation is multiply
    PushVector(&output,
               qMax(val_a.type(), val_b.type()),
               PerformMult<QVector4D, QMatrix4x4>(vec, matrix));
    break;
  }

  case kPairVecNumber:
  {
    QVector4D vec = (val_a.type() & NodeParam::kVector) ? RetrieveVector(val_a) : RetrieveVector(val_b);
    float number = RetrieveNumber((val_a.type() & NodeParam::kMatrix) ? val_b : val_a);

    // Only multiply and divide are valid operations
    PushVector(&output, val_a.type(), PerformMultDiv<QVector4D, float>(vec, number));
    break;
  }

  case kPairMatrixMatrix:
  {
    QMatrix4x4 mat_a = val_a.data().value<QMatrix4x4>();
    QMatrix4x4 mat_b = val_b.data().value<QMatrix4x4>();
    output.Push(NodeParam::kMatrix, PerformAddSubMult<QMatrix4x4, QMatrix4x4>(mat_a, mat_b));
    break;
  }

  case kPairColorColor:
  {
    Color col_a = val_a.data().value<Color>();
    Color col_b = val_b.data().value<Color>();

    // Only add and subtract are valid operations
    output.Push(NodeParam::kColor, QVariant::fromValue(PerformAddSub<Color, Color>(col_a, col_b)));
    break;
  }


  case kPairNumberColor:
  {
    Color col = (val_a.type() == NodeParam::kColor) ? val_a.data().value<Color>() : val_b.data().value<Color>();
    float num = (val_a.type() == NodeParam::kColor) ? val_b.data().toFloat() : val_a.data().toFloat();

    // Only multiply and divide are valid operations
    output.Push(NodeParam::kColor, QVariant::fromValue(PerformMult<Color, float>(col, num)));
    break;
  }

  case kPairSampleSample:
  {
    SampleBufferPtr samples_a = val_a.data().value<SampleBufferPtr>();
    SampleBufferPtr samples_b = val_b.data().value<SampleBufferPtr>();

    int max_samples = qMax(samples_a->sample_count_per_channel(), samples_b->sample_count_per_channel());
    int min_samples = qMin(samples_a->sample_count_per_channel(), samples_b->sample_count_per_channel());

    SampleBufferPtr mixed_samples = SampleBuffer::CreateAllocated(samples_a->audio_params(), max_samples);

    // Mix samples that are in both buffers
    for (int i=0;i<mixed_samples->audio_params().channel_count();i++) {
      for (int j=0;j<min_samples;j++) {
        mixed_samples->data()[i][j] = PerformAll<float, float>(samples_a->data()[i][j], samples_b->data()[i][j]);
      }
    }

    if (max_samples > min_samples) {
      // Fill in remainder space with 0s
      int remainder = max_samples - min_samples;

      for (int i=0;i<mixed_samples->audio_params().channel_count();i++) {
        memset(mixed_samples->data()[i] + min_samples * sizeof(float),
               0,
               remainder * sizeof(float));
      }
    }

    output.Push(NodeParam::kSamples, QVariant::fromValue(mixed_samples));
    break;
  }

  case kPairNone:
  case kPairCount:
  case kPairTextureColor:
  case kPairTextureNumber:
  case kPairTextureTexture:
  case kPairTextureMatrix:
  case kPairSampleNumber:
    // Do nothing
    break;
  }

  return output;
}

NodeInput *MathNode::ProcessesSamplesFrom(const NodeValueDatabase &value) const
{
  PairingCalculator calc(value[param_a_in_], value[param_b_in_]);

  if (calc.GetMostLikelyPairing() == kPairSampleNumber) {
    if (calc.GetMostLikelyValueA().type() == NodeParam::kSamples) {
      return param_a_in_;
    } else {
      return param_b_in_;
    }
  }

  return nullptr;
}

void MathNode::ProcessSamples(const NodeValueDatabase &values, const AudioRenderingParams &params, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  // This function is only used for sample+number pairing
  NodeInput* number_input = (ProcessesSamplesFrom(values) == param_a_in_) ? param_b_in_ : param_a_in_;
  NodeValue number_val = values[number_input].GetWithMeta(NodeParam::kNumber);
  float number_flt = RetrieveNumber(number_val);

  for (int i=0;i<params.channel_count();i++) {
    output->data()[i][index] = PerformAll<float, float>(input->data()[i][index], number_flt);
  }
}

NodeInput *MathNode::param_a_in() const
{
  return param_a_in_;
}

NodeInput *MathNode::param_b_in() const
{
  return param_b_in_;
}

MathNode::Operation MathNode::GetOperation() const
{
  return static_cast<Operation>(method_in_->get_standard_value().toInt());
}

QString MathNode::GetShaderUniformType(const NodeParam::DataType &type)
{
  switch (type) {
  case NodeParam::kTexture:
    return QStringLiteral("sampler2D");
  case NodeParam::kColor:
    return QStringLiteral("vec4");
  case NodeParam::kMatrix:
    return QStringLiteral("mat4");
  default:
    return QStringLiteral("float");
  }
}

QString MathNode::GetShaderVariableCall(const QString &input_id, const NodeParam::DataType &type, const QString& coord_op)
{
  if (type == NodeParam::kTexture) {
    return QStringLiteral("texture2D(%1, ove_texcoord%2)").arg(input_id, coord_op);
  }

  return input_id;
}

QVector<int> MathNode::PairingCalculator::GetPairLikelihood(const NodeValueTable &table)
{
  // FIXME: When we introduce a manual override, placing it here would be the least problematic

  QVector<int> likelihood(kPairCount, -1);

  for (int i=0;i<table.Count();i++) {
    NodeParam::DataType type = table.At(i).type();

    int weight = i;

    if (type & NodeParam::kVector) {
      likelihood.replace(kPairVecVec, weight);
      likelihood.replace(kPairVecNumber, weight);
      likelihood.replace(kPairMatrixVec, weight);
    } else if (type & NodeParam::kMatrix) {
      likelihood.replace(kPairMatrixMatrix, weight);
      likelihood.replace(kPairMatrixVec, weight);
      likelihood.replace(kPairTextureMatrix, weight);
    } else if (type & NodeParam::kColor) {
      likelihood.replace(kPairColorColor, weight);
      likelihood.replace(kPairNumberColor, weight);
      likelihood.replace(kPairTextureColor, weight);
    } else if (type & NodeParam::kNumber) {
      likelihood.replace(kPairNumberNumber, weight);
      likelihood.replace(kPairVecNumber, weight);
      likelihood.replace(kPairNumberColor, weight);
      likelihood.replace(kPairTextureNumber, weight);
      likelihood.replace(kPairSampleNumber, weight);
    } else if (type & NodeParam::kSamples) {
      likelihood.replace(kPairSampleSample, weight);
      likelihood.replace(kPairSampleNumber, weight);
    } else if (type & NodeParam::kTexture) {
      likelihood.replace(kPairTextureTexture, weight);
      likelihood.replace(kPairTextureNumber, weight);
      likelihood.replace(kPairTextureColor, weight);
      likelihood.replace(kPairTextureMatrix, weight);
    }
  }

  return likelihood;
}

MathNode::Pairing MathNode::PairingCalculator::GetMostLikelyPairingInternal(const QVector<int> &a,
                                                                            const QVector<int> &b,
                                                                            const int& weight_a,
                                                                            const int& weight_b)
{
  QVector<int> likelihoods(kPairCount);

  for (int i=0;i<likelihoods.size();i++) {
    if (a.at(i) == -1 || b.at(i) == -1) {
      likelihoods.replace(i, -1);
    } else {
      likelihoods.replace(i, a.at(i) + weight_a + b.at(i) + weight_b);
    }
  }

  Pairing pairing = kPairNone;

  for (int i=0;i<likelihoods.size();i++) {
    if (likelihoods.at(i) > -1) {
      if (pairing == kPairNone
          || likelihoods.at(i) > likelihoods.at(pairing)) {
        pairing = static_cast<Pairing>(i);
      }
    }
  }

  return pairing;
}

QVector4D MathNode::RetrieveVector(const NodeValue &val)
{
  // QVariant doesn't know that QVector*D can convert themselves so we do it here
  switch (val.type()) {
  case NodeParam::kVec2:
    return val.data().value<QVector2D>();
  case NodeParam::kVec3:
    return val.data().value<QVector3D>();
  case NodeParam::kVec4:
  default:
    return val.data().value<QVector4D>();
  }
}

void MathNode::PushVector(NodeValueTable *output, NodeParam::DataType type, const QVector4D &vec)
{
  switch (type) {
  case NodeParam::kVec2:
    output->Push(type, QVector2D(vec));
    break;
  case NodeParam::kVec3:
    output->Push(type, QVector3D(vec));
    break;
  case NodeParam::kVec4:
    output->Push(type, vec);
    break;
  default:
    break;
  }
}

float MathNode::RetrieveNumber(const NodeValue &val)
{
  if (val.type() == NodeParam::kRational) {
    return val.data().value<rational>().toDouble();
  } else {
    return val.data().toFloat();
  }
}

MathNode::PairingCalculator::PairingCalculator(const NodeValueTable &table_a, const NodeValueTable &table_b) :
  table_a_(table_a),
  table_b_(table_b)
{
  pair_likelihood_a_ = GetPairLikelihood(table_a_);
  pair_likelihood_b_ = GetPairLikelihood(table_b_);

  most_likely_pairing_ = GetMostLikelyPairingInternal(pair_likelihood_a_,
                                                      pair_likelihood_b_,
                                                      qMax(0, table_b_.Count() - table_a_.Count()),
                                                      qMax(0, table_a_.Count() - table_b_.Count()));
}

bool MathNode::PairingCalculator::FoundMostLikelyPairing() const
{
  return (most_likely_pairing_ > kPairNone && most_likely_pairing_ < kPairCount);
}

MathNode::Pairing MathNode::PairingCalculator::GetMostLikelyPairing() const
{
  return most_likely_pairing_;
}

const NodeValue &MathNode::PairingCalculator::GetMostLikelyValueA() const
{
  return GetMostLikelyValue(table_a_, pair_likelihood_a_);
}

const NodeValue &MathNode::PairingCalculator::GetMostLikelyValueB() const
{
  return GetMostLikelyValue(table_b_, pair_likelihood_b_);
}

const NodeValue& MathNode::PairingCalculator::GetMostLikelyValue(const NodeValueTable &table, const QVector<int> &likelihood) const
{
  return table.At(likelihood.at(most_likely_pairing_));
}

template<typename T, typename U>
T MathNode::PerformAll(T a, U b) const
{
  switch (GetOperation()) {
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
T MathNode::PerformMultDiv(T a, U b) const
{
  switch (GetOperation()) {
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
T MathNode::PerformAddSub(T a, U b) const
{
  switch (GetOperation()) {
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
T MathNode::PerformMult(T a, U b) const
{
  switch (GetOperation()) {
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
T MathNode::PerformAddSubMult(T a, U b) const
{
  switch (GetOperation()) {
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
T MathNode::PerformAddSubMultDiv(T a, U b) const
{
  switch (GetOperation()) {
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

OLIVE_NAMESPACE_EXIT
