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

  bold_btn_ = CreateToolbarButton(tr("B"), tr("Bold"));
  toolbar_layout->addWidget(bold_btn_);
  italic_btn_ = CreateToolbarButton(tr("I"), tr("Italic"));
  toolbar_layout->addWidget(italic_btn_);
  underline_btn_ = CreateToolbarButton(tr("U"), tr("Underline"));
  toolbar_layout->addWidget(underline_btn_);
  strikeout_btn_ = CreateToolbarButton(tr("S"), tr("Strikethrough"));
  toolbar_layout->addWidget(strikeout_btn_);
  font_combo_ = new QFontComboBox();
  font_combo_->setToolTip(tr("Font Family"));
  toolbar_layout->addWidget(font_combo_);
  size_slider_ = new FloatSlider();
  size_slider_->SetMinimum(0.1);
  size_slider_->setToolTip(tr("Font Size"));
  toolbar_layout->addWidget(size_slider_);

  toolbar_layout->addStretch();

  left_align_btn_ = CreateToolbarButton(tr("L"), tr("Left Align"));
  toolbar_layout->addWidget(left_align_btn_);
  center_align_btn_ = CreateToolbarButton(tr("C"), tr("Center Align"));
  toolbar_layout->addWidget(center_align_btn_);
  right_align_btn_ = CreateToolbarButton(tr("R"), tr("Right Align"));
  toolbar_layout->addWidget(right_align_btn_);
  justify_align_btn_ = CreateToolbarButton(tr("J"), tr("Justify Align"));
  toolbar_layout->addWidget(justify_align_btn_);

  layout->addLayout(toolbar_layout);

  // Create text edit widget
  text_edit_ = new QTextEdit();
  text_edit_->setWordWrapMode(QTextOption::NoWrap);
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
  connect(strikeout_btn_, &QPushButton::clicked, this, [this](bool e){
    QFont current_font = text_edit_->currentFont();
    current_font.setStrikeOut(e);
    text_edit_->setCurrentFont(current_font);
  });
  connect(size_slider_, &FloatSlider::ValueChanged, text_edit_, &QTextEdit::setFontPointSize);
  connect(left_align_btn_, &QPushButton::clicked, this, [this](){
    text_edit_->setAlignment(Qt::AlignLeft);
    UpdateButtons();
  });
  connect(center_align_btn_, &QPushButton::clicked, this, [this](){
    text_edit_->setAlignment(Qt::AlignCenter);
    UpdateButtons();
  });
  connect(right_align_btn_, &QPushButton::clicked, this, [this](){
    text_edit_->setAlignment(Qt::AlignRight);
    UpdateButtons();
  });
  connect(justify_align_btn_, &QPushButton::clicked, this, [this](){
    text_edit_->setAlignment(Qt::AlignJustify);
    UpdateButtons();
  });


  connect(font_combo_, &QFontComboBox::currentTextChanged, this, [this](const QString& s){
    text_edit_->setFontFamily(s);
  });
}

QPushButton *RichTextDialog::CreateToolbarButton(const QString& label, const QString& tooltip)
{
  QPushButton* btn = new QPushButton(label);
  btn->setCheckable(true);
  btn->setToolTip(tooltip);
  btn->setFixedWidth(btn->sizeHint().height());
  return btn;
}

void RichTextDialog::UpdateButtons()
{
  bold_btn_->setChecked(text_edit_->fontWeight() > QFont::Normal);
  italic_btn_->setChecked(text_edit_->fontItalic());
  underline_btn_->setChecked(text_edit_->fontUnderline());
  strikeout_btn_->setChecked(text_edit_->currentFont().strikeOut());

  // Update font family
  font_combo_->blockSignals(true);
  font_combo_->setCurrentFont(text_edit_->currentFont().family());
  font_combo_->blockSignals(false);

  size_slider_->SetValue(text_edit_->fontPointSize());

  left_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignLeft);
  center_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignCenter);
  right_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignRight);
  justify_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignJustify);
}

OLIVE_NAMESPACE_EXIT
