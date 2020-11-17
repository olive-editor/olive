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

OLIVE_NAMESPACE_ENTER

NodeParamViewArrayWidget::NodeParamViewArrayWidget(NodeInputArray* array, QWidget* parent) :
  QWidget(parent),
  array_(array)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  count_lbl_ = new QLabel();
  layout->addWidget(count_lbl_);

  layout->addStretch();

  plus_btn_ = new QPushButton(tr("+"));
  plus_btn_->setFixedWidth(plus_btn_->sizeHint().height());
  layout->addWidget(plus_btn_);

  connect(plus_btn_, &QPushButton::clicked, this, &NodeParamViewArrayWidget::AddElement);
  connect(array_, &NodeInputArray::SizeChanged, this, &NodeParamViewArrayWidget::UpdateCounter);

  UpdateCounter();
}

void NodeParamViewArrayWidget::UpdateCounter()
{
  count_lbl_->setText(tr("%1 elements").arg(array_->GetSize()));
}

void NodeParamViewArrayWidget::AddElement()
{
  array_->Append();
}

OLIVE_NAMESPACE_EXIT
