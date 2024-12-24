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

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextBlock>

namespace olive {

class LineNumberArea;

/// @brief A text editor for GLSL shaders
class CodeEditor : public QPlainTextEdit
{
  Q_OBJECT

public:
  CodeEditor( QWidget *parent = nullptr);

  void lineNumberAreaPaintEvent(QPaintEvent *event);
  int lineNumberAreaWidth();

  void gotoLineNumber(int lineNumber);

public slots:
   void onSearchBackwardRequest( const QString & text, QTextDocument::FindFlags flags);
   void onSearchForwardRequest( const QString & text, QTextDocument::FindFlags flags);
   void onReplaceTextRequest( const QString &src, const QString &dest,
                              QTextDocument::FindFlags flags, bool thenFind);

signals:
   // search operation failed
   void textNotFound();

protected:
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void onCursorPositionChanged();
  bool matchLeftParenthesis(char left, QTextBlock currentBlock, int i, int numLeftParentheses);
  bool matchRightParenthesis( char right, QTextBlock currentBlock, int i, int numRightParentheses);
  void updateLineNumberArea(const QRect &, int);

private:
  void highlightCurrentLine();
  void matchParenthesis();
  int countTrailingSpaces( int block_position);
  void createParenthesisSelection(int pos, Qt::GlobalColor color);
  bool findNext(const QString &text, QTextDocument::FindFlags flags);

private:
  QWidget * m_lineNumberArea;
  int indent_size_;
   QList<QTextEdit::ExtraSelection> extra_selections_;
};

/// @brief helper widget to print line numbers
class LineNumberArea : public QWidget
{
public:
  LineNumberArea(CodeEditor *editor) : QWidget(editor) {
    code_editor_ = editor;
  }

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  CodeEditor *code_editor_;
};


}  // namespace olive


#endif // CODEEDITOR_H
