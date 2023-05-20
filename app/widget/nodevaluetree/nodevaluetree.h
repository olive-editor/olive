#ifndef NODEVALUETREE_H
#define NODEVALUETREE_H

#include <QRadioButton>
#include <QTreeWidget>

#include "node/node.h"
#include "valueswizzlewidget.h"

namespace olive {

class NodeValueTree : public QTreeWidget
{
  Q_OBJECT
public:
  NodeValueTree(QWidget *parent = nullptr);

  void SetNode(const NodeInput &input);

  bool DeleteSelected();

protected:
  virtual void changeEvent(QEvent *event) override;

private:
  void Retranslate();

  ValueSwizzleWidget *GetSwizzleWidgetFromTopLevelItem(int i);

  NodeInput input_;

private slots:
  void RadioButtonChecked(bool e);

  void Update();

  void SwizzleChanged(const SwizzleMap &map);

  void ValueHintChanged(const NodeInput &input);

};

}

#endif // NODEVALUETREE_H
