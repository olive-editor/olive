#include "math.h"

MathNode::MathNode()
{

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
