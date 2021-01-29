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

#include "nodeparamviewarraywidget.h"

#include <QHBoxLayout>

namespace olive {

NodeParamViewArrayWidget::NodeParamViewArrayWidget(NodeInput *array, QWidget* parent) :
  QWidget(parent),
  array_(array)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  count_lbl_ = new QLabel();
  layout->addWidget(count_lbl_);

  connect(array_, &NodeInput::ArraySizeChanged, this, &NodeParamViewArrayWidget::UpdateCounter);

  UpdateCounter();
}

void NodeParamViewArrayWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  QWidget::mouseDoubleClickEvent(event);

  emit DoubleClicked();
}

void NodeParamViewArrayWidget::UpdateCounter()
{
  count_lbl_->setText(tr("%1 element(s)").arg(array_->ArraySize()));
}

NodeParamViewArrayButton::NodeParamViewArrayButton(NodeParamViewArrayButton::Type type, QWidget *parent) :
  QPushButton(parent),
  type_(type)
{
  Retranslate();

  int sz = sizeHint().height() / 3 * 2;
  setFixedSize(sz, sz);
}

void NodeParamViewArrayButton::changeEvent(QEvent *event)
{
  if (event->type() == QEvent::LanguageChange) {
    Retranslate();
  }

  QPushButton::changeEvent(event);
}

void NodeParamViewArrayButton::Retranslate()
{
  if (type_ == kAdd) {
    setText(tr("+"));
  } else {
    setText(tr("-"));
  }
}

}
