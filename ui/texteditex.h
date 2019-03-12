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

#ifndef TEXTEDITEX_H
#define TEXTEDITEX_H

#include <QTextEdit>

/*
class TextEditEx : public QTextEdit {
  Q_OBJECT
public:
  TextEditEx(QWidget* parent = 0);
  void setPlainTextEx(const QString &text);
  const QString& getPreviousValue();
  const QString& getPlainTextEx();
signals:
  void updateSelf();
private slots:
  void updateInternals();
  void updateText();
private:
  QString previousText;
  QString text;
};
*/

class TextEditEx : public QTextEdit {
public:
  TextEditEx(QWidget* parent);
private slots:
  void text_edit_menu();
  void open_text_edit();
};

#endif // TEXTEDITEX_H
