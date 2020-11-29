/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "resizabletimelinescrollbar.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QtMath>

namespace olive {

ResizableTimelineScrollBar::ResizableTimelineScrollBar(QWidget* parent) :
  ResizableScrollBar(parent),
  points_(nullptr),
  scale_(1.0)
{
}

ResizableTimelineScrollBar::ResizableTimelineScrollBar(Qt::Orientation orientation, QWidget* parent) :
  ResizableScrollBar(orientation, parent),
  points_(nullptr),
  scale_(1.0)
{
}

void ResizableTimelineScrollBar::ConnectTimelinePoints(TimelinePoints *points)
{
  if (points_) {
    disconnect(points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    disconnect(points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    disconnect(points_->markers(), &TimelineMarkerList::MarkerAdded, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    disconnect(points_->markers(), &TimelineMarkerList::MarkerRemoved, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
  }

  points_ = points;

  if (points_) {
    connect(points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    connect(points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    connect(points_->markers(), &TimelineMarkerList::MarkerAdded, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    connect(points_->markers(), &TimelineMarkerList::MarkerRemoved, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
  }

  update();
}

void ResizableTimelineScrollBar::SetScale(double d)
{
  scale_ = d;

  update();
}

void ResizableTimelineScrollBar::paintEvent(QPaintEvent *event)
{
  ResizableScrollBar::paintEvent(event);

  if (points_
      && !timebase().isNull()
      && (points_->workarea()->enabled() || !points_->markers()->list().isEmpty())) {
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect gr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                       QStyle::SC_ScrollBarGroove, this);

    double ratio = scale_ * double(gr.width()) / double(this->maximum() + gr.width());

    qDebug() << "Calculated ratio as:" << ratio << "from groove" << gr.width() << "vs max:" << this->maximum();

    QPainter p(this);

    if (points_->workarea()->enabled()) {
      QColor workarea_color(this->palette().highlight().color());
      workarea_color.setAlpha(128);

      int in = qRound(ratio * TimeToScene(points_->workarea()->in()));
      int out = qRound(ratio * TimeToScene(points_->workarea()->out()));
      int length = qMax(1, out-in);

      p.fillRect(gr.x() + in,
                 0,
                 length,
                 height(),
                 workarea_color);
    }

    if (!points_->markers()->list().isEmpty()) {
      QColor marker_color(0, 255, 0, 128);
      foreach (TimelineMarker* marker, points_->markers()->list()) {
        int in = qRound(ratio * TimeToScene(marker->time().in()));
        int out = qRound(ratio * TimeToScene(marker->time().out()));
        int length = qMax(1, out-in);

        p.fillRect(gr.x() + in,
                   0,
                   length,
                   height(),
                   marker_color);
      }
    }
  }
}

}
