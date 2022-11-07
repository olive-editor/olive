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

#ifndef TEXTANIMATIONRENDER_H
#define TEXTANIMATIONRENDER_H

#include <QVector>

#include "node/generator/animation/textanimationengine.h"

class QTextDocument;
class QPainter;
class QTextCursor;


namespace olive {

class TextAnimationRender
{
public:
  TextAnimationRender();

  /// render an animated text.
  /// \param animators is an XML list of animators
  void render(const QString &animators_tags,
              QTextDocument &text_doc,
              QPainter &p);

private:

  qreal maxDescent( QTextCursor cursor) const;

  qreal initialPositionX( QTextDocument & doc,
                          QTextCursor & cur,
                          qreal total_width) const;

  void parseBlock( const QTextCursor & cursor,
                   QVector<qreal> & offset_x,
                   qreal &total_width) const;

  double currentCharWidth( QTextCursor & cursor) const;

  qreal calculateSpacing(int block_start_index, int index, const QVector<double>& spacings);

private:
  TextAnimationEngine engine_;

};

}

#endif // TEXTANIMATIONRENDER_H
