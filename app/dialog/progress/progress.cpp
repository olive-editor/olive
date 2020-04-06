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

#include "progress.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

OLIVE_NAMESPACE_ENTER

ProgressDialog::ProgressDialog(const QString& message, const QString& title, QWidget *parent) :
  QDialog(parent)
{
  if (!title.isEmpty()) {
    setWindowTitle(title);
  }

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(message));

  bar_ = new QProgressBar();
  bar_->setMinimum(0);
  bar_->setValue(0);
  bar_->setMaximum(100);
  layout->addWidget(bar_);

  QHBoxLayout* cancel_layout = new QHBoxLayout();
  layout->addLayout(cancel_layout);
  cancel_layout->setMargin(0);
  cancel_layout->setSpacing(0);
  cancel_layout->addStretch();

  QPushButton* cancel_btn = new QPushButton(tr("Cancel"));
  connect(cancel_btn, &QPushButton::clicked, this, &ProgressDialog::Cancelled);
  cancel_layout->addWidget(cancel_btn);

  cancel_layout->addStretch();
}

void ProgressDialog::SetProgress(int value)
{
  bar_->setValue(value);
}

OLIVE_NAMESPACE_EXIT
