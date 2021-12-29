/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "widget/nodeview/nodewidget.h"
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

  NodeWidget *GetNodeWidget() const
  {
    return node_widget_;
  }

  const QVector<Node*> &GetContexts() const
  {
    return node_widget_->view()->GetContexts();
  }

  void SetContexts(const QVector<Node*> &nodes)
  {
    node_widget_->SetContexts(nodes);
  }

  void CloseContextsBelongingToProject(Project *project)
  {
    node_widget_->view()->CloseContextsBelongingToProject(project);
  }

  const QVector<Node*> &GetCurrentContexts() const
  {
    return node_widget_->view()->GetCurrentContexts();
  }

  virtual void SelectAll() override
  {
    node_widget_->view()->SelectAll();
  }

  virtual void DeselectAll() override
  {
    node_widget_->view()->DeselectAll();
  }

  virtual void DeleteSelected() override
  {
    node_widget_->view()->DeleteSelected();
  }

  virtual void CutSelected() override
  {
    node_widget_->view()->CopySelected(true);
  }

  virtual void CopySelected() override
  {
    node_widget_->view()->CopySelected(false);
  }

  virtual void Paste() override
  {
    node_widget_->view()->Paste();
  }

  virtual void Duplicate() override
  {
    node_widget_->view()->Duplicate();
  }

  virtual void SetColorLabel(int index) override
  {
    node_widget_->view()->SetColorLabel(index);
  }

  virtual void ZoomIn() override
  {
    node_widget_->view()->ZoomIn();
  }

  virtual void ZoomOut() override
  {
    node_widget_->view()->ZoomOut();
  }

public slots:
  void Select(const QVector<Node*>& nodes, bool center_view_on_item)
  {
    node_widget_->view()->Select(nodes, center_view_on_item);
    this->raise();
  }

signals:
  void NodesSelected(const QVector<Node*>& nodes);

  void NodesDeselected(const QVector<Node*>& nodes);

  void NodeGroupOpenRequested(NodeGroup *group);

private:
  virtual void Retranslate() override
  {
    SetTitle(tr("Node Editor"));
  }

  NodeWidget *node_widget_;

};

}

#endif // NODEPANEL_H
