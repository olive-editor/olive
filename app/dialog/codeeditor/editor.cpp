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
#include <QTextDocumentFragment>

#include "glslhighlighter.h"

namespace {
// matching bracket for each kind of parenthesis
const QMap<char, char> BRACKET_PAIR = QMap<char, char>{{'(', ')'}, {'[', ']'}, {'{', '}'},
                                                       {')', '('}, {']', '['}, {'}', '{'}};

const Qt::GlobalColor BRACKET_MATCH_COLOR = Qt::darkGreen;
const Qt::GlobalColor BRACKET_NOT_MATCH_COLOR = Qt::darkRed;

// used in parentheses match algorithm.
const int BRACKET_LAST = -2;

// forward declarations
QString regExpEscape( const QStringView &str);
QString buildSearchExpression( const QStringView &str, QTextDocument::FindFlags & flags);
}

namespace olive {

CodeEditor::CodeEditor(QWidget *parent) :
  QPlainTextEdit(parent)
{
  m_lineNumberArea = new LineNumberArea(this);

  connect( this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
  connect( this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
  connect( this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));

  updateLineNumberAreaWidth(0);
  highlightCurrentLine();

  new GlslHighlighter( document());

  setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded);
  setLineWrapMode( QPlainTextEdit::NoWrap);

  setContextMenuPolicy( Qt::DefaultContextMenu);

  setMinimumSize( 900, 600);

  QFont font("Monospace");
  font.setStyleHint(QFont::TypeWriter);  // use monospaced font for all platform
  QFontMetrics metrics(font);
  bool valid;
  int fontSize = Config::Current()["EditorInternalFontSize"].toInt( & valid);
  fontSize = valid ? fontSize : 14;
  font.setPointSize( fontSize);
  int win_width = Config::Current()["EditorInternalWindowWidth"].toInt( & valid);
  win_width = valid ? win_width : 800;
  int win_height = Config::Current()["EditorInternalWindowHeight"].toInt( & valid);
  win_height = valid ? win_height : 600;

  setMinimumSize( win_width, win_height);

  setFont( font);

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
  onCursorPositionChanged();
}

void CodeEditor::onSearchBackwardRequest(const QString &text, QTextDocument::FindFlags flags)
{
  // try to find backword
  bool found = findNext( text, flags | QTextDocument::FindBackward);

  if ( ! found)
  {
    QTextCursor cursor = textCursor();
    int currentPosition = cursor.position();

    // search again from the end
    cursor.movePosition( QTextCursor::End);
    setTextCursor( cursor);

    found = findNext( text, flags | QTextDocument::FindBackward);

    if ( ! found)
    {
      // 'text' is not present. Go back where we were
      cursor.setPosition( currentPosition );
      setTextCursor( cursor);

      emit textNotFound();
    }
  }
}

void CodeEditor::onSearchForwardRequest(const QString &text, QTextDocument::FindFlags flags)
{
  // try to find forward
  bool found = findNext( text, flags);

  if ( ! found)
  {
     QTextCursor cursor = textCursor();
     int currentPosition = cursor.position();

     // search again from the begin
     cursor.movePosition( QTextCursor::Start);
     setTextCursor( cursor);

     found = findNext( text, flags);

     if ( ! found)
     {
        // 'text' is not present. Go back where we were
        cursor.setPosition( currentPosition );
        setTextCursor( cursor);

        emit textNotFound();
     }
  }
}

bool CodeEditor::findNext(const QString &text, QTextDocument::FindFlags flags)
{
  QString regExpText = buildSearchExpression( text, flags);
  return find( QRegularExpression(regExpText), flags);
}

void CodeEditor::onReplaceTextRequest( const QString & src, const QString & dest,
                                       QTextDocument::FindFlags flags, bool thenFind)
{
  // replace only if selection holds 'src' text
  if ( src == textCursor().selection().toPlainText()) {
    textCursor().insertText( dest);
  }

  if (thenFind == true) {
    onSearchForwardRequest( src, flags);
  }
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


void CodeEditor::onCursorPositionChanged()
{
  extra_selections_.clear();

  highlightCurrentLine();
  matchParenthesis();

  setExtraSelections(extra_selections_);
}


void CodeEditor::highlightCurrentLine()
{
  QTextEdit::ExtraSelection selection;

  QColor lineColor = QColor(40,40,0);

  selection.format.setBackground( lineColor);
  selection.format.setProperty( QTextFormat::FullWidthSelection, true);
  selection.cursor = textCursor();
  selection.cursor.clearSelection();
  extra_selections_.append(selection);
}


void CodeEditor::matchParenthesis()
{
  TextBlockData * parenthesis_data = static_cast<TextBlockData *>(textCursor().block().userData());

  if (parenthesis_data) {
    const QVector<ParenthesisInfo> & infos = parenthesis_data->parentheses();

    int pos = textCursor().block().position();
    for (int i = 0; i < infos.size(); ++i) {
      const ParenthesisInfo & info = infos.at(i);

      int cur_pos = textCursor().position() - textCursor().block().position();

      if ((info.position == (cur_pos)) && (QString("([{").contains(info.character))) {
        if (matchLeftParenthesis( info.character, textCursor().block(), i + 1, 0)) {
          createParenthesisSelection(pos + info.position, BRACKET_MATCH_COLOR);
        } else {
          createParenthesisSelection(pos + info.position, BRACKET_NOT_MATCH_COLOR);
        }
      } else if ((info.position == (cur_pos - 1)) && (QString(")]}").contains(info.character))) {
        if (matchRightParenthesis( info.character, textCursor().block(), i - 1, 0)) {
          createParenthesisSelection(pos + info.position, BRACKET_MATCH_COLOR);
        } else {
          createParenthesisSelection(pos + info.position, BRACKET_NOT_MATCH_COLOR);
        }
      }
    }
  }
}


// search for a right (closing) bracket that matches a given left (opening) bracket.
// The search is performed in the 'currentBlock' or recursively in next one.
// 'numLeftParentheses' is the counter of additional left brackets found in previous calls.
// Param 'i' (index) is used when the search does not start from the begin of the block: in this case,
// we don't have to search brackets after the current cursor position.
//
bool CodeEditor::matchLeftParenthesis( char left, QTextBlock currentBlock, int i, int numLeftParentheses)
{
  TextBlockData * parenthesis_data = static_cast<TextBlockData *>(currentBlock.userData());
  const QVector<ParenthesisInfo> & infos = parenthesis_data->parentheses();

  bool found_matching = false;
  int doc_pos = currentBlock.position();

  for (; (i < infos.size())  && ( ! found_matching); ++i) {
    const ParenthesisInfo & info = infos.at(i);

    if (info.character == left) {
      // found another opening bracket
      ++numLeftParentheses;

    } else if (info.character == BRACKET_PAIR.value(left)) {

      if (numLeftParentheses == 0) {
        //found matching closing bracket
        createParenthesisSelection(doc_pos + info.position, BRACKET_MATCH_COLOR);
        found_matching = true;
      } else {
        //found closing bracket, but not the correct one
        --numLeftParentheses;
      }
    }
  }

  if (found_matching == false) {
    currentBlock = currentBlock.next();
    if (currentBlock.isValid()) {
      found_matching = matchLeftParenthesis( left, currentBlock, 0, numLeftParentheses);
    }
  }

  return found_matching;
}

// search for a left (opening) bracket that matches a given right (closing) bracket.
// The search is performed in the 'currentBlock' or recursively in previous one.
// 'numRightParentheses' is the counter of additional right brackets found in previous calls.
// Param 'i' (index) is used when the search does not start from the end of the block: in this case,
// we don't have to search brackets before the current cursor position. Value "BRACKET_LAST" means
// to start from last item
//
bool CodeEditor::matchRightParenthesis(char right, QTextBlock currentBlock, int i, int numRightParentheses)
{
  TextBlockData * parenthesis_data = static_cast<TextBlockData *>(currentBlock.userData());
  const QVector<ParenthesisInfo> & parentheses = parenthesis_data->parentheses();

  bool found_matching = false;
  int doc_pos = currentBlock.position();

  if ((i == BRACKET_LAST) && (parentheses.size() > 0)){
    // start from last item
    i = parentheses.size() - 1;
  }

  for (; (i > -1) && (parentheses.size() > 0) && ( ! found_matching); --i) {
    const ParenthesisInfo & info = parentheses.at(i);

    if (info.character == right) {
      // found another closing bracket
      ++numRightParentheses;

    } else if (info.character == BRACKET_PAIR.value(right)) {

      if (numRightParentheses == 0) {
        //found matching opening bracket
        createParenthesisSelection(doc_pos + info.position, BRACKET_MATCH_COLOR);
        found_matching = true;
      } else {
        // found opening bracket, but not the correct one
        --numRightParentheses;
      }
    }
  }

  if (found_matching == false) {
    currentBlock = currentBlock.previous();
    if (currentBlock.isValid()) {
      found_matching = matchRightParenthesis( right, currentBlock, BRACKET_LAST, numRightParentheses);
    }
  }

  return found_matching;
}


void CodeEditor::createParenthesisSelection(int pos, Qt::GlobalColor color)
{
  QTextEdit::ExtraSelection selection;
  QTextCharFormat format = selection.format;
  format.setBackground(color);
  selection.format = format;

  QTextCursor cursor = textCursor();
  cursor.setPosition(pos);
  cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
  selection.cursor = cursor;

  extra_selections_.append(selection);
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

    // indent if previous line ends with opening bracket
    if (textCursor().block().text().trimmed().endsWith('{')) {
      n_space += indent_size_;
    }

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
  int text_size = text.length();
  bool done = false;

  Q_ASSERT( indent_size_ > 0);

  while( done == false) {

    // read one char until text finishes or a non blank is found
    QChar c = (block_position < text_size) ? text.at(block_position) : '*';

    switch (c.toLatin1()) {
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

namespace {

// This is taken from QT 5.15 source code for "QRegularExpression::escape".
// Olive must compile with QT 5.9 on.
QString regExpEscape(const QStringView &str)
{
  QString result;
  const int count = str.size();
  result.reserve(count * 2);
  // everything but [a-zA-Z0-9_] gets escaped,
  // cf. perldoc -f quotemeta
  for (int i = 0; i < count; ++i) {
    const QChar current = str.at(i);
    if (current == QChar::Null) {
      // unlike Perl, a literal NUL must be escaped with
      // "\\0" (backslash + 0) and not "\\\0" (backslash + NUL),
      // because pcre16_compile uses a NUL-terminated string
      result.append(QLatin1Char('\\'));
      result.append(QLatin1Char('0'));
    } else if ( (current < QLatin1Char('a') || current > QLatin1Char('z')) &&
                (current < QLatin1Char('A') || current > QLatin1Char('Z')) &&
                (current < QLatin1Char('0') || current > QLatin1Char('9')) &&
                current != QLatin1Char('_') )
    {
      result.append(QLatin1Char('\\'));
      result.append(current);
      if (current.isHighSurrogate() && i < (count - 1))
        result.append(str.at(++i));
    } else {
      result.append(current);
    }
  }
  result.squeeze();
  return result;
}

// This function is required to search for "whole word". The default QT function considers
// underscore "_" as a word breaker, so if you searh for "hello" as whole word it will match
// "hello_world".
// For this reason, we always search for a regular expression and remove "FindWholeWords"
// from 'flags', if present.
QString buildSearchExpression( const QStringView &str, QTextDocument::FindFlags & flags)
{
  QString regExpString = regExpEscape( str);

  if (flags & QTextDocument::FindWholeWords) {
    regExpString.prepend("\\b");
    regExpString.append("\\b");

    flags &= (~QTextDocument::FindWholeWords);
  }

  return regExpString;
}

}

