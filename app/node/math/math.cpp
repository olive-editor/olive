#include "math.h"

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
  if (input[param_a_in_].Has(NodeParam::kTexture) || input[param_b_in_].Has(NodeParam::kTexture)) {
    return kShader;
  } else if (input[param_a_in_].Has(NodeParam::kSamples) || input[param_b_in_].Has(NodeParam::kSamples)) {
    return kSampleProcessor;
  } else {
    return kNormal;
  }
}

QString MathNode::ShaderID(const NodeValueDatabase &input) const
{
  // FIXME: Hardcoded ADD operation
  QString method = QString::number(0);

  QString type_a = QString::number(GuessTypeFromTable(input[param_a_in_]));
  QString type_b = QString::number(GuessTypeFromTable(input[param_b_in_]));

  return id().append(method).append(type_a).append(type_b);
}

QString MathNode::ShaderFragmentCode(const NodeValueDatabase &input) const
{
  NodeParam::DataType type_a = GuessTypeFromTable(input[param_a_in_]);
  NodeParam::DataType type_b = GuessTypeFromTable(input[param_b_in_]);

  return QStringLiteral("#version 110\n"
                        "\n"
                        "varying vec2 ove_texcoord;\n"
                        "\n"
                        "uniform %1 %3;\n"
                        "uniform %2 %4;\n"
                        "\n"
                        "void main(void) {\n"
                        "  gl_FragColor = %5 + %6;\n"
                        "}\n").arg(GetUniformTypeFromType(type_a),
                                   GetUniformTypeFromType(type_b),
                                   param_a_in_->id(),
                                   param_b_in_->id(),
                                   GetVariableCall(param_a_in_->id(), type_a),
                                   GetVariableCall(param_b_in_->id(), type_b));
}

NodeValue MathNode::InputValueFromTable(NodeInput *input, const NodeValueTable &table) const
{
  if (input->IsConnected()
      && (input == param_a_in_ || input == param_b_in_)
      && table.Has(NodeParam::kTexture)) {
    return table.GetWithMeta(NodeParam::kTexture);
  }

  return Node::InputValueFromTable(input, table);
}

NodeParam::DataType MathNode::GuessTypeFromTable(const NodeValueTable &table)
{
  if (table.Has(NodeParam::kTexture)) {
    return NodeParam::kTexture;
  } else if (table.Has(NodeParam::kSamples)) {
    return NodeParam::kSamples;
  } else {
    return NodeParam::kFloat;
  }
}

QString MathNode::GetUniformTypeFromType(const NodeParam::DataType &type)
{
  if (type == NodeParam::kTexture) {
    return QStringLiteral("sampler2D");
  }

  return QStringLiteral("float");
}

QString MathNode::GetVariableCall(const QString &input_id, const NodeParam::DataType &type)
{
  if (type == NodeParam::kTexture) {
    return QStringLiteral("texture2D(%1, ove_texcoord)").arg(input_id);
  }

  return input_id;
}
