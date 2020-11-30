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

#include "seekablewidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "common/qtutils.h"
#include "core.h"

namespace olive {

SeekableWidget::SeekableWidget(QWidget* parent) :
  TimelineScaledWidget(parent),
  time_(0),
  timeline_points_(nullptr),
  scroll_(0),
  snap_service_(nullptr)
{
  QFontMetrics fm = fontMetrics();

  text_height_ = fm.height();

  // Set width of playhead marker
  playhead_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  setContextMenuPolicy(Qt::CustomContextMenu);
}

void SeekableWidget::ConnectTimelinePoints(TimelinePoints *points)
{
  if (timeline_points_) {
    disconnect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerAdded, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerRemoved, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
  }

  timeline_points_ = points;

  if (timeline_points_) {
    connect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerAdded, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerRemoved, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
  }

  update();
}

void SeekableWidget::SetSnapService(SnapService *service)
{
  snap_service_ = service;
}

const int64_t &SeekableWidget::GetTime() const
{
  return time_;
}

const int &SeekableWidget::GetScroll() const
{
  return scroll_;
}

void SeekableWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
    SeekToScreenPoint(event->pos().x());
  }
}

void SeekableWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    SeekToScreenPoint(event->pos().x());
  }
}

void SeekableWidget::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)

  if (snap_service_) {
    snap_service_->HideSnaps();
  }
}

void SeekableWidget::ScaleChangedEvent(const double &)
{
  update();
}

TimelinePoints *SeekableWidget::timeline_points() const
{
  return timeline_points_;
}

void SeekableWidget::SetTime(const int64_t &r)
{
  time_ = r;

  update();
}

void SeekableWidget::SetScroll(int s)
{
  scroll_ = s;

  update();
}

double SeekableWidget::ScreenToUnitFloat(int screen) const
{
  return (screen + scroll_) / GetScale() / timebase_dbl();
}

int64_t SeekableWidget::ScreenToUnit(int screen) const
{
  return qFloor(ScreenToUnitFloat(screen));
}

int64_t SeekableWidget::ScreenToUnitRounded(int screen) const
{
  return qRound64(ScreenToUnitFloat(screen));
}

int SeekableWidget::UnitToScreen(int64_t unit) const
{
  return qFloor(static_cast<double>(unit) * GetScale() * timebase_dbl()) - scroll_;
}

int SeekableWidget::TimeToScreen(const rational &time) const
{
  return qFloor(time.toDouble() * GetScale()) - scroll_;
}

void SeekableWidget::SeekToScreenPoint(int screen)
{
  int64_t timestamp = qMax(static_cast<int64_t>(0), ScreenToUnitRounded(screen));

  if (Core::instance()->snapping() && snap_service_) {
    rational playhead_time = Timecode::timestamp_to_time(timestamp, timebase());
    rational movement;

    snap_service_->SnapPoint({playhead_time},
                             &movement,
                             SnapService::kSnapAll & ~SnapService::kSnapToPlayhead);

    if (!movement.isNull()) {
      timestamp = Timecode::time_to_timestamp(playhead_time + movement,
                                              timebase());
    }
  }

  SetTime(timestamp);

  emit TimeChanged(timestamp);
}

void SeekableWidget::DrawTimelinePoints(QPainter* p, int marker_bottom)
{
  if (!timeline_points()) {
    return;
  }

  // Draw in/out workarea
  if (timeline_points()->workarea()->enabled()) {
    int workarea_left = qMax(0, TimeToScreen(timeline_points()->workarea()->in()));
    int workarea_right;

    if (timeline_points()->workarea()->out() == TimelineWorkArea::kResetOut) {
      workarea_right = width();
    } else {
      workarea_right = qMin(width(), TimeToScreen(timeline_points()->workarea()->out()));
    }

    p->fillRect(workarea_left, 0, workarea_right - workarea_left, height(), palette().highlight());
  }

  // Draw markers
  if (marker_bottom > 0 && !timeline_points()->markers()->list().isEmpty()) {

    int marker_top = marker_bottom - text_height_;

    // FIXME: Hardcoded marker colors
    p->setPen(Qt::black);
    p->setBrush(Qt::green);

    foreach (TimelineMarker* marker, timeline_points()->markers()->list()) {
      int marker_left = TimeToScreen(marker->time().in());
      int marker_right = TimeToScreen(marker->time().out());

      if (marker_left >= width() || marker_right < 0)  {
        continue;
      }

      if (marker->time().length() == 0) {
        // Single point in time marker
        DrawPlayhead(p, marker_left, marker_bottom);
      } else {
        // Marker range
        int rect_left = qMax(0, marker_left);
        int rect_right = qMin(width(), marker_right);

        QRect marker_rect(rect_left, marker_top, rect_right - rect_left, marker_bottom - marker_top);

        p->drawRect(marker_rect);

        if (!marker->name().isEmpty()) {
          p->drawText(marker_rect, marker->name());
        }
      }
    }
  }
}

void SeekableWidget::DrawPlayhead(QPainter *p, int x, int y)
{
  int half_width = playhead_width_ / 2;

  if (x + half_width < 0 || x - half_width > width()) {
    return;
  }

  p->setRenderHint(QPainter::Antialiasing);

  int half_text_height = text_height() / 3;

  QPoint points[] = {
    QPoint(x, y),
    QPoint(x - half_width, y - half_text_height),
    QPoint(x - half_width, y - text_height()),
    QPoint(x + 1 + half_width, y - text_height()),
    QPoint(x + 1 + half_width, y - half_text_height),
    QPoint(x + 1, y),
  };

  p->drawPolygon(points, 6);

  p->setRenderHint(QPainter::Antialiasing, false);
}

}
