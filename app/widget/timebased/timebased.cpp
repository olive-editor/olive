#include "timebased.h"

#include "common/timecodefunctions.h"

TimeBasedWidget::TimeBasedWidget(bool ruler_text_visible, bool ruler_cache_status_visible, QWidget *parent) :
  QWidget(parent),
  viewer_node_(nullptr)
{
  ruler_ = new TimeRuler(ruler_text_visible, ruler_cache_status_visible, this);
  connect(ruler_, &TimeRuler::TimeChanged, this, &TimeBasedWidget::SetTimeAndSignal);

  scrollbar_ = new QScrollBar(Qt::Horizontal, this);
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
  if (viewer_node_) {
    DisconnectNodeInternal(viewer_node_);
  }

  viewer_node_ = node;

  ConnectedNodeChanged(viewer_node_);

  if (viewer_node_) {
    ConnectNodeInternal(viewer_node_);
  }
}

TimeRuler *TimeBasedWidget::ruler() const
{
  return ruler_;
}

QScrollBar *TimeBasedWidget::scrollbar() const
{
  return scrollbar_;
}

void TimeBasedWidget::TimebaseChangedEvent(const rational &timebase)
{
  TimelineScaledObject::TimebaseChangedEvent(timebase);

  ruler_->SetTimebase(timebase);

  emit TimebaseChanged(timebase);
}

void TimeBasedWidget::ScaleChangedEvent(const double &scale)
{
  TimelineScaledObject::ScaleChangedEvent(scale);

  ruler_->SetScale(scale);
}

void TimeBasedWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  // Update horizontal scrollbar's page step to the width of the panel
  scrollbar()->setPageStep(scrollbar()->width());
}

void TimeBasedWidget::SetTime(int64_t timestamp)
{
  ruler_->SetTime(timestamp);

  TimeChangedEvent(timestamp);
}

void TimeBasedWidget::SetTimebase(const rational &timebase)
{
  TimelineScaledObject::SetTimebase(timebase);
}

void TimeBasedWidget::SetScale(const double &scale)
{
  TimelineScaledObject::SetScale(scale);
}

void TimeBasedWidget::ZoomIn()
{
  SetScale(GetScale() * 2);
}

void TimeBasedWidget::ZoomOut()
{
  SetScale(GetScale() * 0.5);
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

  foreach (TrackOutput* track, viewer_node_->Tracks()) {
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

  foreach (TrackOutput* track, GetConnectedNode()->Tracks()) {
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
    SetTimeAndSignal(Timecode::time_to_timestamp(viewer_node_->Length(), timebase()));
  }
}

void TimeBasedWidget::SetTimeAndSignal(const int64_t &t)
{
  SetTime(t);
  emit TimeChanged(t);
}
