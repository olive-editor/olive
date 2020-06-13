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

#include "richtext.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

RichTextDialog::RichTextDialog(const QString &start, QWidget* parent) :
  QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Create toolbar
  QHBoxLayout* toolbar_layout = new QHBoxLayout();

  bold_btn_ = new QPushButton(QStringLiteral("B"));
  bold_btn_->setCheckable(true);
  toolbar_layout->addWidget(bold_btn_);
  italic_btn_ = new QPushButton(QStringLiteral("I"));
  italic_btn_->setCheckable(true);
  toolbar_layout->addWidget(italic_btn_);
  underline_btn_ = new QPushButton(QStringLiteral("U"));
  underline_btn_->setCheckable(true);
  toolbar_layout->addWidget(underline_btn_);
  font_combo_ = new QFontComboBox();
  toolbar_layout->addWidget(font_combo_);
  size_slider_ = new FloatSlider();
  size_slider_->SetMinimum(0.1);
  toolbar_layout->addWidget(size_slider_);

  toolbar_layout->addStretch();

  layout->addLayout(toolbar_layout);

  // Create text edit widget
  text_edit_ = new QTextEdit();
  connect(text_edit_, &QTextEdit::cursorPositionChanged, this, &RichTextDialog::UpdateButtons);
  text_edit_->document()->setHtml(start);
  layout->addWidget(text_edit_);

  // Create buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, &RichTextDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &RichTextDialog::reject);

  // Connect font buttons
  connect(bold_btn_, &QPushButton::clicked, this, [this](bool e){
    text_edit_->setFontWeight(e ? QFont::Bold : QFont::Normal);
  });
  connect(italic_btn_, &QPushButton::clicked, text_edit_, &QTextEdit::setFontItalic);
  connect(underline_btn_, &QPushButton::clicked, text_edit_, &QTextEdit::setFontUnderline);
  connect(size_slider_, &FloatSlider::ValueChanged, text_edit_, &QTextEdit::setFontPointSize);

  connect(font_combo_, &QFontComboBox::currentTextChanged, this, [this](const QString& s){
    QTextCharFormat fmt;
    fmt.setFontFamily(s);

    QTextCursor cursor = text_edit_->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(fmt);
    text_edit_->mergeCurrentCharFormat(fmt);
  });
}

void RichTextDialog::UpdateButtons()
{
  bold_btn_->setChecked(text_edit_->fontWeight() > QFont::Normal);
  italic_btn_->setChecked(text_edit_->fontItalic());
  underline_btn_->setChecked(text_edit_->fontUnderline());

  font_combo_->blockSignals(true);
  QString font_family = text_edit_->fontFamily();
  if (font_family.isEmpty()) {
    font_family = QFont().family();
  }
  font_combo_->setCurrentFont(font_family);
  font_combo_->blockSignals(false);

  size_slider_->SetValue(text_edit_->fontPointSize());
}

OLIVE_NAMESPACE_EXIT
