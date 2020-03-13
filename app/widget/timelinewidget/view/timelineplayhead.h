/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef TIMELINEPLAYHEADSTYLE_H
#define TIMELINEPLAYHEADSTYLE_H

#include <QWidget>

/**
 * @brief A QWidget proxy for TimeRuler and TimelinePlayheadItem to allow them to share CSS values
 *
 * To allow Qt CSS customization (which is only available to QWidgets) to be accessed by TimeRuler
 */
class TimelinePlayhead : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QColor playheadColor READ GetPlayheadColor WRITE SetPlayheadColor DESIGNABLE true)
  Q_PROPERTY(QColor playheadHighlightColor READ GetPlayheadHighlightColor WRITE SetPlayheadHighlightColor DESIGNABLE true)
public:
  TimelinePlayhead() = default;

  const QColor& GetPlayheadColor() const;
  const QColor& GetPlayheadHighlightColor() const;

  void SetPlayheadColor(QColor c);
  void SetPlayheadHighlightColor(QColor c);

  void Draw(QPainter *painter, const QRectF &rect) const;

private:
  QColor playhead_color_;
  QColor playhead_highlight_color_;
};

#endif // TIMELINEPLAYHEADSTYLE_H
