#ifndef NODEEDITOR_H
#define NODEEDITOR_H

#include <QGraphicsScene>

#include "ui/panel.h"
#include "ui/nodeview.h"

class NodeEditor : public Panel {
  Q_OBJECT
public:
  NodeEditor(QWidget* parent = nullptr);

  virtual void Retranslate() override;

private:
  QGraphicsScene scene_;
  NodeView view_;

};

#endif // NODEEDITOR_H
