#ifndef MULTICAMNODE_H
#define MULTICAMNODE_H

#include "node/node.h"

namespace olive {

class MultiCamNode : public Node
{
  Q_OBJECT
public:
  MultiCamNode();

  NODE_DEFAULT_FUNCTIONS(MultiCamNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual ActiveElements GetActiveElementsAtTime(const QString &input, const TimeRange &r) const override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  virtual void Retranslate() override;

  static const QString kCurrentInput;
  static const QString kSourcesInput;

};

}

#endif // MULTICAMNODE_H
