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
#include "timeline/timelineundoworkarea.h"
#include "widget/colorlabelmenu/colorlabelmenu.h"
#include "widget/menu/menushared.h"
#include "widget/timebased/timebasedwidget.h"

namespace olive {

#define super TimeBasedView

SeekableWidget::SeekableWidget(QWidget* parent) :
  super(parent),
  markers_(nullptr),
  workarea_(nullptr),
  dragging_(false),
  ignore_next_focus_out_(false),
  selection_manager_(this),
  resize_item_(nullptr),
  marker_top_(0),
  marker_bottom_(0),
  marker_editing_enabled_(true)
{
  QFontMetrics fm = fontMetrics();

  text_height_ = fm.height();

  // Set width of playhead marker
  playhead_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  setContextMenuPolicy(Qt::CustomContextMenu);
  setFocusPolicy(Qt::ClickFocus);
  setMouseTracking(true);

  selection_manager_.SetSnapMask(TimeBasedWidget::kSnapAll);

  SetIsTimelineAxes(true);
}

void SeekableWidget::SetMarkers(TimelineMarkerList *markers)
{
  if (markers_) {
    selection_manager_.ClearSelection();

    disconnect(markers_, &TimelineMarkerList::MarkerAdded, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(markers_, &TimelineMarkerList::MarkerRemoved, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(markers_, &TimelineMarkerList::MarkerModified, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
  }

  markers_ = markers;

  if (markers_) {
    connect(markers_, &TimelineMarkerList::MarkerAdded, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(markers_, &TimelineMarkerList::MarkerRemoved, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(markers_, &TimelineMarkerList::MarkerModified, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
  }

  viewport()->update();
}

void SeekableWidget::SetWorkArea(TimelineWorkArea *workarea)
{
  if (workarea_) {
    selection_manager_.ClearSelection();

    disconnect(workarea_, &TimelineWorkArea::RangeChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    disconnect(workarea_, &TimelineWorkArea::EnabledChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
  }

  workarea_ = workarea;

  if (workarea_) {
    connect(workarea_, &TimelineWorkArea::RangeChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(workarea_, &TimelineWorkArea::EnabledChanged, viewport(), static_cast<void (QWidget::*)()>(&QWidget::update));
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

    Core::instance()->undo_stack()->push(command, tr("Deleted %1 Marker(s)").arg(selection_manager_.GetSelectedObjects().size()));
  }
}

bool SeekableWidget::CopySelected(bool cut)
{
  if (!selection_manager_.GetSelectedObjects().empty()) {
    ProjectSerializer::SaveData sdata(ProjectSerializer::kOnlyMarkers);
    sdata.SetOnlySerializeMarkers(selection_manager_.GetSelectedObjects());

    ProjectSerializer::Copy(sdata);

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
  ProjectSerializer::Result res = ProjectSerializer::Paste(ProjectSerializer::kOnlyMarkers);
  if (res == ProjectSerializer::kSuccess) {
    const std::vector<TimelineMarker*> &markers = res.GetLoadData().markers;
    if (!markers.empty()) {
      MultiUndoCommand *command = new MultiUndoCommand();

      // Normalize markers to start at playhead
      rational min = RATIONAL_MAX;
      for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
        min = std::min(min, (*it)->time().in());
      }
      min -= GetViewerNode()->GetPlayhead();

      for (auto it=markers.cbegin(); it!=markers.cend(); it++) {
        TimelineMarker *m = *it;

        m->set_time(m->time().in() - min);

        if (TimelineMarker *existing = markers_->GetMarkerAtTime(m->time().in())) {
          command->add_child(new MarkerRemoveCommand(existing));
        }

        command->add_child(new MarkerAddCommand(markers_, m));
      }

      Core::instance()->undo_stack()->push(command, tr("Pasted %1 Marker(s)").arg(markers.size()));
      return true;
    }
  }

  return false;
}

void SeekableWidget::mousePressEvent(QMouseEvent *event)
{
  TimelineMarker *initial;

  if (HandPress(event)) {
    return;
  } else if (event->modifiers() & Qt::ControlModifier) {
    selection_manager_.RubberBandStart(event);
  } else if (marker_editing_enabled_ && (initial = selection_manager_.MousePress(event))) {
    selection_manager_.DragStart(initial, event);
  } else if (resize_item_) {
    // Handle selection, even though we won't be using it for dragging
    if (!(event->modifiers() & Qt::ShiftModifier)) {
      selection_manager_.ClearSelection();
    }
    if (TimelineMarker *m = dynamic_cast<TimelineMarker*>(resize_item_)) {
      selection_manager_.Select(m);
    }
    dragging_ = true;
    resize_start_ = mapToScene(event->pos());
  } else if (!selection_manager_.GetObjectAtPoint(event->pos()) && event->button() == Qt::LeftButton) {
    SeekToScenePoint(mapToScene(event->pos()).x());
    dragging_ = true;

    DeselectAllMarkers();
  }
}

void SeekableWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (HandMove(event)) {
    return;
  } else if (selection_manager_.IsRubberBanding()) {
    selection_manager_.RubberBandMove(event->pos());
    viewport()->update();
  } else if (selection_manager_.IsDragging()) {
    selection_manager_.DragMove(event->pos());
  } else if (dragging_) {
    QPointF scene = mapToScene(event->pos());
    if (resize_item_) {
      DragResizeHandle(scene);
    } else {
      SeekToScenePoint(scene.x());
    }
  } else {
    // Look for resize points
    if (!last_playhead_shape_.containsPoint(event->pos(), Qt::OddEvenFill)
        && !selection_manager_.GetObjectAtPoint(event->pos())
        && FindResizeHandle(event)) {
      setCursor(Qt::SizeHorCursor);
    } else {
      unsetCursor();
      ClearResizeHandle();
    }
  }

  if (event->buttons()) {
    // Signal cursor pos in case we should scroll to catch up to it
    emit DragMoved(event->pos().x(), event->pos().y());
  }
}

void SeekableWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if (HandRelease(event)) {
    return;
  }

  if (selection_manager_.IsRubberBanding()) {
    selection_manager_.RubberBandStop();
    return;
  }

  if (selection_manager_.IsDragging()) {
    MultiUndoCommand *command = new MultiUndoCommand();
    selection_manager_.DragStop(command);
    Core::instance()->undo_stack()->push(command, tr("Moved %1 Marker(s)").arg(selection_manager_.GetSelectedObjects().size()));
  }

  if (GetSnapService()) {
    GetSnapService()->HideSnaps();
  }

  if (resize_item_) {
    CommitResizeHandle();
    resize_item_ = nullptr;
  }

  dragging_ = false;
  emit DragReleased();
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

void SeekableWidget::DrawMarkers(QPainter *p, int marker_bottom)
{
  selection_manager_.ClearDrawnObjects();

  // Draw markers
  if (markers_ && !markers_->empty() && marker_bottom > 0) {
    int lim_left = GetLeftLimit();
    int lim_right = GetRightLimit();

    for (auto it=markers_->cbegin(); it!=markers_->cend(); it++) {
      TimelineMarker* marker = *it;

      int marker_right = TimeToScene(marker->time().out());
      if (marker_right < lim_left) {
        continue;
      }

      int marker_left = TimeToScene(marker->time().in());
      if (marker_left >= lim_right)  {
        break;
      }

      int max_marker_right = lim_right;
      {
        // Check if there's a marker next
        auto next = it;
        next++;
        if (next != markers_->cend()) {
          max_marker_right = std::min(max_marker_right, int(TimeToScene((*next)->time().in())));
        }
      }

      QRect marker_rect = marker->Draw(p, QPoint(marker_left, marker_bottom), max_marker_right, GetScale(), selection_manager_.IsSelected(marker));
      marker_top_ = marker_rect.top();
      selection_manager_.DeclareDrawnObject(marker, marker_rect);
    }
  }

  marker_bottom_ = marker_bottom;
}

void SeekableWidget::DrawWorkArea(QPainter *p)
{
  // Draw in/out workarea
  if (workarea_ && workarea_->enabled()) {
    int lim_left = GetLeftLimit();
    int lim_right = GetRightLimit();

    int workarea_left = qMax(qreal(lim_left), TimeToScene(workarea_->in()));
    int workarea_right;

    if (workarea_->out() == TimelineWorkArea::kResetOut) {
      workarea_right = lim_right;
    } else {
      workarea_right = qMin(qreal(lim_right), TimeToScene(workarea_->out()));
    }

    QColor translucent_highlight = palette().highlight().color();
    translucent_highlight.setAlpha(96);
    p->fillRect(workarea_left, 0, workarea_right - workarea_left, height(), translucent_highlight);
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

  Core::instance()->undo_stack()->push(command, tr("Changed Color of %1 Marker(s)").arg(selection_manager_.GetSelectedObjects().size()));
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

  if (playhead_time != GetViewerNode()->GetPlayhead()) {
    GetViewerNode()->SetPlayhead(playhead_time);
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

void SeekableWidget::CatchUpScrollEvent()
{
  super::CatchUpScrollEvent();

  this->selection_manager_.ForceDragUpdate();
}

void SeekableWidget::DrawPlayhead(QPainter *p, int x, int y)
{
  int half_width = playhead_width_ / 2;

  {
    int test = x - this->GetScroll();
    if (test + half_width < 0 || test - half_width > width()) {
      return;
    }
  }

  p->setRenderHint(QPainter::Antialiasing);

  int half_text_height = text_height() / 3;

  last_playhead_shape_ = QPolygon({
    QPoint(x, y),
    QPoint(x - half_width, y - half_text_height),
    QPoint(x - half_width, y - text_height()),
    QPoint(x + 1 + half_width, y - text_height()),
    QPoint(x + 1 + half_width, y - half_text_height),
    QPoint(x + 1, y),
  });

  p->drawPolygon(last_playhead_shape_);

  p->setRenderHint(QPainter::Antialiasing, false);
}

int SeekableWidget::GetLeftLimit() const
{
  return GetScroll();
}

int SeekableWidget::GetRightLimit() const
{
  return GetLeftLimit() + width();
}

bool SeekableWidget::ShowContextMenu(const QPoint &p)
{
  if (marker_editing_enabled_ && selection_manager_.GetObjectAtPoint(p) && !selection_manager_.GetSelectedObjects().empty()) {
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
  if (!marker_editing_enabled_) {
    return false;
  }

  ClearResizeHandle();

  QPointF scene = mapToScene(event->pos());
  const int border = 10;
  rational min = SceneToTimeNoGrid(scene.x() - border);
  rational max = SceneToTimeNoGrid(scene.x() + border);

  // Test for workarea
  if (workarea_ && workarea_->enabled()) {
    if (workarea_->in() >= min && workarea_->in() < max) {
      resize_mode_ = kResizeIn;
    } else if (workarea_->out() >= min && workarea_->out() < max) {
      resize_mode_ = kResizeOut;
    }
  }

  if (resize_mode_ != kResizeNone) {
    if (workarea_) {
      resize_item_ = workarea_;
      resize_item_range_ = workarea_->range();
      resize_snap_mask_ = TimeBasedWidget::kSnapAll & ~TimeBasedWidget::kSnapToWorkarea;
    }
  } else if (event->pos().y() >= marker_top_ && event->pos().y() < marker_bottom_) {
    if (markers_) {
      // Check for markers
      for (auto it=markers_->cbegin(); it!=markers_->cend(); it++) {
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
  }

  return resize_item_;
}

void SeekableWidget::ClearResizeHandle()
{
  resize_item_ = nullptr;
  resize_mode_ = kResizeNone;
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

  QString command_name;

  if (TimelineMarker *marker = dynamic_cast<TimelineMarker*>(resize_item_)) {
    command->add_child(new MarkerChangeTimeCommand(marker, marker->time(), resize_item_range_));
    command_name = tr("Changed Marker Length");
  } else if (TimelineWorkArea *workarea = dynamic_cast<TimelineWorkArea*>(resize_item_)) {
    command->add_child(new WorkareaSetRangeCommand(workarea, workarea->range(), resize_item_range_));
    command_name = tr("Changed Workarea Length");
  }

  Core::instance()->undo_stack()->push(command, command_name);
}

}
