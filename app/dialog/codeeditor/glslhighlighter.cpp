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

#include "glslhighlighter.h"

namespace olive {

namespace  {

const QString keywordPatterns[] = {
  QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"), QStringLiteral("\\benum\\b"),
  QStringLiteral("\\bbreak\\b"), QStringLiteral("\\bshort\\b"), QStringLiteral("\\bif\\b"),
  QStringLiteral("\\bfor\\b"), QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bstruct\\b"),
  QStringLiteral("\\belse\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\btypedef\\b"),
  QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bgl_FragColor\\b"), QStringLiteral("\\bswitch\\b"),
  QStringLiteral("\\btrue\\b"), QStringLiteral("\\bfalse\\b"), QStringLiteral("\\bstatic\\b"),
  QStringLiteral("\\bdefault\\b"), QStringLiteral("\\b#define\\b")
};

const QString typePatterns[] = {
  QStringLiteral("\\bchar\\b"),
  QStringLiteral("\\bdouble\\b"), QStringLiteral("\\bfloat\\b"), QStringLiteral("\\blong\\b"),
  QStringLiteral("\\bshort\\b"),QStringLiteral("\\bsigned\\b"), QStringLiteral("\\bunsigned\\b"),
  QStringLiteral("\\bunion\\b"), QStringLiteral("\\bvoid\\b"),QStringLiteral("\\bbool\\b"),
  QStringLiteral("\\buniform\\b"), QStringLiteral("\\bvarying\\b"), QStringLiteral("\\bvec\\b"),
  QStringLiteral("\\bvec2\\b"), QStringLiteral("\\bvec3\\b"), QStringLiteral("\\bvec4\\b"),
  QStringLiteral("\\bint\\b"), QStringLiteral("\\bsampler2D\\b")
};

const QString oliveMarkupPatterns[] = {
  QStringLiteral("//OVE\\s+shader_name:[^\n]*"), QStringLiteral("//OVE\\s+shader_description:[^\n]*"),
  QStringLiteral("//OVE\\s+shader_version:[^\n]*"), QStringLiteral("//OVE\\s+name:[^\n]*"),
  QStringLiteral("//OVE\\s+type:[^\n]*"), QStringLiteral("//OVE\\s+flag:[^\n]*"),
  QStringLiteral("//OVE\\s+description:[^\n]*"), QStringLiteral("//OVE\\s+default:[^\n]*"),
  QStringLiteral("//OVE\\s+min:[^\n]*"), QStringLiteral("//OVE\\s+max:[^\n]*"),
   QStringLiteral("//OVE\\s+end\\b[^\n]*")
};

}

GlslHighlighter::GlslHighlighter(QTextDocument *parent)
  : QSyntaxHighlighter(parent)
{
  HighlightingRule rule;

  keyword_format_.setForeground(QColor(110,80,110));
  keyword_format_.setFontWeight(QFont::Bold);

  for (const QString &pattern : keywordPatterns) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = keyword_format_;
    highlighting_rules_.append(rule);
  }

  type_format_.setForeground(QColor(70,110,160));

  for (const QString &pattern : typePatterns) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = type_format_;
    highlighting_rules_.append(rule);
  }

  number_format_.setForeground(QColor(120,110,60));
  rule.format = type_format_;
  // decimal or float
  rule.pattern = QRegularExpression("\\b\\d+\\.?(\\d+)?\\b");
  highlighting_rules_.append(rule);
  // hex
  rule.pattern = QRegularExpression("\\b0[xX][0-9A-Fa-f]+\\b");
  highlighting_rules_.append(rule);

  single_line_comment_format_.setForeground(QColor(60,180,80));
  rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
  rule.format = single_line_comment_format_;
  highlighting_rules_.append(rule);

  markup_line_comment_format_.setForeground(QColor(70,120,130));
  for (const QString &pattern : oliveMarkupPatterns) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = markup_line_comment_format_;
    highlighting_rules_.append(rule);
  }

  multiline_comment_format_.setForeground(QColor(60,180,80));

  quotation_format_.setForeground(QColor(130,130,80));
  rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
  rule.format = quotation_format_;
  highlighting_rules_.append(rule);

  comment_start_expression_ = QRegularExpression(QStringLiteral("/\\*"));
  comment_end_expression_ = QRegularExpression(QStringLiteral("\\*/"));
}


void GlslHighlighter::highlightBlock(const QString &text)
{
  for (const HighlightingRule &rule : qAsConst(highlighting_rules_)) {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }

  setCurrentBlockState(0);

  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = text.indexOf(comment_start_expression_);

  while (startIndex >= 0) {
    QRegularExpressionMatch match = comment_end_expression_.match(text, startIndex);
    int endIndex = match.capturedStart();
    int commentLength = 0;
    if (endIndex == -1) {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    } else {
      commentLength = endIndex - startIndex
          + match.capturedLength();
    }
    setFormat(startIndex, commentLength, multiline_comment_format_);
    startIndex = text.indexOf(comment_start_expression_, startIndex + commentLength);
  }
}

}  // namespace olive

