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

#ifndef NODEPARAMVIEWITEM_H
#define NODEPARAMVIEWITEM_H

#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "node/node.h"
#include "nodeparamviewarraywidget.h"
#include "nodeparamviewconnectedlabel.h"
#include "nodeparamviewkeyframecontrol.h"
#include "nodeparamviewwidgetbridge.h"
#include "widget/clickablelabel/clickablelabel.h"
#include "widget/collapsebutton/collapsebutton.h"

namespace olive {

class NodeParamViewItemTitleBar : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewItemTitleBar(QWidget* parent = nullptr);

  void SetExpanded(bool e);

  void SetText(const QString& s)
  {
    lbl_->setText(s);
    lbl_->setToolTip(s);
    lbl_->setMinimumWidth(1);
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
  NodeParamViewItemBody(Node* node, QWidget* parent = nullptr);

  void SetTimeTarget(Node* target);

  void SetTime(const rational& time);

  void Retranslate();

  int GetElementY(NodeInput c) const;

  // Set the timebase of any timebased widgets contained here
   void SetTimebase(const rational& timebase);

signals:
  void RequestSetTime(const rational& time);

  void RequestSelectNode(const QVector<Node*>& node);

  void ArrayExpandedChanged(bool e);

private:
  void CreateWidgets(QGridLayout *layout, Node* node, const QString& input, int element, int row_index);

  void UpdateUIForEdgeConnection(const NodeInput &input);

  void PlaceWidgetsFromBridge(QGridLayout *layout, NodeParamViewWidgetBridge* bridge, int row);

  struct InputUI {
    InputUI();

    QLabel* main_label;
    NodeParamViewWidgetBridge* widget_bridge;
    NodeParamViewConnectedLabel* connected_label;
    NodeParamViewKeyframeControl* key_control;
    QGridLayout* layout;
    int row;

    NodeParamViewArrayButton* array_insert_btn;
    NodeParamViewArrayButton* array_remove_btn;
  };

  QHash<NodeInput, InputUI> input_ui_map_;

  struct ArrayUI {
    QWidget* widget;
    int count;
    NodeParamViewArrayButton* append_btn;
  };

  QHash<NodeInputPair, ArrayUI> array_ui_;

  QHash<NodeInputPair, CollapseButton*> array_collapse_buttons_;

  /**
   * @brief The column to place the keyframe controls in
   *
   * Serves as an effective "maximum column" index because the keyframe button is always aligned
   * to the right edge.
   */
  static const int kKeyControlColumn;

  static const int kArrayInsertColumn;
  static const int kArrayRemoveColumn;

  static const int kWidgetStartColumn;

private slots:
  void EdgeChanged(const NodeOutput &output, const NodeInput &input);

  void ArrayCollapseBtnPressed(bool checked);

  void InputArraySizeChanged(const QString &input, int old_sz, int size);

  void ArrayAppendClicked();

  void ArrayInsertClicked();

  void ArrayRemoveClicked();

  void ToggleArrayExpanded();

  void ReplaceWidgets(const NodeInput& input);

};

class NodeParamViewItem : public QDockWidget
{
  Q_OBJECT
public:
  NodeParamViewItem(Node* node, QWidget* parent = nullptr);

  void SetTimeTarget(Node* target);

  void SetTime(const rational& time);

  // Set the timebase of the NodeParamViewItemBody
  void SetTimebase(const rational& timebase);

  Node* GetNode() const;

  bool IsExpanded() const;

  void SetHighlighted(bool e)
  {
    highlighted_ = e;

    update();
  }

  int GetElementY(const NodeInput& c) const;

public slots:
  void SetExpanded(bool e);

  void ToggleExpanded();

signals:
  void RequestSetTime(const rational& time);

  void RequestSelectNode(const QVector<Node*>& node);

  void PinToggled(bool e);

  void ExpandedChanged(bool e);

  void ArrayExpandedChanged(bool e);

  void Moved();

protected:
  virtual void changeEvent(QEvent *e) override;

  virtual void paintEvent(QPaintEvent *event) override;

  virtual void moveEvent(QMoveEvent *event) override;

private:
  NodeParamViewItemTitleBar* title_bar_;

  NodeParamViewItemBody* body_;

  Node* node_;

  rational time_;

  bool highlighted_;

private slots:
  void Retranslate();

};

}

#endif // NODEPARAMVIEWITEM_H
