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

// flag to indicate to not store info about matching parentheses.
// This is true for comments and string literals
const int IS_NON_PARSABLE = 0x1000;

const QString keywordPatterns[] = {
  QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"), QStringLiteral("\\benum\\b"),
  QStringLiteral("\\bbreak\\b"), QStringLiteral("\\bshort\\b"), QStringLiteral("\\bif\\b"),
  QStringLiteral("\\bfor\\b"), QStringLiteral("\\bwhile\\b"), QStringLiteral("\\bstruct\\b"),
  QStringLiteral("\\belse\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\btypedef\\b"),
  QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bfrag_color\\b"), QStringLiteral("\\bswitch\\b"),
  QStringLiteral("\\bcase\\b"), QStringLiteral("\\btrue\\b"), QStringLiteral("\\bfalse\\b"),
  QStringLiteral("\\bstatic\\b"), QStringLiteral("\\bdefault\\b"), QStringLiteral("#define\\b"),
  QStringLiteral("\\bove_texcoord\\b"), QStringLiteral("\\bin\\b"), QStringLiteral("\\bout\\b"),
  QStringLiteral("\\bresolution_in\\b"), QStringLiteral("#version\\b")
};

const QString typePatterns[] = {
  QStringLiteral("\\bchar\\b"),
  QStringLiteral("\\bdouble\\b"), QStringLiteral("\\bfloat\\b"), QStringLiteral("\\blong\\b"),
  QStringLiteral("\\bshort\\b"),QStringLiteral("\\bsigned\\b"), QStringLiteral("\\bunsigned\\b"),
  QStringLiteral("\\bunion\\b"), QStringLiteral("\\bvoid\\b"),QStringLiteral("\\bbool\\b"),
  QStringLiteral("\\buniform\\b"), QStringLiteral("\\bvec\\b"),
  QStringLiteral("\\bvec2\\b"), QStringLiteral("\\bvec3\\b"), QStringLiteral("\\bvec4\\b"),
  QStringLiteral("\\bint\\b"), QStringLiteral("\\bsampler2D\\b")
};

// a small set of built-in functions
const QString functionPatterns[] = {
  QStringLiteral("\\bmain\\b"), QStringLiteral("\\btexture2D\\b"), QStringLiteral("\\bfract\\b"),
  QStringLiteral("\\babs\\b"), QStringLiteral("\\bdot\\b"), QStringLiteral("\\bmix\\b"),
  QStringLiteral("\\bceil\\b"), QStringLiteral("\\bfloor\\b"), QStringLiteral("\\bclamp\\b"),
  QStringLiteral("\\bsin\\b"), QStringLiteral("\\bcos\\b"), QStringLiteral("\\btan\\b"),
  QStringLiteral("\\bexp(2)?\\b"), QStringLiteral("\\blog(2)?\\b"), QStringLiteral("\\b\\b"),
  QStringLiteral("\\bmin\\b"), QStringLiteral("\\bmax\\b"), QStringLiteral("\\bpow\\b"),
  QStringLiteral("\\bsqrt\\b"), QStringLiteral("\\bstep\\b"), QStringLiteral("\\bsmoothstep\\b"),
  QStringLiteral("\\bdistance\\b")
};

const QString oliveMarkupPatterns[] = {
  QStringLiteral("//OVE\\s+shader_name:[^\n]*"), QStringLiteral("//OVE\\s+shader_description:[^\n]*"),
  QStringLiteral("//OVE\\s+shader_version:[^\n]*"), QStringLiteral("//OVE\\s+name:[^\n]*"),
  QStringLiteral("//OVE\\s+type:[^\n]*"), QStringLiteral("//OVE\\s+flag:[^\n]*"),
  QStringLiteral("//OVE\\s+values:[^\n]*"), QStringLiteral("//OVE\\s+description:[^\n]*"),
  QStringLiteral("//OVE\\s+default:[^\n]*"), QStringLiteral("//OVE shape: [^\n]*"),
  QStringLiteral("//OVE color: [^\n]*"), QStringLiteral("//OVE\\s+min:[^\n]*"),
  QStringLiteral("//OVE\\s+max:[^\n]*"), QStringLiteral("//OVE\\s+end\\b[^\n]*")
};

} // namespace

GlslHighlighter::GlslHighlighter(QTextDocument *parent)
  : QSyntaxHighlighter(parent)
{
  HighlightingRule rule;

  keyword_format_.setForeground(QColor(110,80,110));
  keyword_format_.setFontWeight(QFont::Bold);

  quotation_format_.setForeground(QColor(130,130,80));
  quotation_format_.setProperty( IS_NON_PARSABLE, QVariant::fromValue<bool>(true));
  rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
  rule.format = quotation_format_;
  highlighting_rules_.append(rule);

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

  function_format_.setForeground(QColor(120,110,30));

  for (const QString &pattern : functionPatterns) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = function_format_;
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

  comment_start_expression_ = QRegularExpression(QStringLiteral("/\\*"));
  comment_end_expression_ = QRegularExpression(QStringLiteral("\\*/"));

  single_line_comment_format_.setForeground(QColor(60,180,80));
  single_line_comment_format_.setProperty( IS_NON_PARSABLE, QVariant::fromValue<bool>(true));
  rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
  rule.format = single_line_comment_format_;
  highlighting_rules_.append(rule);

  markup_line_comment_format_.setForeground(QColor(70,120,130));
  markup_line_comment_format_.setProperty( IS_NON_PARSABLE, QVariant::fromValue<bool>(true));
  for (const QString &pattern : oliveMarkupPatterns) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = markup_line_comment_format_;
    highlighting_rules_.append(rule);
  }

  multiline_comment_format_.setForeground(QColor(60,180,80));
  multiline_comment_format_.setProperty( IS_NON_PARSABLE, QVariant::fromValue<bool>(true));
}


void GlslHighlighter::highlightBlock(const QString &text)
{
  highlightKeywords(text);
  highlightMultilineComments(text);

  storeParenthesisInfo(text);
}

void olive::GlslHighlighter::highlightKeywords(const QString &text)
{
  for (const HighlightingRule &rule : qAsConst(highlighting_rules_)) {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }
}


void olive::GlslHighlighter::highlightMultilineComments(const QString &text)
{
  int start_index = 0;

  setCurrentBlockState(0);

  if (previousBlockState() != 1) {
    start_index = text.indexOf(comment_start_expression_);
  }

  while (start_index >= 0) {
    QRegularExpressionMatch match = comment_end_expression_.match(text, start_index);
    int end_index = match.capturedStart();
    int commentLength = 0;
    if (end_index == -1) {
      setCurrentBlockState(1);
      commentLength = text.length() - start_index;
    } else {
      commentLength = end_index - start_index + match.capturedLength();
    }
    setFormat(start_index, commentLength, multiline_comment_format_);
    start_index = text.indexOf(comment_start_expression_, start_index + commentLength);
  }
}


void olive::GlslHighlighter::storeParenthesisInfo(const QString &text)
{
  static const QRegularExpression ParenthesisRegExp("[\\(\\)\\[\\]\\{\\}]{1}");

  /* the ownership of this will be passed to QTextBlock */
  TextBlockData *data = new TextBlockData;

  QRegularExpressionMatchIterator i = ParenthesisRegExp.globalMatch(text);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();

    // store parenthesis info only if not in comments or string literals
    if (format(match.capturedStart(0)).hasProperty(IS_NON_PARSABLE) == false) {

      ParenthesisInfo info;
      info.character = match.captured().toLatin1().at(0);
      info.position = match.capturedStart(0);
      data->insert( info);
    }
  }

  setCurrentBlockUserData(data);
}

}  // namespace olive

