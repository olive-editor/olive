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

#include "resizabletimelinescrollbar.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QtMath>

#include "ui/colorcoding.h"

namespace olive {

ResizableTimelineScrollBar::ResizableTimelineScrollBar(QWidget* parent) :
  ResizableScrollBar(parent),
  markers_(nullptr),
  workarea_(nullptr),
  scale_(1.0)
{
}

ResizableTimelineScrollBar::ResizableTimelineScrollBar(Qt::Orientation orientation, QWidget* parent) :
  ResizableScrollBar(orientation, parent),
  markers_(nullptr),
  workarea_(nullptr),
  scale_(1.0)
{
}

void ResizableTimelineScrollBar::ConnectMarkers(TimelineMarkerList *markers)
{
  if (markers_) {
    disconnect(markers_, &TimelineMarkerList::MarkerAdded, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    disconnect(markers_, &TimelineMarkerList::MarkerRemoved, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    disconnect(markers_, &TimelineMarkerList::MarkerModified, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
  }

  markers_ = markers;

  if (markers_) {
    connect(markers_, &TimelineMarkerList::MarkerAdded, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    connect(markers_, &TimelineMarkerList::MarkerRemoved, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    connect(markers_, &TimelineMarkerList::MarkerModified, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
  }

  update();
}

void ResizableTimelineScrollBar::ConnectWorkArea(TimelineWorkArea *workarea)
{
  if (workarea_) {
    disconnect(workarea_, &TimelineWorkArea::RangeChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    disconnect(workarea_, &TimelineWorkArea::EnabledChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
  }

  workarea_ = workarea;

  if (workarea_) {
    connect(workarea_, &TimelineWorkArea::RangeChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
    connect(workarea_, &TimelineWorkArea::EnabledChanged, this, static_cast<void (ResizableTimelineScrollBar::*)()>(&ResizableTimelineScrollBar::update));
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

  if (!timebase().isNull() && ((workarea_ && workarea_->enabled()) || (markers_ && !markers_->empty()))) {
    // Draw workarea
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect gr = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                       QStyle::SC_ScrollBarGroove, this);

    double ratio = scale_ * double(gr.width()) / double(this->maximum() + gr.width());
    QPainter p(this);

    if (workarea_ && workarea_->enabled()) {

      QColor workarea_color(this->palette().highlight().color());
      workarea_color.setAlpha(128);

      qint64 in = qMax(qint64(0), qRound64(ratio * TimeToScene(workarea_->in())));

      qint64 out;
      if (workarea_->out() == RATIONAL_MAX) {
        out = gr.width();
      } else {
        out = qMin(qint64(gr.width()), qRound64(ratio * TimeToScene(workarea_->out())));
      }

      qint64 length = qMax(qint64(1), out-in);

      p.fillRect(gr.x() + in,
                 0,
                 length,
                 height(),
                 workarea_color);
    }

    // Draw markers
    if (markers_ && !markers_->empty()) {
      for (auto it=markers_->cbegin(); it!=markers_->cend(); it++) {
        TimelineMarker* marker = *it;

        QColor marker_color = QtUtils::toQColor(ColorCoding::GetColor(marker->color()));
        int64_t in = qRound64(ratio * TimeToScene(marker->time().in()));
        int64_t out = qRound64(ratio * TimeToScene(marker->time().out()));
        int64_t length = qMax(int64_t(1), out-in);

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
