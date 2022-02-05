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

#include "editor.h"
#include "config/config.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QTextBlock>

#include "glslhighlighter.h"


namespace olive {

CodeEditor::CodeEditor(QWidget *parent) :
  QPlainTextEdit(parent)
{
  m_lineNumberArea = new LineNumberArea(this);

  connect( this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
  connect( this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
  connect( this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

  updateLineNumberAreaWidth(0);
  highlightCurrentLine();

  new GlslHighlighter( document());

  setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded);
  setLineWrapMode( QPlainTextEdit::NoWrap);

  setContextMenuPolicy( Qt::DefaultContextMenu);

  // These hard coded settings should be customizable in settings dialog
  setMinimumSize( 900, 600);

  QFont font("Monospace");
  font.setStyleHint(QFont::TypeWriter);  // use monospaced font for all platform
  setFont( font);
  QFontMetrics metrics(font);

  bool valid;
  int fontSize = Config::Current()["EditorInternalFontSize"].toInt( & valid);
  fontSize = valid ? fontSize : 14;
  font.setPointSize( fontSize);

  indent_size_ = Config::Current()["EditorInternalIndentSize"].toInt( & valid);
  indent_size_ = (valid && (indent_size_ > 0)) ? indent_size_ : 3;
  setTabStopDistance( (qreal)(indent_size_) * metrics.horizontalAdvance(' '));

  setStyleSheet("QPlainTextEdit { background-color: black }");
}

int CodeEditor::lineNumberAreaWidth()
{
  int digits = 1;
  int max = qMax(1, blockCount());
  while (max >= 10)
  {
    max /= 10;
    ++digits;
  }

  int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

  return space;
}

void CodeEditor::gotoLineNumber(int lineNumber)
{
  /* block number start from 0 where 'lineNumber' start from 1 */
  QTextCursor cursor(document()->findBlockByLineNumber(lineNumber - 1));
  setTextCursor(cursor);
  highlightCurrentLine();
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
  setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}


void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
  if (dy)
  {
    m_lineNumberArea->scroll(0, dy);
  }
  else
  {
    m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
  }

  if (rect.contains(viewport()->rect()))
  {
    updateLineNumberAreaWidth(0);
  }
}


void CodeEditor::resizeEvent(QResizeEvent *e)
{
  QPlainTextEdit::resizeEvent(e);

  QRect cr = contentsRect();
  m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


void CodeEditor::highlightCurrentLine()
{
  QList<QTextEdit::ExtraSelection> extraSelections;

  QTextEdit::ExtraSelection selection;

  QColor lineColor = QColor(40,40,0);

  selection.format.setBackground( lineColor);
  selection.format.setProperty( QTextFormat::FullWidthSelection, true);
  selection.cursor = textCursor();
  selection.cursor.clearSelection();
  extraSelections.append(selection);

  setExtraSelections(extraSelections);
}


void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
  QPainter painter(m_lineNumberArea);
  painter.fillRect(event->rect(), Qt::lightGray);


  QTextBlock block = firstVisibleBlock();
  int blockNumber = block.blockNumber();
  int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
  int bottom = top + (int) blockBoundingRect(block).height();

  while (block.isValid() && top <= event->rect().bottom())
  {
    if (block.isVisible() && bottom >= event->rect().top())
    {
      QString number = QString::number(blockNumber + 1);
      painter.setPen(Qt::black);
      painter.drawText(0, top, m_lineNumberArea->width(), fontMetrics().height(),
                       Qt::AlignRight, number);
    }

    block = block.next();
    top = bottom;
    bottom = top + (int) blockBoundingRect(block).height();
    ++blockNumber;
  }
}

QSize LineNumberArea::sizeHint() const
{
  return QSize(code_editor_->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
  code_editor_->lineNumberAreaPaintEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
  // replace tab with spaces
  if (event->key() == Qt::Key_Tab) {
    int currentColumn = textCursor().positionInBlock();

    // always add at least one space
    do {
      insertPlainText(" ");
      currentColumn++;
    } while( (currentColumn % indent_size_) != 0);
  }
  else if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter)) {
    // we have not changed line yet. Count blanks of current line
    int n_space = countTrailingSpaces( textCursor().block().position());

    // append new line
    QPlainTextEdit::keyPressEvent( event);

    // indent the new line with the same white spaces as previous line
    insertPlainText( QString(' ').repeated(n_space));
  }
  else {
    QPlainTextEdit::keyPressEvent( event);
  }
}

// count the number of whitespaces at the begin of current line.
// In case of TAB, a number between 1 and 'indent_size_' is added.
// The returned value is always in spaces, not in tabs.
int CodeEditor::countTrailingSpaces(int block_position)
{
  int n_spaces = 0;
  QString text = toPlainText();
  bool done = false;

  Q_ASSERT( indent_size_ > 0);

  while( done == false) {
    switch (text.at(block_position).toLatin1()) {
    case ' ':
      n_spaces++;
      break;
    case '\t':
      n_spaces =((n_spaces / indent_size_) * indent_size_) + indent_size_;
      break;
    default:
      done = true;
    }

    block_position++;
  }

  return n_spaces;
}



}  // namespace olive

