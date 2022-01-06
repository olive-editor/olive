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

#include "nodeparamviewitemtitlebar.h"

#include <QHBoxLayout>
#include <QPainter>

#include "ui/icons/icons.h"

namespace olive {

NodeParamViewItemTitleBar::NodeParamViewItemTitleBar(QWidget *parent) :
  QWidget(parent),
  draw_border_(true)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  collapse_btn_ = new CollapseButton();
  connect(collapse_btn_, &QPushButton::clicked, this, &NodeParamViewItemTitleBar::ExpandedStateChanged);
  layout->addWidget(collapse_btn_);

  lbl_ = new QLabel();
  layout->addWidget(lbl_);

  // Place next buttons on the far side
  layout->addStretch();

  add_fx_btn_ = new QPushButton();
  add_fx_btn_->setIcon(icon::AddEffect);
  add_fx_btn_->setFixedSize(add_fx_btn_->sizeHint().height(), add_fx_btn_->sizeHint().height());
  add_fx_btn_->setVisible(false);
  layout->addWidget(add_fx_btn_);
  connect(add_fx_btn_, &QPushButton::clicked, this, &NodeParamViewItemTitleBar::AddEffectButtonClicked);

  pin_btn_ = new QPushButton(QStringLiteral("P"));
  pin_btn_->setCheckable(true);
  pin_btn_->setFixedSize(pin_btn_->sizeHint().height(), pin_btn_->sizeHint().height());
  pin_btn_->setVisible(false);
  layout->addWidget(pin_btn_);
  connect(pin_btn_, &QPushButton::clicked, this, &NodeParamViewItemTitleBar::PinToggled);
}

void NodeParamViewItemTitleBar::SetExpanded(bool e)
{
  draw_border_ = e;
  collapse_btn_->setChecked(e);

  update();
}

void NodeParamViewItemTitleBar::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if (draw_border_) {
    QPainter p(this);

    // Draw bottom border using text color
    int bottom = height() - 1;
    p.setPen(palette().text().color());
    p.drawLine(0, bottom, width(), bottom);
  }
}

void NodeParamViewItemTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
  QWidget::mouseDoubleClickEvent(event);

  collapse_btn_->click();
}

}
