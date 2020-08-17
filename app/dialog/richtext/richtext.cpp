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

RichTextDialog::RichTextDialog(QString start, QWidget* parent) :
  QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Create toolbar
  QHBoxLayout* toolbar_layout = new QHBoxLayout();

  bold_btn_ = CreateToolbarButton(tr("B"), tr("Bold"), {QStringLiteral("b"), QStringLiteral("strong")});
  toolbar_layout->addWidget(bold_btn_);
  italic_btn_ = CreateToolbarButton(tr("I"), tr("Italic"), {QStringLiteral("i"), QStringLiteral("em")});
  toolbar_layout->addWidget(italic_btn_);
  underline_btn_ = CreateToolbarButton(tr("U"), tr("Underline"), {QStringLiteral("u")});
  toolbar_layout->addWidget(underline_btn_);
  strikeout_btn_ = CreateToolbarButton(tr("S"), tr("Strikethrough"), {QStringLiteral("strike")});
  toolbar_layout->addWidget(strikeout_btn_);
  font_combo_ = new QFontComboBox();
  font_combo_->setToolTip(tr("Font Family"));
  toolbar_layout->addWidget(font_combo_);
  size_slider_ = new FloatSlider();
  size_slider_->SetMinimum(0.1);
  size_slider_->SetLadderElementCount(1);
  size_slider_->setToolTip(tr("Font Size"));
  toolbar_layout->addWidget(size_slider_);

  toolbar_layout->addStretch();

  left_align_btn_ = CreateToolbarButton(tr("L"), tr("Left Align"), {});
  toolbar_layout->addWidget(left_align_btn_);
  center_align_btn_ = CreateToolbarButton(tr("C"), tr("Center Align"), {});
  toolbar_layout->addWidget(center_align_btn_);
  right_align_btn_ = CreateToolbarButton(tr("R"), tr("Right Align"), {});
  toolbar_layout->addWidget(right_align_btn_);
  justify_align_btn_ = CreateToolbarButton(tr("J"), tr("Justify Align"), {});
  toolbar_layout->addWidget(justify_align_btn_);

  layout->addLayout(toolbar_layout);

  // Create text edit widget
  text_edit_ = new QTextEdit();
  text_edit_->setWordWrapMode(QTextOption::NoWrap);
  connect(text_edit_, &QTextEdit::cursorPositionChanged, this, &RichTextDialog::UpdateButtons);
  start.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
  text_edit_->document()->setPlainText(start);
  layout->addWidget(text_edit_);

  // Create buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, &RichTextDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &RichTextDialog::reject);

  // Connect font buttons
  /*
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
  */
}

QPushButton *RichTextDialog::CreateToolbarButton(const QString& label, const QString& tooltip, const QStringList &tags)
{
  QPushButton* btn = new QPushButton(label);
  btn->setCheckable(true);
  btn->setToolTip(tooltip);
  btn->setFixedWidth(btn->sizeHint().height());

  if (!tags.isEmpty()) {
    btn->setProperty("tag", tags);
    connect(btn, &QPushButton::clicked, this, &RichTextDialog::TagButtonToggled);
  }

  return btn;
}

int SnapPositionOutsideTags(const QString& text, int pos)
{
  // Look for closest opening bracket before position
  int opening_bracket_pos = text.lastIndexOf('<', pos - text.size() -1);

  // Look for closest closing bracket before position
  int closing_bracket_pos = text.indexOf('>', opening_bracket_pos);

  if (opening_bracket_pos > -1 && closing_bracket_pos >= pos) {
    // Must be inside an angle bracket, snap to closest position outside of bracket
    closing_bracket_pos++;

    if (pos - opening_bracket_pos < closing_bracket_pos - pos) {
      // Closer to opening bracket pos
      return opening_bracket_pos;
    } else {
      return closing_bracket_pos;
    }
  }

  return pos;
}

void RichTextDialog::SetTags(const QStringList &t, bool enabled)
{
  QString s = text_edit_->toPlainText();

  int selection_start, selection_end;

  {
    QTextCursor c = text_edit_->textCursor();

    if (c.hasSelection()) {
      selection_start = SnapPositionOutsideTags(s, c.selectionStart());
      selection_end = SnapPositionOutsideTags(s, c.selectionEnd());

      c.clearSelection();
      c.setPosition(selection_start, QTextCursor::MoveAnchor);
      c.setPosition(selection_end, QTextCursor::KeepAnchor);
    } else {
      selection_start = SnapPositionOutsideTags(s, c.position());
      selection_end = selection_start;
      c.setPosition(selection_start, QTextCursor::MoveAnchor);
    }

    text_edit_->setTextCursor(c);
  }

  QString open_tag = CreateOpeningTag(t.first());
  QString close_tag = CreateClosingTag(t.first());

  // Insert tags
  QString new_text;

  if (!enabled) {
    std::swap(open_tag, close_tag);
  }

  bool open_tag_cancels_out = !QString::compare(s.mid(selection_start - close_tag.size(), close_tag.size()), close_tag, Qt::CaseInsensitive);
  bool close_tag_cancels_out = !QString::compare(s.mid(selection_end, open_tag.size()), open_tag, Qt::CaseInsensitive);

  QString selected_text = text_edit_->textCursor().selectedText();

  if (open_tag_cancels_out && close_tag_cancels_out) {

    // Both tags cancel each other out, simply remove
    selection_start -= close_tag.size();

    QTextCursor c = text_edit_->textCursor();
    c.clearSelection();
    c.setPosition(selection_start, QTextCursor::MoveAnchor);
    c.setPosition(selection_end + open_tag.size(), QTextCursor::KeepAnchor);
    text_edit_->setTextCursor(c);

    selection_end -= close_tag.size();

    new_text = selected_text;

  } else if (open_tag_cancels_out) {

    // Open tag cancels out, shift close tag rather than inserting new tags
    selection_start -= close_tag.size();

    QTextCursor c = text_edit_->textCursor();
    c.clearSelection();
    c.setPosition(selection_start, QTextCursor::MoveAnchor);
    c.setPosition(selection_end, QTextCursor::KeepAnchor);
    text_edit_->setTextCursor(c);

    selection_end -= close_tag.size();

    new_text = selected_text;
    new_text.append(close_tag);

  } else if (close_tag_cancels_out) {

    // Close tag cancels out, shift open tag rather than inserting new tags
    selection_end += open_tag.size();

    QTextCursor c = text_edit_->textCursor();
    c.clearSelection();
    c.setPosition(selection_start, QTextCursor::MoveAnchor);
    c.setPosition(selection_end, QTextCursor::KeepAnchor);
    text_edit_->setTextCursor(c);

    selection_start += open_tag.size();

    new_text = open_tag;
    new_text.append(selected_text);

  } else {
    // Nothing is cancelled out, simply insert tags
    new_text = QStringLiteral("%1%2%3").arg(open_tag,
                                            selected_text,
                                            close_tag);

    selection_start += open_tag.size();
    selection_end += open_tag.size();
  }

  text_edit_->insertPlainText(new_text);

  text_edit_->setFocus();

  {
    // Re-select text
    QTextCursor c = text_edit_->textCursor();

    c.clearSelection();
    c.setPosition(selection_start, QTextCursor::MoveAnchor);
    c.setPosition(selection_end, QTextCursor::KeepAnchor);
    text_edit_->setTextCursor(c);
  }
}

QString RichTextDialog::CreateOpeningTag(const QString &s)
{
  return QStringLiteral("<%1>").arg(s);
}

QString RichTextDialog::CreateClosingTag(const QString &s)
{
  return QStringLiteral("</%1>").arg(s);
}

void RichTextDialog::UpdateTagButton(QPushButton *btn,
                                     const QString &text,
                                     int cursor_pos)
{
  QStringList tags = btn->property("tag").toStringList();
  foreach (const QString& t, tags) {
    QString opening = CreateOpeningTag(t);
    QString closing = CreateClosingTag(t);

    int opening_index = text.lastIndexOf(opening,
                                         cursor_pos - text.size() - 1,
                                         Qt::CaseInsensitive);
    int closing_index = text.indexOf(closing,
                                     opening_index,
                                     Qt::CaseInsensitive);

    if (opening_index > -1 && closing_index + closing.size() > cursor_pos) {
      btn->setChecked(true);
      btn->setProperty("foundtag", t);
      return;
    }
  }

  btn->setChecked(false);
  btn->setProperty("foundtag", QVariant());
}

void RichTextDialog::TagButtonToggled(bool checked)
{
  QPushButton* src = static_cast<QPushButton*>(sender());
  QStringList tags;

  if (src->property("foundtag").isNull()) {
    tags = src->property("tag").toStringList();
  } else {
    tags = QStringList({src->property("foundtag").toString()});
  }

  SetTags(tags, checked);
}

void RichTextDialog::UpdateButtons()
{
  QString text = text_edit_->toPlainText();
  int cursor_pos = text_edit_->textCursor().position();

  UpdateTagButton(bold_btn_, text, cursor_pos);
  UpdateTagButton(italic_btn_, text, cursor_pos);
  UpdateTagButton(underline_btn_, text, cursor_pos);
  UpdateTagButton(strikeout_btn_, text, cursor_pos);

  /*

  // Update font family
  font_combo_->blockSignals(true);
  font_combo_->setCurrentFont(text_edit_->currentFont().family());
  font_combo_->blockSignals(false);

  size_slider_->SetValue(text_edit_->fontPointSize());

  left_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignLeft);
  center_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignCenter);
  right_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignRight);
  justify_align_btn_->setChecked(text_edit_->alignment() == Qt::AlignJustify);
  */
}

OLIVE_NAMESPACE_EXIT
