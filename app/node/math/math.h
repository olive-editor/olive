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

  virtual NodeValue InputValueFromTable(NodeInput* input, const NodeValueTable& table) const override;

private:
  enum Operation {
    kOpAdd,
    kOpSubtrack,
    kOpMultiply,
    kOpDivide
  };

  static NodeParam::DataType GuessTypeFromTable(const NodeValueTable& table);

  static QString GetUniformTypeFromType(const NodeParam::DataType& type);

  static QString GetVariableCall(const QString& input_id, const NodeParam::DataType& type);

  NodeInput* method_in_;

  NodeInput* param_a_in_;

  NodeInput* param_b_in_;

};

#endif // MATHNODE_H
