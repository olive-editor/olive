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

#ifndef MATHNODE_H
#define MATHNODE_H

#include "node/node.h"

namespace olive {

class MathNode : public Node
{
  Q_OBJECT
public:
  MathNode();

  enum Operation {
    kOpAdd,
    kOpSubtract,
    kOpMultiply,
    kOpDivide,
    kOpPower
  };

  NODE_DEFAULT_FUNCTIONS(MathNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual value_t Value(const ValueParams &p) const override;

  static QString GetOperationName(Operation o);

  static const QString kMethodIn;
  static const QString kParamAIn;
  static const QString kParamBIn;
  static const QString kParamCIn;

  static void ProcessSamplesDouble(const void *context, const SampleJob &job, SampleBuffer &mixed_samples);

private:
  typedef value_t (*operation_t)(const value_t &a, const value_t &b);

  static std::map<Operation, std::map< type_t, std::map<type_t, operation_t> > > operations_;

  static void PopulateOperations();

  static void ProcessSamplesSamples(const void *context, const SampleJob &job, SampleBuffer &mixed_samples);

  static void OperateSampleNumber(Operation operation, const float *input, float *output, float b, size_t start, size_t end);
  static void OperateSampleSample(Operation operation, const float *input, float *output, const float *input2, size_t start, size_t end);

};

}

#endif // MATHNODE_H
