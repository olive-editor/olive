/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef NODEPANEL_H
#define NODEPANEL_H

#include "widget/nodeview/nodeview.h"
#include "widget/panel/panel.h"

namespace olive {

/**
 * @brief A PanelWidget wrapper around a NodeView
 */
class NodePanel : public PanelWidget
{
  Q_OBJECT
public:
  NodePanel(QWidget* parent);

  NodeGraph* GetGraph() const
  {
    return node_view_->GetGraph();
  }

  void SetGraph(NodeGraph *graph)
  {
    node_view_->SetGraph(graph);
  }

  virtual void SelectAll() override
  {
    node_view_->SelectAll();
  }

  virtual void DeselectAll() override
  {
    node_view_->DeselectAll();
  }

  virtual void DeleteSelected() override
  {
    node_view_->DeleteSelected();
  }

  virtual void CutSelected() override
  {
    node_view_->CopySelected(true);
  }

  virtual void CopySelected() override
  {
    node_view_->CopySelected(false);
  }

  virtual void Paste() override
  {
    node_view_->Paste();
  }

  virtual void Duplicate() override
  {
    node_view_->Duplicate();
  }

  virtual void SetColorLabel(int index) override
  {
    node_view_->SetColorLabel(index);
  }

  virtual void ZoomIn() override
  {
    node_view_->ZoomIn();
  }

  virtual void ZoomOut() override
  {
    node_view_->ZoomOut();
  }

public slots:
  void Select(const QVector<Node*>& nodes)
  {
    node_view_->Select(nodes);
  }

  void SelectWithDependencies(const QVector<Node*>& nodes)
  {
    node_view_->SelectWithDependencies(nodes);
  }

  void SelectBlocks(const QVector<Block*>& blocks)
  {
    QVector<Node*> nodes(blocks.size());
    memcpy(nodes.data(), blocks.constData(), blocks.size() * sizeof(Block*));
    node_view_->SelectWithDependencies(nodes);
  }

  void DeselectBlocks(const QVector<Block*>& nodes)
  {
    Q_UNUSED(nodes)
    qDebug() << "Stub";
    //node_view_->DeselectBlocks(nodes);
  }

signals:
  void NodesSelected(const QVector<Node*>& nodes);

  void NodesDeselected(const QVector<Node*>& nodes);

private:
  virtual void Retranslate() override
  {
    SetTitle(tr("Node Editor"));
  }

  NodeView* node_view_;

};

}

#endif // NODEPANEL_H
