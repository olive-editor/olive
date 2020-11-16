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

#include "nodeparamviewrichtext.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "dialog/richtext/richtext.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

NodeParamViewRichText::NodeParamViewRichText(QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);

  line_edit_ = new QTextEdit();
  connect(line_edit_, &QTextEdit::textChanged, this, &NodeParamViewRichText::InnerWidgetTextChanged);
  layout->addWidget(line_edit_);

  QPushButton* edit_btn = new QPushButton();
  edit_btn->setIcon(icon::ToolEdit);
  edit_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
  layout->addWidget(edit_btn);
  connect(edit_btn, &QPushButton::clicked, this, &NodeParamViewRichText::ShowRichTextDialog);
}

void NodeParamViewRichText::ShowRichTextDialog()
{
  RichTextDialog d(this->text(), this);
  if (d.exec() == QDialog::Accepted) {
    QString s = d.text();

    line_edit_->setText(s);
    emit textEdited(s);
  }
}

void NodeParamViewRichText::InnerWidgetTextChanged()
{
  emit textEdited(this->text());
}

OLIVE_NAMESPACE_EXIT
