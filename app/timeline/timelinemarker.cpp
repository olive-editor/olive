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

#include "timelinemarker.h"

#include "common/qtutils.h"
#include "common/xmlutils.h"
#include "config/config.h"
#include "core.h"
#include "ui/colorcoding.h"

namespace olive {

TimelineMarker::TimelineMarker(QObject *parent) :
  color_(OLIVE_CONFIG("MarkerColor").toInt())
{
  setParent(parent);
}

TimelineMarker::TimelineMarker(int color, const TimeRange &time, const QString &name, QObject *parent) :
  time_(time),
  name_(name),
  color_(color)
{
  setParent(parent);
}

void TimelineMarker::set_time(const TimeRange &time)
{
  time_ = time;
  emit TimeChanged(time_);
}

void TimelineMarker::set_time(const rational &time)
{
  set_time(TimeRange(time, time + time_.length()));
}

bool TimelineMarker::has_sibling_at_time(const rational &t) const
{
  TimelineMarker *m = static_cast<TimelineMarkerList*>(parent())->GetMarkerAtTime(t);
  return m && m != this;
}

void TimelineMarker::set_name(const QString &name)
{
  name_ = name;
  emit NameChanged(name_);
}

void TimelineMarker::set_color(int c)
{
  color_ = c;
  emit ColorChanged(color_);
}

int TimelineMarker::GetMarkerHeight(const QFontMetrics &fm)
{
  return fm.height();
}

QRect TimelineMarker::Draw(QPainter *p, const QPoint &pt, double scale, bool selected)
{
  QFontMetrics fm = p->fontMetrics();

  int marker_height = GetMarkerHeight(fm);
  int marker_width = QtUtils::QFontMetricsWidth(fm, QStringLiteral("H"));

  int half_width = marker_width / 2;

  QColor c = ColorCoding::GetColor(color()).toQColor();
  if (selected) {
    p->setPen(Qt::white);
    p->setBrush(c.lighter());
  } else {
    p->setPen(Qt::black);
    p->setBrush(c);
  }

  int top = pt.y() - marker_height;

  if (time_.out() != time_.in()) {
    QRect marker_rect(pt.x(), top, time_.length().toDouble() * scale, marker_height);

    p->drawRect(marker_rect);

    if (!name_.isEmpty()) {
      p->setPen(ColorCoding::GetUISelectorColor(ColorCoding::GetColor(color_)));
      p->drawText(marker_rect.adjusted(marker_width/4, 0, 0, 0), name_, Qt::AlignLeft | Qt::AlignVCenter);
    }

    return marker_rect;
  } else {
    int half_marker_height = marker_height / 3;
    int left = pt.x() - half_width;
    int right = pt.x() + half_width;
    int center_y = pt.y() - half_marker_height;

    QPoint points[] = {
        pt,
        QPoint(left, center_y),
        QPoint(left, top),
        QPoint(right, top),
        QPoint(right, center_y),
        pt,
    };

    p->setRenderHint(QPainter::Antialiasing);
    p->drawPolygon(points, 6);

    return QRect(left, top, marker_width, marker_height);
  }
}

void TimelineMarkerList::childEvent(QChildEvent *e)
{
  QObject::childEvent(e);

  if (TimelineMarker *marker = dynamic_cast<TimelineMarker *>(e->child())) {
    if (e->type() == QChildEvent::ChildAdded) {

      connect(marker, &TimelineMarker::TimeChanged, this, &TimelineMarkerList::HandleMarkerTimeChange);
      connect(marker, &TimelineMarker::TimeChanged, this, &TimelineMarkerList::HandleMarkerModification);
      connect(marker, &TimelineMarker::NameChanged, this, &TimelineMarkerList::HandleMarkerModification);
      connect(marker, &TimelineMarker::ColorChanged, this, &TimelineMarkerList::HandleMarkerModification);

      InsertIntoList(marker);

      emit MarkerAdded(marker);

    } else if (e->type() == QChildEvent::ChildRemoved) {

      RemoveFromList(marker);

      disconnect(marker, &TimelineMarker::TimeChanged, this, &TimelineMarkerList::HandleMarkerTimeChange);
      disconnect(marker, &TimelineMarker::TimeChanged, this, &TimelineMarkerList::HandleMarkerModification);
      disconnect(marker, &TimelineMarker::NameChanged, this, &TimelineMarkerList::HandleMarkerModification);
      disconnect(marker, &TimelineMarker::ColorChanged, this, &TimelineMarkerList::HandleMarkerModification);

      emit MarkerRemoved(marker);

    }
  }
}

void TimelineMarkerList::InsertIntoList(TimelineMarker *marker)
{
  // Insertion sort by time to allow some loop optimizations
  bool found = false;
  for (auto it=markers_.begin(); it!=markers_.end(); it++) {
    TimelineMarker *m = *it;

    Q_ASSERT(m->time() != marker->time());

    if (m->time() > marker->time()) {
      markers_.insert(it, marker);
      found = true;
      break;
    }
  }

  if (!found) {
    markers_.push_back(marker);
  }
}

bool TimelineMarkerList::RemoveFromList(TimelineMarker *marker)
{
  auto it = std::find(markers_.begin(), markers_.end(), marker);

  if (it != markers_.end()) {
    markers_.erase(it);
    return true;
  }

  return false;
}

void TimelineMarkerList::HandleMarkerModification()
{
  emit MarkerModified(static_cast<TimelineMarker*>(sender()));
}

void TimelineMarkerList::HandleMarkerTimeChange()
{
  TimelineMarker *m = static_cast<TimelineMarker*>(sender());

  auto it = std::find(markers_.begin(), markers_.end(), m);

  if ((it+1 != markers_.end() && (*(it+1))->time() < m->time())
      || (it != markers_.begin() && (*(it-1))->time() > m->time())) {
    // Re-sort into list
    markers_.erase(it);
    InsertIntoList(m);
  }
}

MarkerAddCommand::MarkerAddCommand(TimelineMarkerList *marker_list, const TimeRange &range, const QString &name, int color) :
  MarkerAddCommand(marker_list, new TimelineMarker(color, range, name, &memory_manager_))
{
}

MarkerAddCommand::MarkerAddCommand(TimelineMarkerList *marker_list, TimelineMarker *marker) :
  marker_list_(marker_list),
  added_marker_(marker)
{
  added_marker_->setParent(&memory_manager_);
}

Project* MarkerAddCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_list_);
}

void MarkerAddCommand::redo()
{
  added_marker_->setParent(marker_list_);
}

void MarkerAddCommand::undo()
{
  added_marker_->setParent(&memory_manager_);
}

MarkerRemoveCommand::MarkerRemoveCommand(TimelineMarker *marker) :
  marker_(marker)
{
}

Project* MarkerRemoveCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerRemoveCommand::redo()
{
  marker_list_ = marker_->parent();
  marker_->setParent(&memory_manager_);
}

void MarkerRemoveCommand::undo()
{
  marker_->setParent(marker_list_);
}

MarkerChangeColorCommand::MarkerChangeColorCommand(TimelineMarker *marker, int new_color) :
  marker_(marker),
  new_color_(new_color)
{
}

Project* MarkerChangeColorCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerChangeColorCommand::redo()
{
  old_color_ = marker_->color();
  marker_->set_color(new_color_);
}

void MarkerChangeColorCommand::undo()
{
  marker_->set_color(old_color_);
}

MarkerChangeNameCommand::MarkerChangeNameCommand(TimelineMarker *marker, QString new_name) :
  marker_(marker),
  new_name_(new_name)
{
}

Project* MarkerChangeNameCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerChangeNameCommand::redo()
{
  old_name_ = marker_->name();
  marker_->set_name(new_name_);
}

void MarkerChangeNameCommand::undo()
{
  marker_->set_name(old_name_);
}

MarkerChangeTimeCommand::MarkerChangeTimeCommand(TimelineMarker* marker, TimeRange time) :
  marker_(marker),
  new_time_(time)
{
}

Project* MarkerChangeTimeCommand::GetRelevantProject() const
{
  return Project::GetProjectFromObject(marker_);
}

void MarkerChangeTimeCommand::redo()
{
  old_time_ = marker_->time_range();
  marker_->set_time(new_time_);
}

void MarkerChangeTimeCommand::undo()
{
  marker_->set_time(old_time_);
}

}
