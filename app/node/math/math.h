#ifndef MATHNODE_H
#define MATHNODE_H

#include "node/node.h"

class MathNode : public Node
{
public:
  MathNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual Capabilities GetCapabilities(const NodeValueDatabase&) const override;
  virtual QString ShaderID(const NodeValueDatabase&) const override;
  virtual QString ShaderFragmentCode(const NodeValueDatabase&) const override;

  virtual NodeValue InputValueFromTable(NodeInput* input, const NodeValueDatabase &db) const override;

  virtual NodeValueTable Value(const NodeValueDatabase& value) const override;

  virtual NodeInput* ProcessesSamplesFrom(const NodeValueDatabase &value) const override;
  virtual void ProcessSamples(const NodeValueDatabase &values, const AudioRenderingParams& params, const SampleBufferPtr input, SampleBufferPtr output, int index) const override;

  NodeInput* param_a_in() const;
  NodeInput* param_b_in() const;

private:
  enum Operation {
    kOpAdd,
    kOpSubtrack,
    kOpMultiply,
    kOpDivide
  };

  Operation GetOperation() const;

  enum Pairing {
    kPairNone = -1,

    kPairNumberNumber,
    kPairVecVec,
    kPairMatrixMatrix,
    kPairColorColor,
    kPairTextureTexture,

    kPairVecNumber,
    kPairMatrixVec,
    kPairNumberColor,
    kPairTextureNumber,
    kPairTextureColor,
    kPairSampleSample,
    kPairSampleNumber,

    kPairCount
  };

  template<typename T, typename U>
  static T OperationAdd(T a, U b);

  static QString GetShaderUniformType(const NodeParam::DataType& type);

  static QString GetShaderVariableCall(const QString& input_id, const NodeParam::DataType& type);

  static QVector<int> GetPairLikelihood(const NodeValueTable& table);

  static Pairing GetMostLikelyPairing(const QVector<int> &a, const QVector<int> &b);

  static QVector4D RetrieveVector(const NodeValue& val);

  static void PushVector(NodeValueTable* output, NodeParam::DataType type, const QVector4D& vec);

  static float RetrieveNumber(const NodeValue& val);

  NodeInput* method_in_;

  NodeInput* param_a_in_;

  NodeInput* param_b_in_;

};

#endif // MATHNODE_H
