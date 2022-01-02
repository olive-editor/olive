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

#ifndef NODEPARAMVIEWCONTEXT_H
#define NODEPARAMVIEWCONTEXT_H

#include "nodeparamviewdockarea.h"
#include "nodeparamviewitembase.h"
#include "nodeparamviewitem.h"

namespace olive {

class NodeParamViewContext : public NodeParamViewItemBase
{
  Q_OBJECT
public:
  NodeParamViewContext(QWidget *parent = nullptr);

  NodeParamViewDockArea *GetDockArea() const
  {
    return dock_area_;
  }

  const QVector<Node*> &GetContexts() const
  {
    return contexts_;
  }

  const QMap<Node*, NodeParamViewItem*> &GetItems() const
  {
    return items_;
  }

  void AddNode(NodeParamViewItem *item);

  void RemoveNode(Node *node);

  void Clear();

  void SetInputChecked(const NodeInput &input, bool e);

  void SetTimebase(const rational &timebase);

  void SetTimeTarget(Node *n);

  void SetTime(const rational &time);

public slots:
  void AddContext(Node *node)
  {
    contexts_.append(node);
  }

  void RemoveContext(Node *node)
  {
    contexts_.removeOne(node);
  }

protected slots:
  virtual void Retranslate() override;

private:
  NodeParamViewDockArea *dock_area_;

  QVector<Node*> contexts_;

  QMap<Node*, NodeParamViewItem*> items_;

private slots:
  void AddEffectButtonClicked();

};

}

#endif // NODEPARAMVIEWCONTEXT_H
