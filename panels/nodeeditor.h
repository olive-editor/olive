#ifndef NODEEDITOR_H
#define NODEEDITOR_H

#include <QGraphicsScene>

#include "effectspanel.h"
#include "ui/nodeview.h"
#include "ui/nodeui.h"
#include "ui/nodeedgeui.h"

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
  QVector<NodeEdgeUI*> edges_;
  QVector<EffectRow*> connected_rows_;

  void ClearEdges();
  void LoadEdges();

  void ConnectRow(EffectRow* row);
  void DisconnectAllRows();

private slots:
  void ItemsChanged();
  void ReloadEdges();

};

#endif // NODEEDITOR_H
