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

#ifndef MATHNODE_H
#define MATHNODE_H

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

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
    kPairTextureMatrix,
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

OLIVE_NAMESPACE_EXIT

#endif // MATHNODE_H
