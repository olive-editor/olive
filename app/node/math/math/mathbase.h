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

#ifndef MATHNODEBASE_H
#define MATHNODEBASE_H

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class MathNodeBase : public Node
{
public:
  MathNodeBase() = default;

  enum Operation {
    kOpAdd,
    kOpSubtract,
    kOpMultiply,
    kOpDivide,
    kOpPower
  };

protected:
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
  static T PerformAll(Operation operation, T a, U b);

  template<typename T, typename U>
  static T PerformMultDiv(Operation operation, T a, U b);

  template<typename T, typename U>
  static T PerformAddSub(Operation operation, T a, U b);

  template<typename T, typename U>
  static T PerformMult(Operation operation, T a, U b);

  template<typename T, typename U>
  static T PerformAddSubMult(Operation operation, T a, U b);

  template<typename T, typename U>
  static T PerformAddSubMultDiv(Operation operation, T a, U b);

  static QString GetShaderUniformType(const NodeParam::DataType& type);

  static QString GetShaderVariableCall(const QString& input_id, const NodeParam::DataType& type, const QString &coord_op = QString());

  static QVector4D RetrieveVector(const NodeValue& val);

  static float RetrieveNumber(const NodeValue& val);

  static bool NumberIsNoOp(const Operation& op, const float& number);

  ShaderCode GetShaderCodeInternal(const QString &shader_id, NodeInput* param_a_in, NodeInput* param_b_in) const;

  void PushVector(NodeValueTable* output, NodeParam::DataType type, const QVector4D& vec) const;

  NodeValueTable ValueInternal(NodeValueDatabase &value, Operation operation, Pairing pairing, NodeInput* param_a_in, const NodeValue &val_a, NodeInput* param_b_in, const NodeValue& val_b) const;

  void ProcessSamplesInternal(NodeValueDatabase &values, Operation operation, NodeInput* param_a_in, NodeInput* param_b_in, const SampleBufferPtr input, SampleBufferPtr output, int index) const;

};

OLIVE_NAMESPACE_EXIT

#endif // MATHNODEBASE_H
