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

#include "filefield.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>

#include "ui/icons/icons.h"

namespace olive {

FileField::FileField(QWidget* parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  layout->setSpacing(0);
  layout->setMargin(0);

  line_edit_ = new QLineEdit();
  connect(line_edit_, &QLineEdit::textChanged, this, &FileField::LineEditChanged);
  layout->addWidget(line_edit_);

  browse_btn_ = new QPushButton();
  browse_btn_->setIcon(icon::Open);
  connect(browse_btn_, &QPushButton::clicked, this, &FileField::BrowseBtnClicked);
  layout->addWidget(browse_btn_);
}

void FileField::BrowseBtnClicked()
{
  QString s = QFileDialog::getOpenFileName(this, tr("Open File"));

  if (!s.isEmpty()) {
    line_edit_->setText(s);
  }
}

void FileField::LineEditChanged(const QString& text)
{
  if (QFileInfo::exists(text)) {
    line_edit_->setStyleSheet(QString());
  } else {
    line_edit_->setStyleSheet(QStringLiteral("QLineEdit {color: red;}"));
  }

  emit FilenameChanged(text);
}

}
