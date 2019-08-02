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

#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QGraphicsView>

#include "node/block/clip/clip.h"
#include "timelineviewclipitem.h"
#include "timelineviewghostitem.h"

class TimelineView : public QGraphicsView
{
  Q_OBJECT
public:
  TimelineView(QWidget* parent);

  void AddClip(ClipBlock* clip);

  void SetScale(const double& scale);

  void SetTimebase(const rational& timebase);

  void Clear();

public slots:
  void SetTime(const int64_t time);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

private:
  void UpdatePlayheadPosition();

  QGraphicsScene scene_;

  double scale_;

  rational timebase_;

  int64_t playhead_;

  QVector<TimelineViewClipItem*> clip_items_;

  QVector<TimelineViewGhostItem*> ghost_items_;

  QGraphicsLineItem* playhead_line_;
};

#endif // TIMELINEVIEW_H
