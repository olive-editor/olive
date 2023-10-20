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

#include "messagehighlighter.h"

namespace olive {

MessageSyntaxHighlighter::MessageSyntaxHighlighter(QTextDocument *parent) :
  QSyntaxHighlighter(parent)
{
  group_format_.setForeground(QColor(110,110,255));
  group_format_.setFontWeight(QFont::Bold);

  line_format_.setForeground(QColor(128,128,128));
  line_format_.setFontItalic(true);

  shader_error_format_.setForeground(QColor(255,70,70));
  shader_warning_format_.setForeground(QColor(255,255,40));

  group_regexp = QRegularExpression(QStringLiteral("\"[^\"]*\""));
  line_regexp = QRegularExpression(QStringLiteral("line\\s\\d+:"));
  shader_error_regexp = QRegularExpression(QStringLiteral("\\b(error|ERROR|Error)\\b"));
  shader_warning_regexp = QRegularExpression(QStringLiteral("\\b(warning|WARNING|Warning)\\b"));
}

void MessageSyntaxHighlighter::highlightBlock(const QString &text)
{
  QRegularExpressionMatchIterator matchIterator = group_regexp.globalMatch(text);

  while (matchIterator.hasNext()) {
    QRegularExpressionMatch match = matchIterator.next();
    setFormat(match.capturedStart(), match.capturedLength(), group_format_);
  }

  matchIterator = line_regexp.globalMatch(text);

  while (matchIterator.hasNext()) {
    QRegularExpressionMatch match = matchIterator.next();
    setFormat(match.capturedStart(), match.capturedLength(), line_format_);
  }

  matchIterator = shader_error_regexp.globalMatch(text);

  while (matchIterator.hasNext()) {
    QRegularExpressionMatch match = matchIterator.next();
    setFormat(match.capturedStart(1), match.capturedLength(1), shader_error_format_);
  }

  matchIterator = shader_warning_regexp.globalMatch(text);

  while (matchIterator.hasNext()) {
    QRegularExpressionMatch match = matchIterator.next();
    setFormat(match.capturedStart(1), match.capturedLength(1), shader_warning_format_);
  }
}


}  // namespace olive

