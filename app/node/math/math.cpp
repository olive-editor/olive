#include "math.h"

#include <QMatrix4x4>
#include <QVector2D>

MathNode::MathNode()
{
  // FIXME: Make this a combobox
  method_in_ = new NodeInput(QStringLiteral("method_in"), NodeParam::kText);
  AddInput(method_in_);

  param_a_in_ = new NodeInput(QStringLiteral("param_a_in"), NodeParam::kFloat);
  AddInput(param_a_in_);

  param_b_in_ = new NodeInput(QStringLiteral("param_b_in"), NodeParam::kFloat);
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
  return tr("Perform a mathematical operation between two.");
}

void MathNode::Retranslate()
{
  Node::Retranslate();

  method_in_->set_name(tr("Method"));
  param_a_in_->set_name(tr("Value"));
  param_b_in_->set_name(tr("Value"));
}

Node::Capabilities MathNode::GetCapabilities(const NodeValueDatabase &input) const
{
  QVector<int> pair_likelihood_a = GetPairLikelihood(input[param_a_in_]);
  QVector<int> pair_likelihood_b = GetPairLikelihood(input[param_b_in_]);
  Pairing most_likely_pairing = GetMostLikelyPairing(pair_likelihood_a, pair_likelihood_b);

  switch (most_likely_pairing) {
  case kPairTextureColor:
  case kPairTextureNumber:
  case kPairTextureTexture:
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

  QVector<int> pair_likelihood_a = GetPairLikelihood(input[param_a_in_]);
  QVector<int> pair_likelihood_b = GetPairLikelihood(input[param_b_in_]);
  Pairing most_likely_pairing = GetMostLikelyPairing(pair_likelihood_a, pair_likelihood_b);

  QString type_a = QString::number(input[param_a_in_].At(pair_likelihood_a.at(most_likely_pairing)).type());
  QString type_b = QString::number(input[param_b_in_].At(pair_likelihood_b.at(most_likely_pairing)).type());

  return id().append(method).append(type_a).append(type_b);
}

QString MathNode::ShaderFragmentCode(const NodeValueDatabase &input) const
{
  QVector<int> pair_likelihood_a = GetPairLikelihood(input[param_a_in_]);
  QVector<int> pair_likelihood_b = GetPairLikelihood(input[param_b_in_]);
  Pairing most_likely_pairing = GetMostLikelyPairing(pair_likelihood_a, pair_likelihood_b);

  NodeParam::DataType type_a = input[param_a_in_].At(pair_likelihood_a.at(most_likely_pairing)).type();
  NodeParam::DataType type_b = input[param_b_in_].At(pair_likelihood_b.at(most_likely_pairing)).type();

  return QStringLiteral("#version 110\n"
                        "\n"
                        "varying vec2 ove_texcoord;\n"
                        "\n"
                        "uniform %1 %3;\n"
                        "uniform %2 %4;\n"
                        "\n"
                        "void main(void) {\n"
                        "  gl_FragColor = %5 + %6;\n"
                        "}\n").arg(GetShaderUniformType(type_a),
                                   GetShaderUniformType(type_b),
                                   param_a_in_->id(),
                                   param_b_in_->id(),
                                   GetShaderVariableCall(param_a_in_->id(), type_a),
                                   GetShaderVariableCall(param_b_in_->id(), type_b));
}

NodeValue MathNode::InputValueFromTable(NodeInput *input, const NodeValueDatabase &db) const
{
  if (input == param_a_in_ || input == param_b_in_) {
    QVector<int> pair_likelihood_a = GetPairLikelihood(db[param_a_in_]);
    QVector<int> pair_likelihood_b = GetPairLikelihood(db[param_b_in_]);
    Pairing most_likely_pairing = GetMostLikelyPairing(pair_likelihood_a, pair_likelihood_b);

    if (input == param_a_in_) {
      return db[input].At(pair_likelihood_a.at(most_likely_pairing));
    } else {
      return db[input].At(pair_likelihood_b.at(most_likely_pairing));
    }
  }

  return Node::InputValueFromTable(input, db);
}

NodeValueTable MathNode::Value(const NodeValueDatabase &value) const
{
  NodeValueTable output = value.Merge();

  // Auto-detect what values to operate with
  // FIXME: Add manual override for this
  QVector<int> pair_likelihood_a = GetPairLikelihood(value[param_a_in_]);
  QVector<int> pair_likelihood_b = GetPairLikelihood(value[param_b_in_]);
  Pairing most_likely_pairing = GetMostLikelyPairing(pair_likelihood_a, pair_likelihood_b);

  NodeValue val_a, val_b;
  if (most_likely_pairing >= 0 && most_likely_pairing < kPairCount) {
    val_a = value[param_a_in_].At(pair_likelihood_a.at(most_likely_pairing));
    val_b = value[param_a_in_].At(pair_likelihood_a.at(most_likely_pairing));
  }

  switch (most_likely_pairing) {

  case kPairNumberNumber:
  {
    if (val_a.type() == NodeParam::kRational && val_b.type() == NodeParam::kRational) {
      // Preserve rationals
      output.Push(NodeParam::kRational, QVariant::fromValue(val_a.data().value<rational>() + val_b.data().value<rational>()));
    } else {
      float flt_a = RetrieveNumber(val_a);
      float flt_b = RetrieveNumber(val_b);

      output.Push(NodeParam::kFloat, flt_a + flt_b);
    }
    break;
  }

  case kPairVecVec:
  {
    // We convert all vectors to QVector4D just for simplicity and exploit the fact that kVec4 is higher than kVec2 in
    // the enum to find the largest data type
    PushVector(&output, qMax(val_a.type(), val_b.type()), RetrieveVector(val_a) + RetrieveVector(val_b));
    break;
  }

  case kPairMatrixVec:
  {
    QMatrix4x4 matrix = (val_a.type() == NodeParam::kMatrix) ? val_a.data().value<QMatrix4x4>() : val_b.data().value<QMatrix4x4>();
    QVector4D vec = (val_a.type() == NodeParam::kMatrix) ? RetrieveVector(val_b) : RetrieveVector(val_a);

    PushVector(&output, qMax(val_a.type(), val_b.type()), vec * matrix);
    break;
  }

  case kPairVecNumber:
  {
    QVector4D vec = (val_a.type() & NodeParam::kVector) ? RetrieveVector(val_a) : RetrieveVector(val_b);
    float number = RetrieveNumber((val_a.type() & NodeParam::kMatrix) ? val_b : val_a);

    PushVector(&output, val_a.type(), vec * number);
    break;
  }

  case kPairMatrixMatrix:
  {
    QMatrix4x4 mat_a = value[param_a_in_].At(pair_likelihood_a.at(most_likely_pairing)).data().value<QMatrix4x4>();
    QMatrix4x4 mat_b = value[param_b_in_].At(pair_likelihood_b.at(most_likely_pairing)).data().value<QMatrix4x4>();
    output.Push(NodeParam::kMatrix, mat_a + mat_b);
    break;
  }

  case kPairColorColor:
  case kPairNumberColor:
  {
    // FIXME: Still undecided on the true representation of color
    break;
  }

  case kPairSampleSample:
  {
    SampleBufferPtr samples_a = value[param_a_in_].Get(NodeParam::kSamples).value<SampleBufferPtr>();
    SampleBufferPtr samples_b = value[param_b_in_].Get(NodeParam::kSamples).value<SampleBufferPtr>();

    int max_samples = qMax(samples_a->sample_count_per_channel(), samples_b->sample_count_per_channel());
    int min_samples = qMin(samples_a->sample_count_per_channel(), samples_b->sample_count_per_channel());

    SampleBufferPtr mixed_samples = SampleBuffer::CreateAllocated(samples_a->audio_params(), max_samples);

    // Mix samples that are in both buffers
    for (int i=0;i<mixed_samples->audio_params().channel_count();i++) {
      for (int j=0;j<min_samples;j++) {
        mixed_samples->data()[i][j] = samples_a->data()[i][j] + samples_b->data()[i][j];
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
  case kPairSampleNumber:
    // Do nothing
    break;
  }

  return output;
}

NodeInput *MathNode::ProcessesSamplesFrom(const NodeValueDatabase &value) const
{
  QVector<int> pair_likelihood_a = GetPairLikelihood(value[param_a_in_]);
  QVector<int> pair_likelihood_b = GetPairLikelihood(value[param_b_in_]);
  Pairing most_likely_pairing = GetMostLikelyPairing(pair_likelihood_a, pair_likelihood_b);

  if (most_likely_pairing == kPairSampleNumber) {
    if (value[param_a_in_].At(pair_likelihood_a.at(most_likely_pairing)).type() == NodeParam::kSamples) {
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
    output->data()[i][index] = input->data()[i][index] + number_flt;
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
  return kOpAdd;
}

QString MathNode::GetShaderUniformType(const NodeParam::DataType &type)
{
  switch (type) {
  case NodeParam::kTexture:
    return QStringLiteral("sampler2D");
  case NodeParam::kColor:
    return QStringLiteral("vec4");
  default:
    return QStringLiteral("float");
  }
}

QString MathNode::GetShaderVariableCall(const QString &input_id, const NodeParam::DataType &type)
{
  if (type == NodeParam::kTexture) {
    return QStringLiteral("texture2D(%1, ove_texcoord)").arg(input_id);
  }

  return input_id;
}

QVector<int> MathNode::GetPairLikelihood(const NodeValueTable &table)
{
  // FIXME: When we introduce a manual override, placing it here would be the least problematic

  QVector<int> likelihood(kPairCount, -1);

  for (int i=0;i<table.Count();i++) {
    NodeParam::DataType type = table.At(i).type();

    if (type & NodeParam::kVector) {
      likelihood.replace(kPairVecVec, i);
      likelihood.replace(kPairVecNumber, i);
      likelihood.replace(kPairMatrixVec, i);
    } else if (type & NodeParam::kMatrix) {
      likelihood.replace(kPairMatrixMatrix, i);
      likelihood.replace(kPairMatrixVec, i);
    } else if (type & NodeParam::kColor) {
      likelihood.replace(kPairColorColor, i);
      likelihood.replace(kPairNumberColor, i);
      likelihood.replace(kPairTextureColor, i);
    } else if (type & NodeParam::kNumber) {
      likelihood.replace(kPairNumberNumber, i);
      likelihood.replace(kPairVecNumber, i);
      likelihood.replace(kPairNumberColor, i);
      likelihood.replace(kPairTextureNumber, i);
      likelihood.replace(kPairSampleNumber, i);
    } else if (type & NodeParam::kSamples) {
      likelihood.replace(kPairSampleSample, i);
      likelihood.replace(kPairSampleNumber, i);
    } else if (type & NodeParam::kTexture) {
      likelihood.replace(kPairTextureTexture, i);
      likelihood.replace(kPairTextureNumber, i);
      likelihood.replace(kPairTextureColor, i);
    }
  }

  return likelihood;
}

MathNode::Pairing MathNode::GetMostLikelyPairing(const QVector<int> &a, const QVector<int> &b)
{
  QVector<int> likelihoods(kPairCount);

  for (int i=0;i<likelihoods.size();i++) {
    if (a.at(i) == -1 || b.at(i) == -1) {
      likelihoods.replace(i, -1);
    } else {
      likelihoods.replace(i, a.at(i) + b.at(i));
    }
  }

  Pairing pairing = kPairNone;

  for (int i=0;i<likelihoods.size();i++) {
    //qDebug() << "Likelihood" << i << "is" << likelihoods.at(i);

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
