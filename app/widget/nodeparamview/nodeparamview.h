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

#ifndef NODEPARAMVIEW_H
#define NODEPARAMVIEW_H

#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
#include "nodeparamviewitem.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/timebased/timebased.h"

OLIVE_NAMESPACE_ENTER

class NodeParamView : public TimeBasedWidget
{
  Q_OBJECT
public:
  NodeParamView(QWidget* parent = nullptr);

  void SetNodes(QList<Node*> nodes);
  const QList<Node*>& nodes();

  Node* GetTimeTarget() const;

  void DeleteSelected();

signals:
  void InputDoubleClicked(NodeInput* input);

  void RequestSelectNode(const QList<Node*>& target);

  void OpenedNode(Node* n);

  void ClosedNode(Node* n);

  void FoundGizmos(Node* n);

protected:
  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void ScaleChangedEvent(const double &) override;
  virtual void TimebaseChangedEvent(const rational&) override;
  virtual void TimeChangedEvent(const int64_t &) override;

  virtual void ConnectedNodeChanged(ViewerOutput* n) override;

  virtual void ConnectNodeInternal(ViewerOutput* n) override;
  virtual void DisconnectNodeInternal(ViewerOutput* n) override;

private:
  void UpdateItemTime(const int64_t &timestamp);

  QVBoxLayout* param_layout_;

  KeyframeView* keyframe_view_;

  QList<Node*> nodes_;

  QList<NodeParamViewItem*> items_;

  QScrollBar* vertical_scrollbar_;

  int last_scroll_val_;

  QWidget* param_widget_area_;

private slots:
  void ItemRequestedTimeChanged(const rational& time);

  void ForceKeyframeViewToScroll();

  void PlaceKeyframesOnView();

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEW_H
