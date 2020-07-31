/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

OLIVE_NAMESPACE_ENTER

/**
 * @brief A PanelWidget wrapper around a NodeView
 */
class NodePanel : public PanelWidget
{
  Q_OBJECT
public:
  NodePanel(QWidget* parent);

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

public slots:
  void Select(const QList<Node*>& nodes)
  {
    node_view_->Select(nodes);
  }

  void SelectWithDependencies(const QList<Node*>& nodes)
  {
    node_view_->SelectWithDependencies(nodes);
  }

  void SelectBlocks(const QList<Block*>& nodes)
  {
    node_view_->SelectBlocks(nodes);
  }

  void DeselectBlocks(const QList<Block*>& nodes)
  {
    node_view_->DeselectBlocks(nodes);
  }

signals:
  void NodesSelected(const QList<Node*>& nodes);

  void NodesDeselected(const QList<Node*>& nodes);

private:
  virtual void Retranslate() override
  {
    SetTitle(tr("Node Editor"));
  }

  NodeView* node_view_;

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPANEL_H
