/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef MESSAGEHIGHLIGHTER_H
#define MESSAGEHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class QTextDocument;

namespace olive {

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class MessageSyntaxHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT
public:

  MessageSyntaxHighlighter( QTextDocument * parent = nullptr);

  // QSyntaxHighlighter interface
protected:
  void highlightBlock(const QString &text) override;

private:
  QTextCharFormat group_format_;
  QTextCharFormat line_format_;

  QRegularExpression group_regexp;
  QRegularExpression line_regexp;
};


}  // namespace olive

#endif // MESSAGEHIGHLIGHTER_H
