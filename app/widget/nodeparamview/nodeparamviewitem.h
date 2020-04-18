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

#ifndef NODEPARAMVIEWITEM_H
#define NODEPARAMVIEWITEM_H

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
#include "nodeparamviewconnectedlabel.h"
#include "nodeparamviewkeyframecontrol.h"
#include "nodeparamviewwidgetbridge.h"
#include "widget/clickablelabel/clickablelabel.h"
#include "widget/collapsebutton/collapsebutton.h"

OLIVE_NAMESPACE_ENTER

class NodeParamViewItemTitleBar : public QWidget {
public:
  NodeParamViewItemTitleBar(QWidget* parent = nullptr);

protected:
  virtual void paintEvent(QPaintEvent *event) override;
};

class NodeParamViewItemBody : public QWidget {
  Q_OBJECT
public:
  NodeParamViewItemBody(const QList<NodeInput*>& inputs, QWidget* parent = nullptr);

  void SetTimeTarget(Node* target);

  void SetTime(const rational& time);

  void Retranslate();

  void SignalAllKeyframes();

signals:
  void KeyframeAdded(NodeKeyframePtr key, int y);

  void KeyframeRemoved(NodeKeyframePtr key);

  void RequestSetTime(const rational& time);

  void InputClicked(NodeInput* input);

  void RequestSelectNode(const QList<Node*>& node);

private:
  void UpdateUIForEdgeConnection(NodeInput* input);

  void InputAddedKeyframeInternal(NodeInput* input, NodeKeyframePtr keyframe);

  struct InputUI {
    InputUI();

    ClickableLabel* main_label;
    NodeParamViewWidgetBridge* widget_bridge;
    NodeParamViewConnectedLabel* connected_label;
    NodeParamViewKeyframeControl* key_control;
  };

  QMap<NodeInput*, InputUI> input_ui_map_;

  QList<NodeParamViewItemBody*> sub_bodies_;

private slots:
  void EdgeChanged();

  void InputKeyframeEnableChanged(bool e);

  void InputAddedKeyframe(NodeKeyframePtr key);

  void LabelClicked();

  void ConnectionClicked();

};

class NodeParamViewItem : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewItem(Node* node, QWidget* parent = nullptr);

  void SetTimeTarget(Node* target);

  void SetTime(const rational& time);

public slots:
  void SignalAllKeyframes();

signals:
  void KeyframeAdded(NodeKeyframePtr key, int y);

  void KeyframeRemoved(NodeKeyframePtr key);

  void RequestSetTime(const rational& time);

  void InputClicked(NodeInput* input);

  void RequestSelectNode(const QList<Node*>& node);

protected:
  virtual void changeEvent(QEvent *e) override;

private:
  void Retranslate();

  bool expanded_;

  NodeParamViewItemTitleBar* title_bar_;

  QLabel* title_bar_lbl_;

  CollapseButton* title_bar_collapse_btn_;

  NodeParamViewItemBody* body_;

  Node* node_;

  rational time_;

private slots:
  void SetExpanded(bool e);

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEWITEM_H
