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
  QWidget(parent),
  directory_mode_(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  layout->setMargin(0);

  line_edit_ = new QLineEdit();
  connect(line_edit_, &QLineEdit::textChanged, this, &FileField::LineEditChanged);
  connect(line_edit_, &QLineEdit::textEdited, this, &FileField::FilenameChanged);
  layout->addWidget(line_edit_);

  browse_btn_ = new QPushButton();
  browse_btn_->setIcon(icon::Open);
  connect(browse_btn_, &QPushButton::clicked, this, &FileField::BrowseBtnClicked);
  layout->addWidget(browse_btn_);
}

void FileField::BrowseBtnClicked()
{
  QString s;

  if (directory_mode_) {
    s = QFileDialog::getExistingDirectory(this, tr("Open Directory"));
  } else {
    s = QFileDialog::getOpenFileName(this, tr("Open File"));
  }

  if (!s.isEmpty()) {
    line_edit_->setText(s);
    emit FilenameChanged(s);
  }
}

void FileField::LineEditChanged(const QString& text)
{
  if (QFileInfo::exists(text) || text.isEmpty()) {
    line_edit_->setStyleSheet(QString());
  } else {
    line_edit_->setStyleSheet(QStringLiteral("QLineEdit {color: red;}"));
  }
}

}
