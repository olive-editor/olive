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

#include "seekablewidget.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "common/qtutils.h"
#include "common/range.h"
#include "core.h"
#include "dialog/markerproperties/markerpropertiesdialog.h"
#include "node/project/serializer/serializer.h"
#include "widget/colorlabelmenu/colorlabelmenu.h"
#include "widget/menu/menushared.h"
#include "widget/timebased/timebasedwidget.h"
#include "widget/timelinewidget/undo/timelineundoworkarea.h"

namespace olive {

#define super TimeBasedView

SeekableWidget::SeekableWidget(QWidget* parent) :
  super(parent),
  timeline_points_(nullptr),
  dragging_(false),
  ignore_next_focus_out_(false),
  selection_manager_(this),
  resize_item_(nullptr),
  marker_top_(0),
  marker_bottom_(0)
{
  QFontMetrics fm = fontMetrics();

  text_height_ = fm.height();

  // Set width of playhead marker
  playhead_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  setContextMenuPolicy(Qt::CustomContextMenu);
  setFocusPolicy(Qt::ClickFocus);
  setMouseTracking(true);

  selection_manager_.SetSnapMask(TimeBasedWidget::kSnapAll);
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
  if (!selection_manager_.IsDragging()) {
    MultiUndoCommand* command = new MultiUndoCommand();

    foreach (TimelineMarker *marker, selection_manager_.GetSelectedObjects()) {
      command->add_child(new MarkerRemoveCommand(marker));
    }

    Core::instance()->undo_stack()->pushIfHasChildren(command);
  }
}

bool SeekableWidget::CopySelected(bool cut)
{
  if (!selection_manager_.GetSelectedObjects().empty()) {
    ProjectSerializer::SaveData sdata;
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

bool SeekableWidget::PasteMarkers()
{
  ProjectSerializer::Result res = ProjectSerializer::Paste(QStringLiteral("markers"));
  if (res == ProjectSerializer::kSuccess) {
    const std::vector<TimelineMarker*> &markers = res.GetLoadData().markers;
    if (!markers.empty()) {
      MultiUndoCommand *command = new MultiUndoCommand();

      // Normalize markers to start at playhead
      rational min = RATIONAL_MAX;
      for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
        min = std::min(min, (*it)->time().in());
      }
      min -= GetTime();

      for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
        TimelineMarker *m = *it;

        m->set_time(m->time().in() - min);

        if (TimelineMarker *existing = timeline_points_->markers()->GetMarkerAtTime(m->time().in())) {
          command->add_child(new MarkerRemoveCommand(existing));
        }

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
  if (resize_item_) {
    // Handle selection, even though we won't be using it for dragging
    if (!(event->modifiers() & Qt::ShiftModifier)) {
      selection_manager_.ClearSelection();
    }
    if (TimelineMarker *m = dynamic_cast<TimelineMarker*>(resize_item_)) {
      selection_manager_.Select(m);
    }
    dragging_ = true;
    resize_start_ = mapToScene(event->pos());
  } else if (TimelineMarker *initial = selection_manager_.MousePress(event)) {
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
    QPointF scene = mapToScene(event->pos());
    if (resize_item_) {
      DragResizeHandle(scene);
    } else {
      SeekToScenePoint(scene.x());
    }
  } else if (timeline_points_) {
    // Look for resize points
    setCursor(FindResizeHandle(event) ? Qt::SizeHorCursor : Qt::ArrowCursor);
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

  if (resize_item_) {
    CommitResizeHandle();
    resize_item_ = nullptr;
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

  rational playhead_time = qMax(rational(0), SceneToTime(scene));

  if (Core::instance()->snapping() && GetSnapService()) {
    rational movement;

    GetSnapService()->SnapPoint({playhead_time},
                                &movement,
                                TimeBasedWidget::kSnapAll & ~TimeBasedWidget::kSnapToPlayhead);

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
  if (!GetTimelinePoints()) {
    return;
  }

  int lim_left = GetScroll();
  int lim_right = lim_left + width();

  selection_manager_.ClearDrawnObjects();

  // Draw in/out workarea
  if (GetTimelinePoints()->workarea()->enabled()) {
    int workarea_left = qMax(qreal(lim_left), TimeToScene(GetTimelinePoints()->workarea()->in()));
    int workarea_right;

    if (GetTimelinePoints()->workarea()->out() == TimelineWorkArea::kResetOut) {
      workarea_right = lim_right;
    } else {
      workarea_right = qMin(qreal(lim_right), TimeToScene(GetTimelinePoints()->workarea()->out()));
    }

    p->fillRect(workarea_left, 0, workarea_right - workarea_left, height(), palette().highlight());
  }

  // Draw markers
  if (marker_bottom > 0 && !GetTimelinePoints()->markers()->empty()) {
    for (auto it=GetTimelinePoints()->markers()->cbegin(); it!=GetTimelinePoints()->markers()->cend(); it++) {
      TimelineMarker* marker = *it;

      int marker_right = TimeToScene(marker->time().out());
      if (marker_right < lim_left) {
        continue;
      }

      int marker_left = TimeToScene(marker->time().in());
      if (marker_left >= lim_right)  {
        break;
      }

      QRect marker_rect = marker->Draw(p, QPoint(marker_left, marker_bottom), GetScale(), selection_manager_.IsSelected(marker));
      marker_top_ = marker_rect.top();
      selection_manager_.DeclareDrawnObject(marker, marker_rect);
    }
  }

  marker_bottom_ = marker_bottom;
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

bool SeekableWidget::FindResizeHandle(QMouseEvent *event)
{
  resize_item_ = nullptr;
  resize_mode_ = kResizeNone;

  QPointF scene = mapToScene(event->pos());
  const int border = 10;
  rational min = SceneToTimeNoGrid(scene.x() - border);
  rational max = SceneToTimeNoGrid(scene.x() + border);

  // Test for workarea
  if (timeline_points_->workarea()->in() >= min && timeline_points_->workarea()->in() < max) {
    resize_mode_ = kResizeIn;
  } else if (timeline_points_->workarea()->out() >= min && timeline_points_->workarea()->out() < max) {
    resize_mode_ = kResizeOut;
  }

  if (resize_mode_ != kResizeNone) {
    resize_item_ = timeline_points_->workarea();
    resize_item_range_ = timeline_points_->workarea()->range();
    resize_snap_mask_ = TimeBasedWidget::kSnapAll & ~TimeBasedWidget::kSnapToWorkarea;
  } else if (event->pos().y() >= marker_top_ && event->pos().y() < marker_bottom_) {
    // Check for markers
    for (auto it=timeline_points_->markers()->cbegin(); it!=timeline_points_->markers()->cend(); it++) {
      TimelineMarker *m = *it;
      if (m->time().in() != m->time().out()) {
        if (m->time().in() >= min && m->time().in() < max) {
          resize_mode_ = kResizeIn;
        } else if (m->time().out() >= min && m->time().out() < max) {
          resize_mode_ = kResizeOut;
        }

        if (resize_mode_ != kResizeNone) {
          resize_item_ = m;
          resize_item_range_ = m->time();
          resize_snap_mask_ = TimeBasedWidget::kSnapAll;
          break;
        }
      }
    }
  }

  return resize_item_;
}

void SeekableWidget::DragResizeHandle(const QPointF &scene)
{
  qreal diff = scene.x() - resize_start_.x();

  rational proposed_time;

  if (resize_mode_ == kResizeIn) {
    proposed_time = qMax(rational(0), qMin(resize_item_range_.out(), resize_item_range_.in() + SceneToTimeNoGrid(diff)));
  } else {
    proposed_time = qMax(resize_item_range_.in(), resize_item_range_.out() + SceneToTimeNoGrid(diff));
  }

  rational presnap_time = proposed_time;

  if (Core::instance()->snapping() && GetSnapService()) {
    rational movement;

    GetSnapService()->SnapPoint({proposed_time}, &movement, resize_snap_mask_);

    proposed_time += movement;
  }

  TimeRange new_range = resize_item_range_;
  if (resize_mode_ == kResizeIn) {
    // Markers should not have the same time as anything else
    // NOTE: This code is largely duplicated from TimeBasedViewSelectionManager::DragMove. Not ideal,
    //       but I'm not sure if there's a good way to re-use that code
    if (TimelineMarker *marker = dynamic_cast<TimelineMarker*>(resize_item_)) {
      if (marker->has_sibling_at_time(proposed_time)) {
        proposed_time = presnap_time;

        if (GetSnapService()) {
          GetSnapService()->HideSnaps();
        }
      }

      while (marker->has_sibling_at_time(proposed_time)) {
        proposed_time += rational(1, 1000);
      }
    }

    new_range.set_in(proposed_time);
  } else {
    new_range.set_out(proposed_time);
  }

  if (TimelineMarker *marker = dynamic_cast<TimelineMarker*>(resize_item_)) {
    marker->set_time(new_range);
  } else if (TimelineWorkArea *workarea = dynamic_cast<TimelineWorkArea*>(resize_item_)) {
    workarea->set_range(new_range);
  }
}

void SeekableWidget::CommitResizeHandle()
{
  MultiUndoCommand *command = new MultiUndoCommand();

  if (TimelineMarker *marker = dynamic_cast<TimelineMarker*>(resize_item_)) {
    command->add_child(new MarkerChangeTimeCommand(marker, marker->time(), resize_item_range_));
  } else if (TimelineWorkArea *workarea = dynamic_cast<TimelineWorkArea*>(resize_item_)) {
    command->add_child(new WorkareaSetRangeCommand(workarea, workarea->range(), resize_item_range_));
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

}
