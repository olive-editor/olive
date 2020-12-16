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

#ifndef NODEPARAMVIEW_H
#define NODEPARAMVIEW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
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
  NodeParamView(QWidget* parent = nullptr);

  void SelectNodes(const QVector<Node *> &nodes);
  void DeselectNodes(const QVector<Node*>& nodes);

  const QMap<Node*, NodeParamViewItem*>& GetItemMap() const
  {
    return items_;
  }

  Node* GetTimeTarget() const;

  void DeleteSelected();

signals:
  void InputDoubleClicked(NodeInput* input);

  void RequestSelectNode(const QVector<Node*>& target);

  void NodeOrderChanged(const QVector<Node*>& nodes);

  void FocusedNodeChanged(Node* n);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double &) override;
  virtual void TimebaseChangedEvent(const rational&) override;
  virtual void TimeChangedEvent(const int64_t &) override;

  virtual void ConnectedNodeChanged(ViewerOutput* n) override;

private:
  void UpdateItemTime(const int64_t &timestamp);

  void QueueKeyframePositionUpdate();

  void SignalNodeOrder();

  void RemoveNode(Node* n);

  KeyframeView* keyframe_view_;

  QMap<Node*, NodeParamViewItem*> items_;

  QScrollBar* vertical_scrollbar_;

  int last_scroll_val_;

  NodeParamViewParamContainer* param_widget_container_;

  // This may look weird, but QMainWindow is just a QWidget with a fancy layout that allows
  // docking windows
  QMainWindow* param_widget_area_;

  QVector<Node*> pinned_nodes_;

  QVector<Node*> active_nodes_;

  QMap<Node*, bool> node_expanded_state_;

  Node* focused_node_;

private slots:
  void ItemRequestedTimeChanged(const rational& time);

  void UpdateGlobalScrollBar();

  void PlaceKeyframesOnView();

  void PinNode(bool pin);

  void FocusChanged(QWidget *old, QWidget *now);

};

}

#endif // NODEPARAMVIEW_H
