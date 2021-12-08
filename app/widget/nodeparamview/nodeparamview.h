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

class NodeParamViewParamContainer : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewParamContainer(QWidget* parent = nullptr) :
    QWidget(parent)
  {
  }

protected:
  virtual void resizeEvent(QResizeEvent *event) override
  {
    QWidget::resizeEvent(event);

    emit Resized(event->size().height());
  }

signals:
  void Resized(int new_height);

};

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

  void SetCreateCheckBoxes(NodeParamViewCheckBoxBehavior e)
  {
    create_checkboxes_ = e;
  }

  bool IsInputChecked(const NodeInput &input) const
  {
    return input_checked_.value(input);
  }

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

public slots:
  void SetInputChecked(const NodeInput &input, bool e);

  void SetContexts(const QVector<Node*> &contexts);

signals:
  void RequestSelectNode(const QVector<Node*>& target);

  void NodeOrderChanged(const QVector<Node*>& nodes);

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

  void SignalNodeOrder();

  void AddNode(Node* n, NodeParamViewContext *context);

  //void AddNode(Node *node, Node *context, NodeParamViewContext *ctx_item);

  void RemoveNode(Node* n);

  void SortItemsInContext(NodeParamViewContext *context);

  KeyframeView* keyframe_view_;

  QVector<NodeParamViewContext*> context_items_;

  QScrollBar* vertical_scrollbar_;

  int last_scroll_val_;

  QScrollArea* param_scroll_area_;

  NodeParamViewParamContainer* param_widget_container_;

  NodeParamViewDockArea* param_widget_area_;

  QVector<Node*> pinned_nodes_;

  QVector<Node*> active_nodes_;

  NodeParamViewItem* focused_node_;

  NodeParamViewCheckBoxBehavior create_checkboxes_;

  Node *time_target_;

  QHash<NodeInput, bool> input_checked_;

private slots:
  void UpdateGlobalScrollBar();

  void PinNode(bool pin);

  void FocusChanged(QWidget *old, QWidget *now);

  void KeyframeViewDragged(int x, int y);

  void UpdateElementY();

};

}

#endif // NODEPARAMVIEW_H
