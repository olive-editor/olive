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

#include "nodeviewfilter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLayoutItem>
#include <QPushButton>
#include <QVBoxLayout>

#define FILTER_COLUMN_COUNT 7

OLIVE_NAMESPACE_ENTER

NodeViewFilterDialog::NodeViewFilterDialog(QWidget *parent) :
  QDialog(parent),
  row_count_(0)
{
  setWindowTitle(tr("Configure Node Filters"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  {
    QGroupBox* filter_group = new QGroupBox();
    filter_group->setTitle(tr("Filters"));

    QVBoxLayout* filter_outer_layout = new QVBoxLayout(filter_group);
    filter_layout_ = new QGridLayout();
    filter_layout_->setMargin(0);
    filter_outer_layout->addLayout(filter_layout_);
    filter_outer_layout->addStretch();

    layout->addWidget(filter_group);
  }

  plus_btn_ = new QPushButton(tr("+"));
  plus_btn_->setFixedWidth(plus_btn_->sizeHint().height());
  connect(plus_btn_, &QPushButton::clicked, this, &NodeViewFilterDialog::AppendRow);

  AppendRow();

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);
}

void NodeViewFilterDialog::AppendRow()
{
  QCheckBox* enabled_check = new QCheckBox();
  enabled_check->setChecked(true);
  filter_layout_->addWidget(enabled_check, row_count_, 0);

  QComboBox* showhide_combo = new QComboBox();
  showhide_combo->addItem(tr("Show"));
  showhide_combo->addItem(tr("Hide"));
  filter_layout_->addWidget(showhide_combo, row_count_, 1);

  QComboBox* type_combo = new QComboBox();
  type_combo->addItem(tr("Any"));
  type_combo->addItem(tr("Selected"));
  filter_layout_->addWidget(type_combo, row_count_, 2);

  QComboBox* from_combo = new QComboBox();
  from_combo->addItem(tr("From Node"));
  from_combo->addItem(tr("From Category"));
  filter_layout_->addWidget(from_combo, row_count_, 3);

  QComboBox* from_entry_combo = new QComboBox();
  filter_layout_->addWidget(from_entry_combo, row_count_, 4);

  QComboBox* to_entry_combo = new QComboBox();
  filter_layout_->addWidget(to_entry_combo, row_count_, 5);

  QPushButton* minus_btn = new QPushButton(tr("-"));
  minus_btn->setFixedWidth(minus_btn->sizeHint().height());
  connect(minus_btn, &QPushButton::clicked, this, &NodeViewFilterDialog::RemoveRowButtonClicked);
  filter_layout_->addWidget(minus_btn, row_count_, 6);

  row_count_++;

  UpdateAddButtonPos();
}

void NodeViewFilterDialog::RemoveRowButtonClicked()
{
  for (int i=0;i<row_count_;i++) {
    if (filter_layout_->itemAtPosition(i, FILTER_COLUMN_COUNT-1)->widget() == sender()) {
      RemoveRow(i);
      return;
    }
  }
}

void NodeViewFilterDialog::RemoveRow(int row)
{
  for (int i=0;i<FILTER_COLUMN_COUNT;i++) {
    delete filter_layout_->itemAtPosition(row, i)->widget();
  }

  for (int j=row+1;j<row_count_;j++) {
    for (int i=0;i<FILTER_COLUMN_COUNT;i++) {
      QWidget* w = filter_layout_->itemAtPosition(j, i)->widget();

      filter_layout_->addWidget(w, j - 1, i);
    }
  }

  row_count_--;

  UpdateAddButtonPos();
}

void NodeViewFilterDialog::UpdateAddButtonPos()
{
  filter_layout_->addWidget(plus_btn_, row_count_, FILTER_COLUMN_COUNT - 1);
}

OLIVE_NAMESPACE_EXIT
