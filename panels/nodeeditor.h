#ifndef NODEEDITOR_H
#define NODEEDITOR_H

#include <QGraphicsScene>

#include "effectspanel.h"
#include "ui/nodeview.h"
#include "ui/nodeui.h"

class NodeEditor : public EffectsPanel {
  Q_OBJECT
public:
  NodeEditor(QWidget* parent = nullptr);

  virtual void Retranslate() override;

protected:
  virtual void LoadEvent() override;
  virtual void ClearEvent() override;

private:
  QGraphicsScene scene_;
  NodeView view_;
  QVector<NodeUI*> nodes_;

private slots:
  void Scroll(qreal x, qreal y);

};

#endif // NODEEDITOR_H
