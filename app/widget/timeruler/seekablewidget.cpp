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

#include "seekablewidget.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "common/qtutils.h"
#include "core.h"
#include "dialog/markerproperties/markerpropertiesdialog.h"
#include "node/project/serializer/serializer.h"
#include "widget/colorlabelmenu/colorlabelmenu.h"
#include "widget/menu/menushared.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

#define super TimeBasedView

SeekableWidget::SeekableWidget(QWidget* parent) :
  super(parent),
  timeline_points_(nullptr),
  dragging_(false),
  ignore_next_focus_out_(false),
  selection_manager_(this)
{
  QFontMetrics fm = fontMetrics();

  text_height_ = fm.height();

  // Set width of playhead marker
  playhead_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  setContextMenuPolicy(Qt::CustomContextMenu);
  setFocusPolicy(Qt::ClickFocus);
}

void SeekableWidget::ConnectTimelinePoints(TimelinePoints *points)
{
  if (timeline_points_) {
    selection_manager_.ClearSelection();

    disconnect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerAdded, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerRemoved, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(timeline_points_->markers(), &TimelineMarkerList::MarkerModified, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
  }

  timeline_points_ = points;

  if (timeline_points_) {
    connect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerAdded, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerRemoved, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(timeline_points_->markers(), &TimelineMarkerList::MarkerModified, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
  }

  viewport()->update();
}

void SeekableWidget::DeleteSelected()
{
  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (TimelineMarker *marker, selection_manager_.GetSelectedObjects()) {
    command->add_child(new MarkerRemoveCommand(marker));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

bool SeekableWidget::CopySelected(bool cut)
{
  if (!selection_manager_.GetSelectedObjects().empty()) {
    ProjectSerializer::SaveData sdata(Project::GetProjectFromObject(timeline_points_));
    sdata.SetOnlySerializeMarkers(selection_manager_.GetSelectedObjects());

    ProjectSerializer::Copy(sdata, QStringLiteral("markers"));

    if (cut) {
      DeleteSelected();
    }

    return true;
  } else {
    return false;
  }
}

bool SeekableWidget::PasteMarkers(bool insert, rational insert_time)
{
  ProjectSerializer::Result res = ProjectSerializer::Paste(QStringLiteral("markers"));
  if (res == ProjectSerializer::kSuccess) {
    const std::vector<TimelineMarker*> &markers = res.GetLoadData().markers;
    if (!markers.empty()) {
      MultiUndoCommand *command = new MultiUndoCommand();

      // Normalize markers to start at playhead
      rational min = RATIONAL_MAX;
      for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
        min = qMin(min, (*it)->time());
      }
      min -= GetTime();

      // Avoid duplicates
      bool loop;
      do {
        loop = false;
        for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
          rational proposed_time = (*it)->time() - min;

          if (timeline_points_->markers()->GetMarkerAtTime(proposed_time)) {
            min -= timebase();
            loop = true;
            break;
          }
        }
      } while (loop);

      for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
        TimelineMarker *m = *it;

        m->set_time(m->time() - min);

        command->add_child(new MarkerAddCommand(timeline_points_->markers(), m));
      }

      Core::instance()->undo_stack()->push(command);
      return true;
    }
  }

  return false;
}

void SeekableWidget::mousePressEvent(QMouseEvent *event)
{
  if (TimelineMarker *initial = selection_manager_.MousePress(event)) {
    selection_manager_.DragStart(initial, event);
  } else if (!selection_manager_.GetObjectAtPoint(event->pos()) && event->button() == Qt::LeftButton) {
    SeekToScenePoint(mapToScene(event->pos()).x());
    dragging_ = true;

    DeselectAllMarkers();
  }
}

void SeekableWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (selection_manager_.IsDragging()) {
    selection_manager_.DragMove(event);
  } else if (dragging_) {
    SeekToScenePoint(mapToScene(event->pos()).x());
  }
}

void SeekableWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if (selection_manager_.IsDragging()) {
    MultiUndoCommand *command = new MultiUndoCommand();
    selection_manager_.DragStop(command);
    Core::instance()->undo_stack()->pushIfHasChildren(command);
  }

  if (GetSnapService()) {
    GetSnapService()->HideSnaps();
  }

  dragging_ = false;
}

void SeekableWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  super::mouseDoubleClickEvent(event);

  if (selection_manager_.GetObjectAtPoint(event->pos()) && !selection_manager_.GetSelectedObjects().empty()) {
    ShowMarkerProperties();
  }
}

void SeekableWidget::focusOutEvent(QFocusEvent *event)
{
  super::focusOutEvent(event);

  if (ignore_next_focus_out_) {
    ignore_next_focus_out_ = false;
  } else {
    // Deselect everything when we lose focus
    DeselectAllMarkers();
  }
}

TimelinePoints *SeekableWidget::timeline_points() const
{
  return timeline_points_;
}

void SeekableWidget::DeselectAllMarkers()
{
  selection_manager_.ClearSelection();

  viewport()->update();
}

void SeekableWidget::SetMarkerColor(int c)
{
  MultiUndoCommand *command = new MultiUndoCommand();

  foreach(TimelineMarker* marker, selection_manager_.GetSelectedObjects()) {
    command->add_child(new MarkerChangeColorCommand(marker, c));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void SeekableWidget::ShowMarkerProperties()
{
  MarkerPropertiesDialog mpd(selection_manager_.GetSelectedObjects(), timebase(), this);
  ignore_next_focus_out_ = true;
  mpd.exec();
}

void SeekableWidget::TimebaseChangedEvent(const rational &t)
{
  super::TimebaseChangedEvent(t);

  selection_manager_.SetTimebase(t);
}

void SeekableWidget::SeekToScenePoint(qreal scene)
{
  if (timebase().isNull()) {
    return;
  }

  rational playhead_time = SceneToTime(scene);

  if (Core::instance()->snapping() && GetSnapService()) {
    rational movement;

    GetSnapService()->SnapPoint({playhead_time},
                                &movement,
                                SnapService::kSnapAll & ~SnapService::kSnapToPlayhead);

    playhead_time += movement;
  }

  if (playhead_time != GetTime()) {
    SetTime(playhead_time);

    emit TimeChanged(playhead_time);
  }
}

void SeekableWidget::SelectionManagerSelectEvent(void *obj)
{
  super::SelectionManagerSelectEvent(obj);

  viewport()->update();
}

void SeekableWidget::SelectionManagerDeselectEvent(void *obj)
{
  super::SelectionManagerDeselectEvent(obj);

  viewport()->update();
}

void SeekableWidget::DrawTimelinePoints(QPainter* p, int marker_bottom)
{
  if (!timeline_points()) {
    return;
  }

  int lim_left = GetScroll();
  int lim_right = lim_left + width();

  selection_manager_.ClearDrawnObjects();

  // Draw in/out workarea
  if (timeline_points()->workarea()->enabled()) {
    int workarea_left = qMax(qreal(lim_left), TimeToScene(timeline_points()->workarea()->in()));
    int workarea_right;

    if (timeline_points()->workarea()->out() == TimelineWorkArea::kResetOut) {
      workarea_right = lim_right;
    } else {
      workarea_right = qMin(qreal(lim_right), TimeToScene(timeline_points()->workarea()->out()));
    }

    p->fillRect(workarea_left, 0, workarea_right - workarea_left, height(), palette().highlight());
  }

  // Draw markers
  if (marker_bottom > 0 && !timeline_points()->markers()->empty()) {
    for (auto it=timeline_points()->markers()->cbegin(); it!=timeline_points()->markers()->cend(); it++) {
      TimelineMarker* marker = *it;

      int marker_right = TimeToScene(marker->time_range().out());
      if (marker_right < lim_left) {
        continue;
      }

      int marker_left = TimeToScene(marker->time_range().in());
      if (marker_left >= lim_right)  {
        break;
      }

      QRect marker_rect = marker->Draw(p, QPoint(marker_left, marker_bottom), GetScale(), selection_manager_.IsSelected(marker));
      selection_manager_.DeclareDrawnObject(marker, marker_rect);
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

bool SeekableWidget::ShowContextMenu(const QPoint &p)
{
  if (selection_manager_.GetObjectAtPoint(p) && !selection_manager_.GetSelectedObjects().empty()) {
    // Show marker-specific menu
    Menu m;

    ColorLabelMenu color_coding_menu;
    connect(&color_coding_menu, &ColorLabelMenu::ColorSelected, this, &SeekableWidget::SetMarkerColor);
    m.addMenu(&color_coding_menu);

    m.addSeparator();

    MenuShared::instance()->AddItemsForEditMenu(&m, false);

    m.addSeparator();

    QAction *properties_action = m.addAction(tr("Properties"));
    connect(properties_action, &QAction::triggered, this, &SeekableWidget::ShowMarkerProperties);

    ignore_next_focus_out_ = true;
    m.exec(mapToGlobal(p));
    return true;
  } else {
    return false;
  }
}

}
