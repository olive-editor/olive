/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef TEXTEDITDIALOG_H
#define TEXTEDITDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>

class TextEditDialog : public QDialog {
  Q_OBJECT
public:
  TextEditDialog(QWidget* parent = nullptr, const QString& s = nullptr);
  const QString& get_string();
private slots:
  void save();
  void cancel();
private:
  QString result_str;
  QTextEdit* textEdit;
};

#endif // TEXTEDITDIALOG_H
