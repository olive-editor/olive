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

#include <QDockWidget>
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

class NodeParamViewItemTitleBar : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewItemTitleBar(QWidget* parent = nullptr);

  void SetExpanded(bool e);

  void SetText(const QString& s)
  {
    lbl_->setText(s);
  }

signals:
  void ExpandedStateChanged(bool e);

  void PinToggled(bool e);

protected:
  virtual void paintEvent(QPaintEvent *event) override;

  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
  bool draw_border_;

  QLabel* lbl_;

  CollapseButton* collapse_btn_;

};

class NodeParamViewItemBody : public QWidget {
  Q_OBJECT
public:
  NodeParamViewItemBody(const QVector<NodeInput*>& inputs, QWidget* parent = nullptr);

  void SetTimeTarget(Node* target);

  void SetTime(const rational& time);

  void Retranslate();

  void SignalAllKeyframes();

signals:
  void KeyframeAdded(NodeKeyframePtr key, int y);

  void KeyframeRemoved(NodeKeyframePtr key);

  void RequestSetTime(const rational& time);

  void InputDoubleClicked(NodeInput* input);

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

  QVector<NodeParamViewItemBody*> sub_bodies_;

private slots:
  void EdgeChanged();

  void InputKeyframeEnableChanged(bool e);

  void InputAddedKeyframe(NodeKeyframePtr key);

  void LabelDoubleClicked();

  void ConnectionClicked();

};

class NodeParamViewItem : public QDockWidget
{
  Q_OBJECT
public:
  NodeParamViewItem(Node* node, QWidget* parent = nullptr);

  void SetTimeTarget(Node* target);

  void SetTime(const rational& time);

  Node* GetNode() const;

  bool IsExpanded() const;

public slots:
  void SignalAllKeyframes();

signals:
  void KeyframeAdded(NodeKeyframePtr key, int y);

  void KeyframeRemoved(NodeKeyframePtr key);

  void RequestSetTime(const rational& time);

  void InputDoubleClicked(NodeInput* input);

  void RequestSelectNode(const QList<Node*>& node);

  void PinToggled(bool e);

public slots:
  void SetExpanded(bool e);

  void ToggleExpanded();

protected:
  virtual void changeEvent(QEvent *e) override;

private:
  NodeParamViewItemTitleBar* title_bar_;

  NodeParamViewItemBody* body_;

  Node* node_;

  rational time_;

private slots:
  void Retranslate();

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEWITEM_H
