#include "timelinewidget.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QtMath>

#include "common/timecodefunctions.h"

TimelineWidget::TimelineWidget(QWidget *parent) :
  QWidget(parent),
  timeline_node_(nullptr),
  playhead_(0)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  ruler_ = new TimeRuler(true);
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), this, SLOT(UpdateInternalTime(const int64_t&)));
  layout->addWidget(ruler_);

  // Create list of TimelineViews - these MUST correspond to the ViewType enum

  QSplitter* view_splitter = new QSplitter(Qt::Vertical);
  view_splitter->setChildrenCollapsible(false);
  layout->addWidget(view_splitter);

  // Video view
  views_.append(new TimelineView(Qt::AlignBottom));

  // Audio view
  views_.append(new TimelineView(Qt::AlignTop));

  // Set both views as siblings
  TimelineView::SetSiblings(views_);

  // Global scrollbar
  horizontal_scroll_ = new QScrollBar(Qt::Horizontal);
  connect(horizontal_scroll_, SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  connect(views_.first()->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), horizontal_scroll_, SLOT(setRange(int, int)));
  layout->addWidget(horizontal_scroll_);

  foreach (TimelineView* view, views_) {
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    view_splitter->addWidget(view);

    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
    connect(view, SIGNAL(ScaleChanged(double)), this, SLOT(SetScale(double)));
    connect(view, SIGNAL(TimebaseChanged(const rational&)), this, SLOT(SetTimebase(const rational&)));
    connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), view, SLOT(SetTime(const int64_t&)));
    connect(view, SIGNAL(TimeChanged(const int64_t&)), ruler_, SLOT(SetTime(const int64_t&)));
    connect(view, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));
    connect(horizontal_scroll_, SIGNAL(valueChanged(int)), view->horizontalScrollBar(), SLOT(setValue(int)));
    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), horizontal_scroll_, SLOT(setValue(int)));

    // Connect each view's scroll to each other
    foreach (TimelineView* other_view, views_) {
      if (view != other_view) {
        connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)), other_view->horizontalScrollBar(), SLOT(setValue(int)));
      }
    }
  }

  // Split viewer 50/50
  view_splitter->setSizes({INT_MAX, INT_MAX});

  // FIXME: Magic number
  SetScale(90.0);
}

void TimelineWidget::Clear()
{
  SetTimebase(0);

  foreach (TimelineView* view, views_) {
    view->Clear();
  }
}

void TimelineWidget::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  ruler_->SetTimebase(timebase);

  foreach (TimelineView* view, views_) {
    view->SetTimebase(timebase);
  }
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  horizontal_scroll_->setPageStep(horizontal_scroll_->width());
}

void TimelineWidget::SetTime(const int64_t &timestamp)
{
  ruler_->SetTime(timestamp);

  foreach (TimelineView* view, views_) {
    view->SetTime(timestamp);
  }

  UpdateInternalTime(timestamp);
}

void TimelineWidget::ConnectTimelineNode(TimelineOutput *node)
{
  if (timeline_node_ != nullptr) {
    disconnect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateTimelineLength(const rational&)));
  }

  timeline_node_ = node;

  int track_type = 0;

  foreach (TimelineView* view, views_) {
    view->ConnectTimelineNode(node->track_list(static_cast<TimelineOutput::TrackType>(track_type)));

    track_type++;
  }

  if (timeline_node_ != nullptr) {
    connect(timeline_node_, SIGNAL(LengthChanged(const rational&)), this, SLOT(UpdateTimelineLength(const rational&)));

    foreach (TimelineView* view, views_) {
      view->SetEndTime(timeline_node_->timeline_length());
    }
  }
}

void TimelineWidget::DisconnectTimelineNode()
{
  timeline_node_ = nullptr;

  foreach (TimelineView* view, views_) {
    view->DisconnectTimelineNode();
  }
}

void TimelineWidget::ZoomIn()
{
  SetScale(scale_ * 2);
}

void TimelineWidget::ZoomOut()
{
  SetScale(scale_ * 0.5);
}

void TimelineWidget::SelectAll()
{
  foreach (TimelineView* view, views_) {
    view->SelectAll();
  }
}

void TimelineWidget::DeselectAll()
{
  foreach (TimelineView* view, views_) {
    view->DeselectAll();
  }
}

void TimelineWidget::RippleToIn()
{
  RippleEditTo(olive::timeline::kTrimIn, false);
}

void TimelineWidget::RippleToOut()
{
  RippleEditTo(olive::timeline::kTrimOut, false);
}

void TimelineWidget::EditToIn()
{
  RippleEditTo(olive::timeline::kTrimIn, true);
}

void TimelineWidget::EditToOut()
{
  RippleEditTo(olive::timeline::kTrimOut, true);
}

void TimelineWidget::GoToPrevCut()
{
  if (timeline_node_ == nullptr) {
    return;
  }

  if (playhead_ == 0) {
    return;
  }

  int64_t closest_cut = 0;

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    int64_t this_track_closest_cut = 0;

    foreach (Block* block, track->Blocks()) {
      int64_t block_out_ts = olive::time_to_timestamp(block->out(), timebase_);

      if (block_out_ts < playhead_) {
        this_track_closest_cut = block_out_ts;
      } else {
        break;
      }
    }

    closest_cut = qMax(closest_cut, this_track_closest_cut);
  }

  SetTimeAndSignal(closest_cut);
}

void TimelineWidget::GoToNextCut()
{
  if (timeline_node_ == nullptr) {
    return;
  }

  int64_t closest_cut = INT64_MAX;

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    int64_t this_track_closest_cut = olive::time_to_timestamp(track->in(), timebase_);

    if (this_track_closest_cut <= playhead_) {
      this_track_closest_cut = INT64_MAX;
    }

    foreach (Block* block, track->Blocks()) {
      int64_t block_in_ts = olive::time_to_timestamp(block->in(), timebase_);

      if (block_in_ts > playhead_) {
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

void TimelineWidget::RippleEditTo(olive::timeline::MovementMode mode, bool insert_gaps)
{
  rational playhead_time = olive::timestamp_to_time(playhead_, timebase_);

  rational closest_point_to_playhead;
  if (mode == olive::timeline::kTrimIn) {
    closest_point_to_playhead = 0;
  } else {
    closest_point_to_playhead = RATIONAL_MAX;
  }

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    Block* b = track->NearestBlockBefore(playhead_time);

    if (b != nullptr) {
      if (mode == olive::timeline::kTrimIn) {
        closest_point_to_playhead = qMax(b->in(), closest_point_to_playhead);
      } else {
        closest_point_to_playhead = qMin(b->out(), closest_point_to_playhead);
      }
    }
  }

  QUndoCommand* command = new QUndoCommand();

  if (closest_point_to_playhead == playhead_time) {
    // Remove one frame only
    if (mode == olive::timeline::kTrimIn) {
      playhead_time += timebase_;
    } else {
      playhead_time -= timebase_;
    }
  }

  rational in_ripple = qMin(closest_point_to_playhead, playhead_time);
  rational out_ripple = qMax(closest_point_to_playhead, playhead_time);
  rational ripple_length = out_ripple - in_ripple;

  foreach (TrackOutput* track, timeline_node_->Tracks()) {
    TrackRippleRemoveAreaCommand* ripple_command = new TrackRippleRemoveAreaCommand(track,
                                                                                    in_ripple,
                                                                                    out_ripple,
                                                                                    command);

    if (insert_gaps) {
      GapBlock* gap = new GapBlock();
      gap->set_length(ripple_length);
      ripple_command->SetInsert(gap);
    }
  }

  olive::undo_stack.pushIfHasChildren(command);

  if (mode == olive::timeline::kTrimIn && !insert_gaps) {
    int64_t new_time = olive::time_to_timestamp(closest_point_to_playhead, timebase_);

    SetTimeAndSignal(new_time);
  }
}

void TimelineWidget::SetTimeAndSignal(const int64_t &t)
{
  SetTime(t);
  emit TimeChanged(t);
}

void TimelineWidget::SetScale(double scale)
{
  scale_ = scale;

  ruler_->SetScale(scale_);

  foreach (TimelineView* view, views_) {
    view->SetScale(scale_);
  }
}

void TimelineWidget::UpdateInternalTime(const int64_t &timestamp)
{
  playhead_ = timestamp;
}

void TimelineWidget::UpdateTimelineLength(const rational &length)
{
  foreach (TimelineView* view, views_) {
    view->SetEndTime(length);
  }
}
