/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GLSLHIGHLIGHTER_H
#define GLSLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>


class QTextDocument;

namespace olive {

class GlslHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

public:
  GlslHighlighter(QTextDocument *parent = 0);

protected:
  void highlightBlock(const QString &text) override;

private:
  struct HighlightingRule
  {
    QRegularExpression pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlighting_rules_;

  QRegularExpression comment_start_expression_;
  QRegularExpression comment_end_expression_;

  QTextCharFormat keyword_format_;
  QTextCharFormat type_format_;
  QTextCharFormat single_line_comment_format_;
  QTextCharFormat markup_line_comment_format_;
  QTextCharFormat multiline_comment_format_;
  QTextCharFormat quotation_format_;
  QTextCharFormat number_format_;
  QTextCharFormat function_format_;

private:
  void highlightKeywords(const QString &text);
  void highlightMultilineComments(const QString &text);
  void storeParenthesisInfo(const QString &text);
};


struct ParenthesisInfo
{
  char character;
  int position;
};

/* utility class to store brakets info */
class TextBlockData : public QTextBlockUserData
{
public:
  TextBlockData() {
  }

  const QVector<ParenthesisInfo> & parentheses() {
    return m_parentheses;
  }

  void insert(ParenthesisInfo & info) {
    int i = 0;
    while (i < m_parentheses.size() &&
           info.position > m_parentheses.at(i).position)
    {
      ++i;
    }

    m_parentheses.insert(i, info);
  }

private:
  QVector<ParenthesisInfo> m_parentheses;
};

}  // namespace olive

#endif // GLSLHIGHLIGHTER_H
