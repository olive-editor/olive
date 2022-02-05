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

  void setEditMode( bool isEditMode);

  void gotoLineNumber(int lineNumber);

signals:
  void lineDoubleClicked( int blockNumber);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void highlightCurrentLine();
  void updateLineNumberArea(const QRect &, int);

private:
  int countTrailingSpaces( int block_position);

private:
  QWidget * m_lineNumberArea;
  int indent_size_;
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
