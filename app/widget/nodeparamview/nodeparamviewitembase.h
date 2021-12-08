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

#ifndef NODEPARAMVIEWITEMBASE_H
#define NODEPARAMVIEWITEMBASE_H

#include <QDockWidget>

#include "nodeparamviewitemtitlebar.h"
#include "node/node.h"

namespace olive {

class NodeParamViewItemBase : public QDockWidget
{
  Q_OBJECT
public:
  NodeParamViewItemBase(QWidget* parent = nullptr);

  void SetHighlighted(bool e)
  {
    highlighted_ = e;

    update();
  }

  bool IsExpanded() const;

  static QString GetTitleBarTextFromNode(Node *n);

public slots:
  void SetExpanded(bool e);

  void ToggleExpanded()
  {
    SetExpanded(!IsExpanded());
  }

signals:
  void PinToggled(bool e);

  void ExpandedChanged(bool e);

  void Moved();

protected:
  void SetBody(QWidget *body);

  virtual void paintEvent(QPaintEvent *event) override;

  NodeParamViewItemTitleBar* title_bar() const
  {
    return title_bar_;
  }

  virtual void changeEvent(QEvent *e) override;

  virtual void moveEvent(QMoveEvent *event) override;

protected slots:
  virtual void Retranslate(){}

private:
  NodeParamViewItemTitleBar* title_bar_;

  QWidget *body_;

  QWidget *hidden_body_;

  bool highlighted_;

};

}

#endif // NODEPARAMVIEWITEMBASE_H
