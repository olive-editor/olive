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

  method_in_->set_name(tr(""));
  param_a_in_->set_name(tr(""));
  param_b_in_->set_name(tr(""));
}
