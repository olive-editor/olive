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

#include "timebased.h"

#include <QInputDialog>
#include <QUndoCommand>

#include "common/autoscroll.h"
#include "common/timecodefunctions.h"
#include "config/config.h"
#include "core.h"
#include "project/item/sequence/sequence.h"
#include "widget/timelinewidget/undo/undo.h"

namespace olive {

TimeBasedWidget::TimeBasedWidget(bool ruler_text_visible, bool ruler_cache_status_visible, QWidget *parent) :
  TimelineScaledWidget(parent),
  viewer_node_(nullptr),
  auto_max_scrollbar_(false),
  points_(nullptr),
  toggle_show_all_(false),
  auto_set_timebase_(true)
{
  ruler_ = new TimeRuler(ruler_text_visible, ruler_cache_status_visible, this);
  connect(ruler_, &TimeRuler::TimeChanged, this, &TimeBasedWidget::SetTimeAndSignal);

  scrollbar_ = new ResizableScrollBar(Qt::Horizontal, this);
  connect(scrollbar_, &ResizableScrollBar::RequestScale, this, &TimeBasedWidget::ScrollBarResized);
}

void TimeBasedWidget::SetScaleAndCenterOnPlayhead(const double &scale)
{
  SetScale(scale);

  // Zoom towards the playhead
  // (using a hacky singleShot so the scroll occurs after the scene and its scrollbars have updated)
  QTimer::singleShot(0, this, &TimeBasedWidget::CenterScrollOnPlayhead);
}

rational TimeBasedWidget::GetTime() const
{
  return Timecode::timestamp_to_time(ruler()->GetTime(), timebase());
}

const int64_t &TimeBasedWidget::GetTimestamp() const
{
  return ruler_->GetTime();
}

ViewerOutput *TimeBasedWidget::GetConnectedNode() const
{
  return viewer_node_;
}

void TimeBasedWidget::ConnectViewerNode(ViewerOutput *node)
{
  if (viewer_node_ == node) {
    return;
  }

  if (viewer_node_) {
    DisconnectNodeInternal(viewer_node_);

    disconnect(viewer_node_, &ViewerOutput::LengthChanged, this, &TimeBasedWidget::UpdateMaximumScroll);
    disconnect(viewer_node_, &ViewerOutput::TimebaseChanged, this, &TimeBasedWidget::SetTimebase);

    if (auto_set_timebase_) {
      SetTimebase(rational());
    }

    points_ = nullptr;
    ruler()->ConnectTimelinePoints(nullptr);
  }

  viewer_node_ = node;

  ConnectedNodeChanged(viewer_node_);

  if (viewer_node_) {
    connect(viewer_node_, &ViewerOutput::LengthChanged, this, &TimeBasedWidget::UpdateMaximumScroll);

    if ((points_ = ConnectTimelinePoints())) {
      ruler()->ConnectTimelinePoints(points_);
    }

    if (auto_set_timebase_) {
      if (!viewer_node_->video_params().time_base().isNull()) {
        SetTimebase(viewer_node_->video_params().time_base());
      } else if (viewer_node_->audio_params().sample_rate() > 0) {
        SetTimebase(viewer_node_->audio_params().time_base());
      } else {
        SetTimebase(rational());
      }

      connect(viewer_node_, &ViewerOutput::TimebaseChanged, this, &TimeBasedWidget::SetTimebase);
    }

    ConnectNodeInternal(viewer_node_);
  }

  UpdateMaximumScroll();
}

void TimeBasedWidget::UpdateMaximumScroll()
{
  rational length = (viewer_node_) ? viewer_node_->GetLength() : rational();

  if (auto_max_scrollbar_) {
    scrollbar_->setMaximum(qMax(0, qCeil(TimeToScene(length)) - width()));
  }

  foreach (TimelineViewBase* base, timeline_views_) {
    base->SetEndTime(length);
  }
}

void TimeBasedWidget::ScrollBarResized(const double &multiplier)
{
  QScrollBar* bar = static_cast<QScrollBar*>(sender());

  // Our extension area (represented by a TimelineViewEndItem) is NOT scaled, but the ResizableScrollBar doesn't know
  // this. Here we re-calculate the requested scale knowing that the end item is not affected by scale.

  int current_max = bar->maximum();
  double proposed_max = static_cast<double>(current_max) * multiplier;

  proposed_max = proposed_max - (bar->width() * 0.5 / multiplier) + (bar->width() * 0.5);

  double corrected_scale;

  if (current_max == 0) {
    corrected_scale = multiplier;
  } else {
    corrected_scale = (proposed_max / static_cast<double>(current_max));
  }

  SetScale(GetScale() * corrected_scale);
}

void TimeBasedWidget::PageScrollToPlayhead()
{
  int playhead_pos = qRound(TimeToScene(GetTime()));

  int viewport_width = ruler()->width();
  int viewport_padding = viewport_width / 16;

  if (playhead_pos < scrollbar()->value()) {
    // Anchor the playhead to the RIGHT of where we scroll to
    scrollbar()->setValue(playhead_pos - viewport_width + viewport_padding);
  } else if (playhead_pos > scrollbar()->value() + viewport_width) {
    // Anchor the playhead to the LEFT of where we scroll to
    scrollbar()->setValue(playhead_pos - viewport_padding);
  }
}

TimeRuler *TimeBasedWidget::ruler() const
{
  return ruler_;
}

ResizableScrollBar *TimeBasedWidget::scrollbar() const
{
  return scrollbar_;
}

void TimeBasedWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimelineScaledWidget::TimebaseChangedEvent(timebase);

  ruler_->SetTimebase(timebase);

  emit TimebaseChanged(timebase);
}

void TimeBasedWidget::ScaleChangedEvent(const double &scale)
{
  TimelineScaledWidget::ScaleChangedEvent(scale);

  ruler_->SetScale(scale);

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

TimelinePoints *TimeBasedWidget::ConnectTimelinePoints()
{
  return static_cast<Sequence*>(viewer_node_->parent());
}

Project *TimeBasedWidget::GetTimelinePointsProject()
{
  return static_cast<Sequence*>(viewer_node_->parent())->project();
}

TimelinePoints *TimeBasedWidget::GetConnectedTimelinePoints() const
{
  return points_;
}

void TimeBasedWidget::ConnectTimelineView(TimelineViewBase *base)
{
  timeline_views_.append(base);
}

void TimeBasedWidget::SetTimestamp(int64_t timestamp)
{
  ruler_->SetTime(timestamp);

  switch (static_cast<AutoScroll::Method>(Config::Current()["Autoscroll"].toInt())) {
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

  TimeChangedEvent(timestamp);
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
  if (!GetConnectedNode()) {
    return;
  }

  if (GetTimestamp() == 0) {
    return;
  }

  int64_t closest_cut = 0;

  foreach (TrackOutput* track, viewer_node_->GetTracks()) {
    int64_t this_track_closest_cut = 0;

    foreach (Block* block, track->Blocks()) {
      int64_t block_out_ts = Timecode::time_to_timestamp(block->out(), timebase());

      if (block_out_ts < GetTimestamp()) {
        this_track_closest_cut = block_out_ts;
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
  if (!GetConnectedNode()) {
    return;
  }

  int64_t closest_cut = INT64_MAX;

  foreach (TrackOutput* track, GetConnectedNode()->GetTracks()) {
    int64_t this_track_closest_cut = Timecode::time_to_timestamp(track->track_length(), timebase());

    if (this_track_closest_cut <= GetTimestamp()) {
      this_track_closest_cut = INT64_MAX;
    }

    foreach (Block* block, track->Blocks()) {
      int64_t block_in_ts = Timecode::time_to_timestamp(block->in(), timebase());

      if (block_in_ts > GetTimestamp()) {
        this_track_closest_cut = block_in_ts;
        break;
      }
    }

    closest_cut = qMin(closest_cut, this_track_closest_cut);
  }

  if (closest_cut < INT64_MAX) {
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
    SetTimeAndSignal(qMax(static_cast<int64_t>(0), ruler()->GetTime() - 1));
  }
}

void TimeBasedWidget::NextFrame()
{
  if (viewer_node_) {
    SetTimeAndSignal(ruler()->GetTime() + 1);
  }
}

void TimeBasedWidget::GoToEnd()
{
  if (viewer_node_) {
    SetTimeAndSignal(Timecode::time_to_timestamp(viewer_node_->GetLength(), timebase()));
  }
}

void TimeBasedWidget::SetTimeAndSignal(const int64_t &t)
{
  SetTimestamp(t);
  emit TimeChanged(t);
}

void TimeBasedWidget::CenterScrollOnPlayhead()
{
  scrollbar_->setValue(qRound(TimeToScene(Timecode::timestamp_to_time(ruler_->GetTime(), timebase()))) - scrollbar_->width()/2);
}

void TimeBasedWidget::SetAutoSetTimebase(bool e)
{
  auto_set_timebase_ = e;
}

void TimeBasedWidget::SetPoint(Timeline::MovementMode m, const rational& time)
{
  if (!points_) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  // Enable workarea if it isn't already enabled
  if (!points_->workarea()->enabled()) {
    new WorkareaSetEnabledCommand(GetTimelinePointsProject(), points_, true, command);
  }

  // Determine our new range
  rational in_point, out_point;

  if (m == Timeline::kTrimIn) {
    in_point = time;

    if (!points_->workarea()->enabled() || points_->workarea()->out() < in_point) {
      out_point = TimelineWorkArea::kResetOut;
    } else {
      out_point = points_->workarea()->out();
    }
  } else {
    out_point = time;

    if (!points_->workarea()->enabled() || points_->workarea()->in() > out_point) {
      in_point = TimelineWorkArea::kResetIn;
    } else {
      in_point = points_->workarea()->in();
    }
  }

  // Set workarea
  new WorkareaSetRangeCommand(GetTimelinePointsProject(), points_, TimeRange(in_point, out_point), command);

  Core::instance()->undo_stack()->push(command);
}

void TimeBasedWidget::ResetPoint(Timeline::MovementMode m)
{
  if (!points_ || !points_->workarea()->enabled()) {
    return;
  }

  TimeRange r = points_->workarea()->range();

  if (m == Timeline::kTrimIn) {
    r.set_in(TimelineWorkArea::kResetIn);
  } else {
    r.set_out(TimelineWorkArea::kResetOut);
  }

  Core::instance()->undo_stack()->push(new WorkareaSetRangeCommand(GetTimelinePointsProject(), points_, r));
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
  if (!points_) {
    return;
  }


  Core::instance()->undo_stack()->push(new WorkareaSetEnabledCommand(GetTimelinePointsProject(), points_, false));
}

void TimeBasedWidget::SetMarker()
{
  if (!points_) {
    return;
  }

  bool ok;
  QString marker_name;

  if (Config::Current()["SetNameWithMarker"].toBool()) {
    marker_name = QInputDialog::getText(this, tr("Set Marker"), tr("Marker name:"), QLineEdit::Normal, QString(), &ok);
  } else {
    ok = true;
  }

  if (ok) {
    Core::instance()->undo_stack()->push(new MarkerAddCommand(static_cast<Sequence*>(GetConnectedNode()->parent())->project(),
                                                              points_->markers(), TimeRange(GetTime(), GetTime()), marker_name));
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
  if (viewer_node_) {
    if (points_ && points_->workarea()->enabled()) {
      SetTimeAndSignal(Timecode::time_to_timestamp(points_->workarea()->in(), timebase()));
    } else {
      GoToStart();
    }
  }
}

void TimeBasedWidget::GoToOut()
{
  if (viewer_node_) {
    if (points_ && points_->workarea()->enabled()) {
      SetTimeAndSignal(Timecode::time_to_timestamp(points_->workarea()->out(), timebase()));
    } else {
      GoToEnd();
    }
  }
}

TimeBasedWidget::MarkerAddCommand::MarkerAddCommand(Project *project, TimelineMarkerList *marker_list, const TimeRange &range, const QString &name) :
  project_(project),
  marker_list_(marker_list),
  range_(range),
  name_(name)
{
}

Project *TimeBasedWidget::MarkerAddCommand::GetRelevantProject() const
{
  return project_;
}

void TimeBasedWidget::MarkerAddCommand::redo_internal()
{
  added_marker_ = marker_list_->AddMarker(range_, name_);
}

void TimeBasedWidget::MarkerAddCommand::undo_internal()
{
  marker_list_->RemoveMarker(added_marker_);
}

}
