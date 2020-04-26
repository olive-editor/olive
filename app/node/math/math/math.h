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
  virtual QString ShaderVertexCode(const NodeValueDatabase&input) const override;

  virtual NodeValue InputValueFromTable(NodeInput* input, NodeValueDatabase &db, bool take) const override;

  virtual NodeValueTable Value(NodeValueDatabase &value) const override;

  virtual NodeInput* ProcessesSamplesFrom(const NodeValueDatabase &value) const override;
  virtual void ProcessSamples(const NodeValueDatabase &values, const AudioRenderingParams& params, const SampleBufferPtr input, SampleBufferPtr output, int index) const override;

  NodeInput* param_a_in() const;
  NodeInput* param_b_in() const;

  enum Operation {
    kOpAdd,
    kOpSubtract,
    kOpMultiply,
    kOpDivide,
    kOpPower
  };

  Operation GetOperation() const;
  void SetOperation(Operation o);

private:
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

  class PairingCalculator {
  public:
    PairingCalculator(const NodeValueTable &table_a, const NodeValueTable &table_b);

    bool FoundMostLikelyPairing() const;
    Pairing GetMostLikelyPairing() const;

    const NodeValue& GetMostLikelyValueA() const;
    const NodeValue& GetMostLikelyValueB() const;

  private:
    static QVector<int> GetPairLikelihood(const NodeValueTable& table);

    Pairing most_likely_pairing_;

    NodeValue most_likely_value_a_;

    NodeValue most_likely_value_b_;

  };

  template<typename T, typename U>
  T PerformAll(T a, U b) const;

  template<typename T, typename U>
  T PerformMultDiv(T a, U b) const;

  template<typename T, typename U>
  T PerformAddSub(T a, U b) const;

  template<typename T, typename U>
  T PerformMult(T a, U b) const;

  template<typename T, typename U>
  T PerformAddSubMult(T a, U b) const;

  template<typename T, typename U>
  T PerformAddSubMultDiv(T a, U b) const;

  static QString GetShaderUniformType(const NodeParam::DataType& type);

  static QString GetShaderVariableCall(const QString& input_id, const NodeParam::DataType& type, const QString &coord_op = QString());

  static QVector4D RetrieveVector(const NodeValue& val);

  static float RetrieveNumber(const NodeValue& val);

  static void PushVector(NodeValueTable* output, NodeParam::DataType type, const QVector4D& vec);

  NodeInput* method_in_;

  NodeInput* param_a_in_;

  NodeInput* param_b_in_;

};

OLIVE_NAMESPACE_EXIT

#endif // MATHNODE_H
