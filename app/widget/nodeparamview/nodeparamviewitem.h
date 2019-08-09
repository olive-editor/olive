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
#include "nodeparamviewwidgetbridge.h"

class NodeParamViewItemTitleBar : public QWidget {
public:
  NodeParamViewItemTitleBar(QWidget* parent = nullptr);

protected:
  virtual void paintEvent(QPaintEvent *event) override;
};

class NodeParamViewItem : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewItem(QWidget* parent);

  void AttachNode(Node* n);

  bool CanAddNode(Node *n);

protected:
  void changeEvent(QEvent *e) override;

private:
  void SetupUI();

  void AddAdditionalNode(Node* n);

  bool expanded_;

  NodeParamViewItemTitleBar* title_bar_;

  QLabel* title_bar_lbl_;

  QPushButton* title_bar_collapse_btn_;

  QWidget* contents_;

  QGridLayout* content_layout_;

  QList<Node*> nodes_;

  QList<NodeParamViewWidgetBridge*> bridges_;

private slots:
  void SetExpanded(bool e);
};

#endif // NODEPARAMVIEWITEM_H
