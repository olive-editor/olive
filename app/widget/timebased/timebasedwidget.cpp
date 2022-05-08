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

#include "timebasedwidget.h"

#include <QInputDialog>

#include "common/autoscroll.h"
#include "common/range.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "dialog/markerproperties/markerpropertiesdialog.h"
#include "node/project/sequence/sequence.h"
#include "widget/timeruler/timeruler.h"
#include "widget/timelinewidget/undo/timelineundoworkarea.h"

namespace olive {

TimeBasedWidget::TimeBasedWidget(bool ruler_text_visible, bool ruler_cache_status_visible, QWidget *parent) :
  TimelineScaledWidget(parent),
  viewer_node_(nullptr),
  auto_max_scrollbar_(false),
  toggle_show_all_(false),
  auto_set_timebase_(true)
{
  ruler_ = new TimeRuler(ruler_text_visible, ruler_cache_status_visible, this);
  ConnectTimelineView(ruler_, true);
  ruler()->SetSnapService(this);

  scrollbar_ = new ResizableTimelineScrollBar(Qt::Horizontal, this);
  connect(scrollbar_, &ResizableScrollBar::ResizeBegan, this, &TimeBasedWidget::ScrollBarResizeBegan);
  connect(scrollbar_, &ResizableScrollBar::ResizeMoved, this, &TimeBasedWidget::ScrollBarResizeMoved);

  PassWheelEventsToScrollBar(ruler_);
}

void TimeBasedWidget::SetScaleAndCenterOnPlayhead(const double &scale)
{
  SetScale(scale);

  // Zoom towards the playhead
  // (using a hacky singleShot so the scroll occurs after the scene and its scrollbars have updated)
  QTimer::singleShot(0, this, &TimeBasedWidget::CenterScrollOnPlayhead);
}

const rational &TimeBasedWidget::GetTime() const
{
  return ruler_->GetTime();
}

ViewerOutput *TimeBasedWidget::GetConnectedNode() const
{
  return viewer_node_;
}

void TimeBasedWidget::ConnectViewerNode(ViewerOutput *node)
{
  // Ignore no-op
  if (viewer_node_ == node) {
    return;
  }

  // Set viewer node
  ViewerOutput* old = viewer_node_;
  viewer_node_ = node;

  if (old) {
    // Call potential derivative functions for disconnecting the viewer node
    DisconnectNodeEvent(old);

    // Disconnect length changed signal
    disconnect(old, &ViewerOutput::LengthChanged, this, &TimeBasedWidget::UpdateMaximumScroll);
    disconnect(old, &ViewerOutput::RemovedFromGraph, this, &TimeBasedWidget::ConnectedNodeRemovedFromGraph);

    // Disconnect rate change signals if they were connected
    disconnect(old, &ViewerOutput::FrameRateChanged, this, &TimeBasedWidget::AutoUpdateTimebase);
    disconnect(old, &ViewerOutput::SampleRateChanged, this, &TimeBasedWidget::AutoUpdateTimebase);

    // Reset timebase to null
    SetTimebase(rational());

    // Disconnect ruler and scrollbar from timeline points
    ruler()->ConnectTimelinePoints(nullptr);
    scrollbar_->ConnectTimelinePoints(nullptr);
  }

  // Call derivatives
  ConnectedNodeChangeEvent(viewer_node_);

  if (viewer_node_) {
    // Connect length changed signal
    connect(viewer_node_, &ViewerOutput::LengthChanged, this, &TimeBasedWidget::UpdateMaximumScroll);
    connect(viewer_node_, &ViewerOutput::RemovedFromGraph, this, &TimeBasedWidget::ConnectedNodeRemovedFromGraph);

    // Connect ruler and scrollbar to timeline points
    ruler()->ConnectTimelinePoints(viewer_node_->GetTimelinePoints());
    scrollbar_->ConnectTimelinePoints(viewer_node_->GetTimelinePoints());

    // If we're setting the timebase, set it automatically based on the video and audio parameters
    if (auto_set_timebase_) {
      AutoUpdateTimebase();
      connect(viewer_node_, &ViewerOutput::FrameRateChanged, this, &TimeBasedWidget::AutoUpdateTimebase);
      connect(viewer_node_, &ViewerOutput::SampleRateChanged, this, &TimeBasedWidget::AutoUpdateTimebase);
    }

    // Call derivatives
    ConnectNodeEvent(viewer_node_);
  }

  UpdateMaximumScroll();

  emit ConnectedNodeChanged(old, node);
}

void TimeBasedWidget::UpdateMaximumScroll()
{
  rational length = (viewer_node_) ? viewer_node_->GetLength() : 0;

  if (auto_max_scrollbar_) {
    scrollbar_->setMaximum(qMax(0, qCeil(TimeToScene(length)) - width()));
  }

  foreach (TimeBasedView* base, timeline_views_) {
    base->SetEndTime(length);
  }
}

void TimeBasedWidget::ScrollBarResizeBegan(int current_bar_width, bool top_handle)
{
  QScrollBar* bar = static_cast<QScrollBar*>(sender());

  scrollbar_start_width_ = current_bar_width;
  scrollbar_start_value_ = bar->value();
  scrollbar_start_scale_ = GetScale();
  scrollbar_top_handle_ = top_handle;
}

void TimeBasedWidget::ScrollBarResizeMoved(int movement)
{
  ResizableScrollBar* bar = static_cast<ResizableScrollBar*>(sender());

  // Negate movement for the top handle
  if (scrollbar_top_handle_) {
    movement = -movement;
  }

  // The user wants the bar to be this size
  int proposed_size = scrollbar_start_width_ + movement;

  double ratio = double(scrollbar_start_width_) / double(proposed_size);

  if (ratio > 0) {
    SetScale(scrollbar_start_scale_ * ratio);

    if (scrollbar_top_handle_) {
      int viewable_area;

      if (timeline_views_.isEmpty()) {
        viewable_area = width();
      } else {
        viewable_area = timeline_views_.first()->width();
      }

      bar->setValue((scrollbar_start_value_ + viewable_area) * ratio - viewable_area);
    } else {
      bar->setValue(scrollbar_start_value_ * ratio);
    }
  }
}

void TimeBasedWidget::PageScrollToPlayhead()
{
  PageScrollInternal(qRound(TimeToScene(GetTime())), true);
}

void TimeBasedWidget::CatchUpScrollToPlayhead()
{
  CatchUpScrollToPoint(qRound(TimeToScene(GetTime())));
}

void TimeBasedWidget::CatchUpScrollToPoint(int point)
{
  PageScrollInternal(point, false);
}

void TimeBasedWidget::AutoUpdateTimebase()
{
  rational video_tb = viewer_node_->GetVideoParams().frame_rate_as_time_base();

  if (!video_tb.isNull()) {
    SetTimebase(video_tb);
  } else {
    rational audio_tb = viewer_node_->GetAudioParams().sample_rate_as_time_base();

    if (!audio_tb.isNull()) {
      SetTimebase(audio_tb);
    } else {
      SetTimebase(rational());
    }
  }
}

void TimeBasedWidget::ConnectedNodeRemovedFromGraph()
{
  ConnectViewerNode(nullptr);
}

TimeRuler *TimeBasedWidget::ruler() const
{
  return ruler_;
}

ResizableTimelineScrollBar *TimeBasedWidget::scrollbar() const
{
  return scrollbar_;
}

void TimeBasedWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimelineScaledWidget::TimebaseChangedEvent(timebase);

  ruler_->SetTimebase(timebase);
  scrollbar_->SetTimebase(timebase);

  emit TimebaseChanged(timebase);
}

void TimeBasedWidget::ScaleChangedEvent(const double &scale)
{
  TimelineScaledWidget::ScaleChangedEvent(scale);

  ruler_->SetScale(scale);
  scrollbar_->SetScale(scale);

  UpdateMaximumScroll();

  toggle_show_all_ = false;
}

void TimeBasedWidget::SetAutoMaxScrollBar(bool e)
{
  auto_max_scrollbar_ = e;
}

void TimeBasedWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  // Update horizontal scrollbar's page step to the width of the panel
  scrollbar()->setPageStep(scrollbar()->width());

  UpdateMaximumScroll();
}

void TimeBasedWidget::ConnectTimelineView(TimeBasedView *base, bool connect_time_change_event)
{
  if (connect_time_change_event) {
    connect(base, &TimeBasedView::TimeChanged, this, &TimeBasedWidget::SetTimeAndSignal);
  }

  timeline_views_.append(base);
}

void TimeBasedWidget::PassWheelEventsToScrollBar(QObject *object)
{
  wheel_passthrough_objects_.append(object);
  object->installEventFilter(this);
}

void TimeBasedWidget::SetTime(const rational &time)
{
  if (UserIsDraggingPlayhead()) {
    // If the user is dragging the playhead, we will simply nudge over and not use autoscroll rules.
    QMetaObject::invokeMethod(this, "CatchUpScrollToPlayhead", Qt::QueuedConnection);
  } else {
    // Otherwise, assume we jumped to this out of nowhere and must now autoscroll
    switch (static_cast<AutoScroll::Method>(OLIVE_CONFIG("Autoscroll").toInt())) {
    case AutoScroll::kNone:
      // Do nothing
      break;
    case AutoScroll::kPage:
      QMetaObject::invokeMethod(this, "PageScrollToPlayhead", Qt::QueuedConnection);
      break;
    case AutoScroll::kSmooth:
      QMetaObject::invokeMethod(this, "CenterScrollOnPlayhead", Qt::QueuedConnection);
      break;
    }
  }

  ruler_->SetTime(time);

  TimeChangedEvent(time);
}

void TimeBasedWidget::SetTimebase(const rational &timebase)
{
  TimelineScaledWidget::SetTimebase(timebase);
}

void TimeBasedWidget::SetScale(const double &scale)
{
  // Simple QObject slot wrapper around TimelineScaledWidget::SetScale()
  TimelineScaledWidget::SetScale(scale);
}

void TimeBasedWidget::ZoomIn()
{
  SetScaleAndCenterOnPlayhead(GetScale() * 2);
}

void TimeBasedWidget::ZoomOut()
{
  SetScaleAndCenterOnPlayhead(GetScale() * 0.5);
}

void TimeBasedWidget::GoToPrevCut()
{
  // Cuts are only possible in sequences
  Sequence* sequence = dynamic_cast<Sequence*>(viewer_node_);

  if (!sequence) {
    return;
  }

  if (GetTime().isNull()) {
    return;
  }

  rational closest_cut = 0;

  foreach (Track* track, sequence->GetTracks()) {
    rational this_track_closest_cut = 0;

    foreach (Block* block, track->Blocks()) {
      if (block->out() < GetTime()) {
        this_track_closest_cut = block->out();
      } else {
        break;
      }
    }

    closest_cut = qMax(closest_cut, this_track_closest_cut);
  }

  SetTimeAndSignal(closest_cut);
}

void TimeBasedWidget::GoToNextCut()
{
  // Cuts are only possible in sequences
  Sequence* sequence = dynamic_cast<Sequence*>(viewer_node_);

  if (!sequence) {
    return;
  }

  rational closest_cut = RATIONAL_MAX;

  foreach (Track* track, sequence->GetTracks()) {
    rational this_track_closest_cut = track->track_length();

    if (this_track_closest_cut <= GetTime()) {
      this_track_closest_cut = RATIONAL_MAX;
    }

    foreach (Block* block, track->Blocks()) {
      if (block->in() > GetTime()) {
        this_track_closest_cut = block->in();
        break;
      }
    }

    closest_cut = qMin(closest_cut, this_track_closest_cut);
  }

  if (closest_cut < RATIONAL_MAX) {
    SetTimeAndSignal(closest_cut);
  }
}

void TimeBasedWidget::GoToStart()
{
  if (viewer_node_) {
    SetTimeAndSignal(0);
  }
}

void TimeBasedWidget::PrevFrame()
{
  if (viewer_node_) {
    rational proposed_time = Timecode::snap_time_to_timebase(GetTime() - timebase(), timebase(), Timecode::kCeil);
    if (proposed_time == GetTime()) {
      // Catch rounding error, assume this time is snapped and just subtract a timebase
      proposed_time -= timebase();
    }
    SetTimeAndSignal(qMax(rational(0), proposed_time));
  }
}

void TimeBasedWidget::NextFrame()
{
  if (viewer_node_) {
    rational proposed_time = Timecode::snap_time_to_timebase(GetTime() + timebase(), timebase(), Timecode::kFloor);
    if (proposed_time == GetTime()) {
      // Catch rounding error, assume this time is snapped and just add a timebase
      proposed_time += timebase();
    }
    SetTimeAndSignal(proposed_time);
  }
}

void TimeBasedWidget::GoToEnd()
{
  if (viewer_node_) {
    SetTimeAndSignal(viewer_node_->GetLength());
  }
}

void TimeBasedWidget::SetTimeAndSignal(const rational &t)
{
  SetTime(t);
  emit TimeChanged(t);
}

void TimeBasedWidget::CenterScrollOnPlayhead()
{
  scrollbar_->setValue(qRound(TimeToScene(ruler_->GetTime())) - scrollbar_->width()/2);
}

void TimeBasedWidget::SetAutoSetTimebase(bool e)
{
  auto_set_timebase_ = e;
}

void TimeBasedWidget::SetPoint(Timeline::MovementMode m, const rational& time)
{
  if (!viewer_node_) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();
  TimelinePoints* points = viewer_node_->GetTimelinePoints();

  // Enable workarea if it isn't already enabled
  if (!points->workarea()->enabled()) {
    command->add_child(new WorkareaSetEnabledCommand(viewer_node_->project(), points, true));
  }

  // Determine our new range
  rational in_point, out_point;

  if (m == Timeline::kTrimIn) {
    in_point = time;

    if (!points->workarea()->enabled() || points->workarea()->out() < in_point) {
      out_point = TimelineWorkArea::kResetOut;
    } else {
      out_point = points->workarea()->out();
    }
  } else {
    out_point = time;

    if (!points->workarea()->enabled() || points->workarea()->in() > out_point) {
      in_point = TimelineWorkArea::kResetIn;
    } else {
      in_point = points->workarea()->in();
    }
  }

  // Set workarea
  command->add_child(new WorkareaSetRangeCommand(viewer_node_->project(), points, TimeRange(in_point, out_point)));

  Core::instance()->undo_stack()->push(command);
}

void TimeBasedWidget::ResetPoint(Timeline::MovementMode m)
{
  if (!GetConnectedNode()) {
    return;
  }

  TimelinePoints* points = GetConnectedNode()->GetTimelinePoints();

  if (!GetConnectedNode() || !points->workarea()->enabled()) {
    return;
  }

  TimeRange r = points->workarea()->range();

  if (m == Timeline::kTrimIn) {
    r.set_in(TimelineWorkArea::kResetIn);
  } else {
    r.set_out(TimelineWorkArea::kResetOut);
  }

  Core::instance()->undo_stack()->push(new WorkareaSetRangeCommand(viewer_node_->project(), points, r));
}

void TimeBasedWidget::PageScrollInternal(QScrollBar *bar, int maximum, int screen_position, bool whole_page_scroll)
{
  int viewport_padding = maximum / 16;

  if (whole_page_scroll) {
    if (screen_position < bar->value()) {
      // Anchor the playhead to the RIGHT of where we scroll to
      bar->setValue(screen_position - maximum + viewport_padding);
    } else if (screen_position > bar->value() + maximum) {
      // Anchor the playhead to the LEFT of where we scroll to
      bar->setValue(screen_position - viewport_padding);
    }
  } else {
    // Just jump in increments
    if (screen_position < bar->value() + viewport_padding) {
      bar->setValue(bar->value() - viewport_padding);
    } else if (screen_position > bar->value() + maximum - viewport_padding) {
      bar->setValue(bar->value() + viewport_padding);
    }
  }
}

void TimeBasedWidget::PageScrollInternal(int screen_position, bool whole_page_scroll)
{
  PageScrollInternal(scrollbar(), ruler()->width(), screen_position, whole_page_scroll);
}

bool TimeBasedWidget::UserIsDraggingPlayhead() const
{
  if (ruler_->IsDraggingPlayhead()) {
    return true;
  }

  foreach (TimeBasedView* view, timeline_views_) {
    if (view->IsDraggingPlayhead()) {
      return true;
    }
  }

  return false;
}

void TimeBasedWidget::SetInAtPlayhead()
{
  SetPoint(Timeline::kTrimIn, GetTime());
}

void TimeBasedWidget::SetOutAtPlayhead()
{
  SetPoint(Timeline::kTrimOut, GetTime());
}

void TimeBasedWidget::ResetIn()
{
  ResetPoint(Timeline::kTrimIn);
}

void TimeBasedWidget::ResetOut()
{
  ResetPoint(Timeline::kTrimOut);
}

void TimeBasedWidget::ClearInOutPoints()
{
  if (!GetConnectedNode()) {
    return;
  }


  Core::instance()->undo_stack()->push(new WorkareaSetEnabledCommand(GetConnectedNode()->project(), GetConnectedNode()->GetTimelinePoints(), false));
}

void TimeBasedWidget::SetMarker()
{
  if (!GetConnectedNode()) {
    return;
  }

  TimelineMarkerList *markers = GetConnectedNode()->GetTimelinePoints()->markers();

  if (TimelineMarker *existing = markers->GetMarkerAtTime(GetTime())) {
    // We already have a marker here, so pop open the edit dialog
    MarkerPropertiesDialog mpd({existing}, timebase(), this);
    mpd.exec();
  } else {
    // Create a new marker and place it here
    int color;
    if (TimelineMarker *closest = markers->GetClosestMarkerToTime(GetTime())) {
      // Copy color of closest marker to this time
      color = closest->color();
    } else {
      // Fallback to default color in preferences
      color = OLIVE_CONFIG("MarkerColor").toInt();
    }

    TimelineMarker *marker = new TimelineMarker(color, TimeRange(GetTime(), GetTime()));

    if (OLIVE_CONFIG("SetNameWithMarker").toBool()) {
      MarkerPropertiesDialog mpd({marker}, timebase(), this);
      if (mpd.exec() != QDialog::Accepted) {
        delete marker;
        marker = nullptr;
      }
    }

    if (marker) {
      Core::instance()->undo_stack()->push(new MarkerAddCommand(markers, marker));
    }
  }
}

void TimeBasedWidget::ToggleShowAll()
{
  if (!GetConnectedNode()) {
    return;
  }

  if (toggle_show_all_) {
    SetScale(toggle_show_all_old_scale_);
    scrollbar_->setValue(toggle_show_all_old_scroll_);

    // Don't have to set toggle_show_all_ because SetScale() will automatically set it to false
  } else {
    int w;

    if (timeline_views_.isEmpty()) {
      w = width();
    } else {
      w = timeline_views_.first()->width();
    }



    toggle_show_all_old_scale_ = GetScale();
    toggle_show_all_old_scroll_ = scrollbar_->value();

    SetScaleFromDimensions(w, GetConnectedNode()->GetLength().toDouble());
    scrollbar_->setValue(0);

    // Must explicitly do this because SetScale() will automatically set this to false
    toggle_show_all_ = true;
  }
}

void TimeBasedWidget::GoToIn()
{
  if (GetConnectedNode()) {
    if (GetConnectedNode()->GetTimelinePoints()->workarea()->enabled()) {
      SetTimeAndSignal(GetConnectedNode()->GetTimelinePoints()->workarea()->in());
    } else {
      GoToStart();
    }
  }
}

void TimeBasedWidget::GoToOut()
{
  if (GetConnectedNode()) {
    if (GetConnectedNode()->GetTimelinePoints()->workarea()->enabled()) {
      SetTimeAndSignal(GetConnectedNode()->GetTimelinePoints()->workarea()->out());
    } else {
      GoToEnd();
    }
  }
}

void TimeBasedWidget::DeleteSelected()
{
  if (ruler_->underMouse()) {
    ruler_->DeleteSelected();
  }
}

bool TimeBasedWidget::eventFilter(QObject *object, QEvent *event)
{
  if (wheel_passthrough_objects_.contains(object) && event->type() == QEvent::Wheel) {
    QCoreApplication::sendEvent(scrollbar(), event);
  }

  return false;
}

struct SnapData {
  rational time;
  rational movement;
};

void AttemptSnap(std::vector<SnapData> &snap_data,
                 const std::vector<double>& screen_pt,
                 double compare_pt,
                 const std::vector<rational>& start_times,
                 const rational& compare_time)
{
  const qreal kSnapRange = 10; // FIXME: Hardcoded number

  for (size_t i=0;i<screen_pt.size();i++) {
    // Attempt snapping to clip out point
    if (InRange(screen_pt.at(i), compare_pt, kSnapRange)) {
      snap_data.push_back({compare_time, compare_time - start_times.at(i)});
    }
  }
}

bool TimeBasedWidget::SnapPoint(const std::vector<rational> &start_times, rational *movement, SnapMask snap_points)
{
  std::vector<double> screen_pt(start_times.size());

  for (size_t i=0; i<start_times.size(); i++) {
    screen_pt[i] = TimeToScene(start_times.at(i) + *movement);
  }

  std::vector<SnapData> potential_snaps;

  if (snap_points & kSnapToPlayhead) {
    rational playhead_abs_time = GetTime();
    qreal playhead_pos = TimeToScene(playhead_abs_time);
    AttemptSnap(potential_snaps, screen_pt, playhead_pos, start_times, playhead_abs_time);
  }

  if ((snap_points & kSnapToClips) && GetSnapBlocks()) {
    for (auto it=GetSnapBlocks()->cbegin(); it!=GetSnapBlocks()->cend(); it++) {
      Block *b = *it;

      qreal rect_left = TimeToScene(b->in());
      qreal rect_right = TimeToScene(b->out());

      // Attempt snapping to clip in point
      AttemptSnap(potential_snaps, screen_pt, rect_left, start_times, b->in());

      // Attempt snapping to clip out point
      AttemptSnap(potential_snaps, screen_pt, rect_right, start_times, b->out());

      if (snap_points & kSnapToMarkers) {
        // Snap to clip markers too
        if (ClipBlock *clip = dynamic_cast<ClipBlock*>(b)) {
          if (clip->connected_viewer()) {
            TimelineMarkerList *markers = clip->connected_viewer()->GetTimelinePoints()->markers();
            for (auto jt=markers->cbegin(); jt!=markers->cend(); jt++) {
              TimelineMarker *marker = *jt;

              TimeRange marker_range = marker->time_range() + clip->in() - clip->media_in();

              qreal marker_in_screen = TimeToScene(marker_range.in());
              qreal marker_out_screen = TimeToScene(marker_range.out());

              AttemptSnap(potential_snaps, screen_pt, marker_in_screen, start_times, marker_range.in());
              AttemptSnap(potential_snaps, screen_pt, marker_out_screen, start_times, marker_range.out());
            }
          }
        }
      }
    }
  }

  if ((snap_points & kSnapToMarkers) && ruler()->GetTimelinePoints()) {
    for (auto it=ruler()->GetTimelinePoints()->markers()->cbegin(); it!=ruler()->GetTimelinePoints()->markers()->cend(); it++) {
      TimelineMarker* m = *it;

      qreal marker_pos = TimeToScene(m->time_range().in());
      AttemptSnap(potential_snaps, screen_pt, marker_pos, start_times, m->time_range().in());

      if (m->time_range().in() != m->time_range().out()) {
        marker_pos = TimeToScene(m->time_range().out());
        AttemptSnap(potential_snaps, screen_pt, marker_pos, start_times, m->time_range().out());
      }
    }
  }

  if ((snap_points & kSnapToKeyframes) && GetSnapKeyframes()) {
    for (auto it=GetSnapKeyframes()->cbegin(); it!=GetSnapKeyframes()->cend(); it++) {
      const QVector<NodeKeyframe*> &keys = (*it)->GetKeyframes();
      for (auto jt=keys.cbegin(); jt!=keys.cend(); jt++) {
        NodeKeyframe *key = *jt;

        auto ignore = GetSnapIgnoreKeyframes();
        if (ignore && std::find(ignore->cbegin(), ignore->cend(), key) != ignore->cend()) {
          continue;
        }

        qreal key_scene_pt = TimeToScene(key->time());

        AttemptSnap(potential_snaps, screen_pt, key_scene_pt, start_times, key->time());
      }
    }
  }

  if (potential_snaps.empty()) {
    HideSnaps();
    return false;
  }

  int closest_snap = 0;
  rational closest_diff = qAbs(potential_snaps.at(0).movement - *movement);

  // Determine which snap point was the closest
  for (size_t i=1; i<potential_snaps.size(); i++) {
    rational this_diff = qAbs(potential_snaps.at(i).movement - *movement);

    if (this_diff < closest_diff) {
      closest_snap = i;
      closest_diff = this_diff;
    }
  }

  *movement = potential_snaps.at(closest_snap).movement;

  // Find all points at this movement
  std::vector<rational> snap_times;
  foreach (const SnapData& d, potential_snaps) {
    if (d.movement == *movement) {
      snap_times.push_back(d.time);
    }
  }

  ShowSnaps(snap_times);

  return true;
}

void TimeBasedWidget::ShowSnaps(const std::vector<rational> &times)
{
  foreach (TimeBasedView* view, timeline_views_) {
    view->EnableSnap(times);
  }
}

void TimeBasedWidget::HideSnaps()
{
  foreach (TimeBasedView* view, timeline_views_) {
    view->DisableSnap();
  }
}

}
