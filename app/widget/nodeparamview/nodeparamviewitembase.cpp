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

#include "nodeparamviewitembase.h"

#include <QEvent>
#include <QPainter>

namespace olive {

#define super QDockWidget

NodeParamViewItemBase::NodeParamViewItemBase(QWidget *parent) :
  super(parent),
  highlighted_(false)
{
  // Create title bar widget
  title_bar_ = new NodeParamViewItemTitleBar(this);

  // Add title bar to widget
  this->setTitleBarWidget(title_bar_);

  // Connect title bar to this
  connect(title_bar_, &NodeParamViewItemTitleBar::ExpandedStateChanged, this, &NodeParamViewItemBase::SetExpanded);
  connect(title_bar_, &NodeParamViewItemTitleBar::PinToggled, this, &NodeParamViewItemBase::PinToggled);

  // Use dummy QWidget to retain width when not expanded (QDockWidget seems to ignore the titlebar
  // size hints and will shrink as small as possible if the body is hidden)
  hidden_body_ = new QWidget(this);

  setAutoFillBackground(true);

  setFocusPolicy(Qt::ClickFocus);
}

bool NodeParamViewItemBase::IsExpanded() const
{
  return title_bar_->IsExpanded();
}

QString NodeParamViewItemBase::GetTitleBarTextFromNode(Node *n)
{
  if (n->GetLabel().isEmpty()) {
    return n->Name();
  } else {
    return tr("%1 (%2)").arg(n->GetLabel(), n->Name());
  }
}

void NodeParamViewItemBase::SetBody(QWidget *body)
{
  body_ = body;
  body_->setParent(this);

  if (title_bar_->IsExpanded()) {
    setWidget(body_);
  }
}

void NodeParamViewItemBase::paintEvent(QPaintEvent *event)
{
  super::paintEvent(event);

  // Draw border if focused
  if (highlighted_) {
    QPainter p(this);
    p.setBrush(Qt::NoBrush);
    p.setPen(palette().highlight().color());
    p.drawRect(rect().adjusted(0, 0, -1, -1));
  }
}

void NodeParamViewItemBase::SetExpanded(bool e)
{
  setWidget(e ? body_ : hidden_body_);
  title_bar_->SetExpanded(e);

  emit ExpandedChanged(e);
}

void NodeParamViewItemBase::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }

  super::changeEvent(e);
}

void NodeParamViewItemBase::moveEvent(QMoveEvent *event)
{
  super::moveEvent(event);

  emit Moved();
}

}
