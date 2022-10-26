/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <QPainter>

#include "textanimationrender.h"
#include "node/generator/animation/textanimationxmlparser.h"

#include <QDebug>
namespace olive {

TextAnimationRender::TextAnimationRender()
{
}

void TextAnimationRender::render(const QString & animators_tags,
                                  QTextDocument & text_doc,
                                  QPainter & p)
{
  QTextCursor cursor( &text_doc);
  int block_count = text_doc.blockCount();
  // character index in full document
  int index = 0;
  cursor.movePosition( QTextCursor::Start);

  qreal line_Voffset = text_doc.documentLayout()->blockBoundingRect(cursor.block()).bottom() -
      maxDescent(cursor);

  // calculate animation data
  QList<TextAnimation::Descriptor> animators = TextAnimationXmlParser::Parse( "<R>" + animators_tags + "</R>");
  engine_.SetTextSize( text_doc.characterCount());
  engine_.SetAnimators( & animators);
  engine_.Calulate();

  const QVector<double> & horiz_offsets = engine_.HorizontalOffset();
  const QVector<double> & horiz_stretches = engine_.HorizontalStretch();
  const QVector<double> & rotations = engine_.Rotation();
  const QVector<double> & spacings = engine_.Spacing();
  const QVector<double> & vert_offsets = engine_.VerticalOffset();
  const QVector<double> & vert_stretches = engine_.VerticalStretch();
  const QVector<int> & transparencies = engine_.Transparency();

  qDebug() << vert_offsets;

  // parse the whole text document and print every character
  for (int blk=0; blk < block_count; blk++) {
    qreal posx = 0.;

    // x position of characters of current block without any animation
    QVector<qreal> base_position_x;
    qreal total_width = 0.;

    parseBlock( cursor, base_position_x, total_width);
    posx = initialPositionX( text_doc, cursor, total_width);

    /* keep the cursor on the right of the character that we're going to
     * print because the "charFormat()" refers to previous character */
    cursor.movePosition( QTextCursor::Right);

    for( qreal & x: base_position_x) {
      p.save();
      p.setFont( cursor.charFormat().font());

      QColor ch_color = Qt::white;

      if (cursor.charFormat().foreground().style() != Qt::NoBrush) {
        ch_color = cursor.charFormat().foreground().color();
      }

      // "transparencies" can be assumed to be in range 0 - 255
      ch_color.setAlpha(255 - transparencies[index]);

      p.setPen( ch_color);
      p.translate( QPointF((posx + x)*(1. + horiz_offsets[index])*(1. + spacings[index]),
                           line_Voffset + vert_offsets[index]));
      p.rotate( rotations[index]);
      p.scale( 1.+ horiz_stretches[index], 1. + vert_stretches[index]);
      p.drawText( QPointF(0,0), text_doc.characterAt(cursor.position() - 1));
      p.restore();

      cursor.movePosition( QTextCursor::Right);
      index++;
    }

    line_Voffset = text_doc.documentLayout()->blockBoundingRect(cursor.block()).bottom() -
                   maxDescent(cursor);
  }

  qDebug() << "index:" << index;
}

qreal TextAnimationRender::maxDescent( QTextCursor cursor) const
{
  int descent = QFontMetrics(cursor.charFormat().font()).descent();
  cursor.movePosition( QTextCursor::Right);

  while (cursor.atBlockEnd() ==  false) {
    int new_descent = QFontMetrics(cursor.charFormat().font()).descent();

    if (new_descent > descent) {
      descent = new_descent;
    }

    cursor.movePosition( QTextCursor::Right);
  }

  return descent;
}

qreal TextAnimationRender::initialPositionX( QTextDocument & doc,
                                             QTextCursor & cur,
                                             qreal total_width) const
{
  qreal posx = 0;
  Qt::Alignment align = cur.blockFormat().alignment();
  QRectF boundingRect = doc.documentLayout()->blockBoundingRect(cur.block());

  if ((align == Qt::AlignLeft) || (align == Qt::AlignJustify)){
    posx = boundingRect.left();
  } else if (cur.blockFormat().alignment() == Qt::AlignHCenter) {
    posx = (boundingRect.right() / 2.) - (total_width/2.);
  } else {  // align right
    posx = boundingRect.right() - total_width;
  }

  return posx;
}

// parse the current cursor until the end of block and calculate the x position
// of each letter in case of no animation.
// While we are parsing, it is handy to also calculate the line descent and total text
// width, that will be useful in case of non-left aligned text
void TextAnimationRender::parseBlock(const QTextCursor &cursor,
                                     QVector<qreal> & offset_x,
                                     qreal & total_width) const
{
  double char_width;

  offset_x.clear();
  total_width = 0.;

  QTextCursor blockCursor( cursor.block());
  /* the first char is always at offset 0. Keep the cursor on the
   * right of the character that we're analysing
   * because the "charFormat()" refers to previous character */
  offset_x << 0.;
  blockCursor.movePosition( QTextCursor::Right);

  while (blockCursor.atBlockEnd() ==  false) {
    char_width = currentCharWidth( blockCursor);
    total_width += char_width;

    offset_x << offset_x.last() + char_width;
    blockCursor.movePosition( QTextCursor::Right);
  }

  /* the last character only counts for the total width, not for the offset list */
  char_width = currentCharWidth( blockCursor);
  total_width += char_width;
}

double TextAnimationRender::currentCharWidth( QTextCursor & cursor) const
{
  QChar ch = cursor.document()->characterAt( cursor.position() - 1);

  qreal char_size = QFontMetrics( cursor.charFormat().font()).horizontalAdvance(ch);
  qreal spacing = cursor.charFormat().font().letterSpacing();
  double stretch_factor = (spacing > 0.) ?
                            spacing/100. :
                            1.;

  return  (double)char_size*(stretch_factor);
}

}  // olive
