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

#ifndef RICHTEXTDIALOG_H
#define RICHTEXTDIALOG_H

#include <QDialog>
#include <QFontComboBox>
#include <QTextEdit>

#include "common/define.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

class RichTextDialog : public QDialog
{
  Q_OBJECT
public:
  RichTextDialog(QString start, QWidget* parent = nullptr);

  QString text() const
  {
    QString s = text_edit_->document()->toPlainText();

    // Convert linebreaks
    s.replace('\n', QStringLiteral("<br>"));

    return s;
  }

private:
  QPushButton* CreateToolbarButton(const QString &label,
                                   const QString &tooltip,
                                   const QStringList& tags);

  void SetTags(const QStringList& t, bool enabled);

  static QString CreateOpeningTag(const QString& s);
  static QString CreateClosingTag(const QString& s);

  static void UpdateTagButton(QPushButton* btn,
                              const QString &text,
                              int cursor_pos);

  QFontDatabase font_db_;

  QTextEdit* text_edit_;

  QPushButton* bold_btn_;
  QPushButton* italic_btn_;
  QPushButton* underline_btn_;
  QPushButton* strikeout_btn_;
  QFontComboBox* font_combo_;
  FloatSlider* size_slider_;
  QPushButton* left_align_btn_;
  QPushButton* center_align_btn_;
  QPushButton* right_align_btn_;
  QPushButton* justify_align_btn_;

private slots:
  void TagButtonToggled(bool checked);

  void UpdateButtons();

};

OLIVE_NAMESPACE_EXIT

#endif // RICHTEXTDIALOG_H
