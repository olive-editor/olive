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

#include "pathwidget.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

PathWidget::PathWidget(const QString &path, QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);

  path_edit_ = new QLineEdit();
  path_edit_->setText(path);
  layout->addWidget(path_edit_);
  connect(path_edit_, &QLineEdit::textChanged, this, &PathWidget::LineEditChanged);

  browse_btn_ = new QPushButton(tr("Browse"));
  layout->addWidget(browse_btn_);

  connect(browse_btn_, &QPushButton::clicked, this, &PathWidget::BrowseClicked);
}

void PathWidget::BrowseClicked()
{
  QString dir = QFileDialog::getExistingDirectory(static_cast<QWidget*>(parent()),
                                                  tr("Browse for path"),
                                                  path_edit_->text());

  if (!dir.isEmpty()) {
    path_edit_->setText(dir);
  }
}

void PathWidget::LineEditChanged()
{
  if (FileFunctions::DirectoryIsValid(text(), false)) {
    path_edit_->setStyleSheet(QString());
  } else {
    path_edit_->setStyleSheet(QStringLiteral("QLineEdit {color: red;}"));
  }
}

OLIVE_NAMESPACE_EXIT
