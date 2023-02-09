#ifndef NODEVALUETREE_H
#define NODEVALUETREE_H

#include <QRadioButton>
#include <QTreeWidget>

#include "node/node.h"

namespace olive {

class NodeValueTree : public QTreeWidget
{
  Q_OBJECT
public:
  NodeValueTree(QWidget *parent = nullptr);

  void SetNode(const NodeInput &input, const rational &time);

protected:
  virtual void changeEvent(QEvent *event) override;

private:
  void Retranslate();

private slots:
  void RadioButtonChecked(bool e);

};

}

#endif // NODEVALUETREE_H
