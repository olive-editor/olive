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

#ifndef NODEPARAMVIEW_H
#define NODEPARAMVIEW_H

#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
#include "nodeparamviewcontext.h"
#include "nodeparamviewdockarea.h"
#include "nodeparamviewitem.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

class NodeParamView : public TimeBasedWidget
{
  Q_OBJECT
public:
  NodeParamView(bool create_keyframe_view, QWidget* parent = nullptr);
  NodeParamView(QWidget* parent = nullptr) :
    NodeParamView(true, parent)
  {
  }

  virtual ~NodeParamView() override;

  void CloseContextsBelongingToProject(Project *p);

  Node* GetTimeTarget() const;

  void DeleteSelected();

  void SelectAll()
  {
    keyframe_view_->SelectAll();
  }

  void DeselectAll()
  {
    keyframe_view_->DeselectAll();
  }

  void SelectNodes(const QVector<Node*> &nodes);
  void DeselectNodes(const QVector<Node*> &nodes);

  const QVector<Node*> &GetContexts() const
  {
    return contexts_;
  }

public slots:
  void SetContexts(const QVector<Node*> &contexts);

  void UpdateElementY();

signals:
  void RequestSelectNode(const QVector<Node*>& target);

  void FocusedNodeChanged(Node* n);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double &) override;
  virtual void TimebaseChangedEvent(const rational&) override;
  virtual void TimeChangedEvent(const rational &time) override;

  virtual void ConnectedNodeChangeEvent(ViewerOutput* n) override;

private:
  void UpdateItemTime(const rational &time);

  void QueueKeyframePositionUpdate();

  void AddContext(Node *context);

  void RemoveContext(Node *context);

  void AddNode(Node* n, Node *ctx, NodeParamViewContext *context);

  void SortItemsInContext(NodeParamViewContext *context);

  NodeParamViewContext *GetContextItemFromContext(Node *context);

  bool IsGroupMode() const
  {
    return contexts_.size() == 1 && dynamic_cast<NodeGroup*>(contexts_.first());
  }

  KeyframeView* keyframe_view_;

  QVector<NodeParamViewContext*> context_items_;

  QScrollBar* vertical_scrollbar_;

  int last_scroll_val_;

  QScrollArea* param_scroll_area_;

  QWidget* param_widget_container_;

  NodeParamViewDockArea* param_widget_area_;

  QVector<Node*> pinned_nodes_;

  QVector<Node*> active_nodes_;

  NodeParamViewItem* focused_node_;

  Node *time_target_;

  QVector<Node*> contexts_;
  QVector<Node*> current_contexts_;

private slots:
  void UpdateGlobalScrollBar();

  void PinNode(bool pin);

  void FocusChanged(QWidget *old, QWidget *now);

  void KeyframeViewDragged(int x, int y);

  void NodeAddedToContext(Node *n);

  void NodeRemovedFromContext(Node *n);

  void InputCheckBoxChanged(const NodeInput &input, bool e);

  void GroupInputPassthroughAdded(olive::NodeGroup *group, const olive::NodeInput &input);

  void GroupInputPassthroughRemoved(olive::NodeGroup *group, const olive::NodeInput &input);

  void UpdateContexts();

  void ItemAboutToBeRemoved(NodeParamViewItem *item);

};

}

#endif // NODEPARAMVIEW_H
