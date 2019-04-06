#include "timelinearea.h"

#include <QScrollArea>

#include "panels/timeline.h"
#include "global/config.h"

int olive::timeline::kTimelineLabelFixedWidth = 200;

TimelineArea::TimelineArea(Timeline* timeline, olive::timeline::Alignment alignment) :
  timeline_(timeline),
  track_list_(nullptr),
  alignment_(alignment)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  // LABELS
  label_container_ = new QWidget();
  label_container_layout_ = new QVBoxLayout(label_container_);
  label_container_layout_->setMargin(0);
  label_container_layout_->setSpacing(0);
  label_container_layout_->addStretch();

  // LABEL SCROLLAREA
  QScrollArea* label_area = new QScrollArea();
  label_area->setFrameShape(QFrame::NoFrame);
  label_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  label_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  label_area->verticalScrollBar()->setMaximum(INT_MAX);
  label_area->setWidgetResizable(true);
  label_area->setFixedWidth(olive::timeline::kTimelineLabelFixedWidth);
  label_area->setWidget(label_container_);
  layout->addWidget(label_area);

  // VIEW
  view_ = new TimelineView(timeline_);
  view_->SetAlignment(alignment_);
  layout->addWidget(view_);

  // SCROLLBAR
  scrollbar_ = new QScrollBar(Qt::Vertical);
  layout->addWidget(scrollbar_);

  connect(scrollbar_, SIGNAL(valueChanged(int)), view_, SLOT(setScroll(int)));
  connect(scrollbar_, SIGNAL(valueChanged(int)), label_area->verticalScrollBar(), SLOT(setValue(int)));
  connect(view_, SIGNAL(setScrollMaximum(int)), this, SLOT(setScrollMaximum(int)));
  connect(view_, SIGNAL(requestScrollChange(int)), scrollbar_, SLOT(setValue(int)));
}

void TimelineArea::SetTrackList(Sequence *sequence, Track::Type track_list)
{
  if (track_list_ != nullptr) {
    disconnect(track_list_, SIGNAL(TrackCountChanged()), this, SLOT(RefreshLabels()));
  }

  if (sequence == nullptr) {

    track_list_ = nullptr;

  } else {

    track_list_ = sequence->GetTrackList(track_list);
    connect(track_list_, SIGNAL(TrackCountChanged()), this, SLOT(RefreshLabels()));

  }

  RefreshLabels();

  view_->SetTrackList(track_list_);

}

void TimelineArea::SetAlignment(olive::timeline::Alignment alignment)
{
  view_->SetAlignment(alignment);
  alignment_ = alignment;

  RefreshLabels();
}

TrackList *TimelineArea::track_list()
{
  return track_list_;
}

TimelineView *TimelineArea::view()
{
  return view_;
}

void TimelineArea::wheelEvent(QWheelEvent *event)
{

  // TODO: implement pixel scrolling

  bool shift = (event->modifiers() & Qt::ShiftModifier);
  bool ctrl = (event->modifiers() & Qt::ControlModifier);
  bool alt = (event->modifiers() & Qt::AltModifier);

  // "Scroll Zooms" false + Control up  : not zooming
  // "Scroll Zooms" false + Control down:     zooming
  // "Scroll Zooms" true  + Control up  :     zooming
  // "Scroll Zooms" true  + Control down: not zooming
  bool zooming = (olive::config.scroll_zooms != ctrl);

  // Allow shift for axis swap, but don't swap on zoom... Unless
  // we need to override Qt's axis swap via Alt
  bool swap_hv = ((shift != olive::config.invert_timeline_scroll_axes) &
                  !zooming) | (alt & !shift & zooming);

  int delta_h = swap_hv ? event->angleDelta().y() : event->angleDelta().x();
  int delta_v = swap_hv ? event->angleDelta().x() : event->angleDelta().y();

  if (zooming) {

    // Zoom only uses vertical scrolling, to avoid glitches on touchpads.
    // Don't do anything if not scrolling vertically.

    if (delta_v != 0) {

      // delta_v == 120 for one click of a mousewheel. Less or more for a
      // touchpad gesture. Calculate speed to compensate.
      // 120 = ratio of 4/3 (1.33), -120 = ratio of 3/4 (.75)

      double zoom_ratio = 1.0 + (abs(delta_v) * 0.33 / 120);

      if (delta_v < 0) {
        zoom_ratio = 1.0 / zoom_ratio;
      }

      timeline_->multiply_zoom(zoom_ratio);
    }

  } else {

    // Use the Timeline's main scrollbar for horizontal scrolling, and this
    // widget's scrollbar for vertical scrolling.

    QScrollBar* bar_v = scrollbar_;
    QScrollBar* bar_h = timeline_->horizontalScrollBar;

    // Match the wheel events to the size of a step as per
    // https://doc.qt.io/qt-5/qwheelevent.html#angleDelta

    int step_h = bar_h->singleStep() * delta_h / -120;
    int step_v = bar_v->singleStep() * delta_v / -120;

    // Apply to appropriate scrollbars

    bar_h->setValue(bar_h->value() + step_h);
    bar_v->setValue(bar_v->value() + step_v);
  }
}

void TimelineArea::RefreshLabels()
{
  if (track_list_ == nullptr) {

    labels_.clear();

  } else {

    labels_.resize(track_list_->TrackCount());
    for (int i=0;i<labels_.size();i++) {
      labels_[i] = std::make_shared<TimelineLabel>();
      labels_[i]->SetTrack(track_list_->TrackAt(i));

      switch (alignment_) {
      case olive::timeline::kAlignmentTop:
        label_container_layout_->insertWidget(label_container_layout_->count()-1, labels_[i].get());
        break;
      case olive::timeline::kAlignmentBottom:
        label_container_layout_->insertWidget(1, labels_[i].get());
        break;
      case olive::timeline::kAlignmentSingle:
        break;
      }
    }

  }
}

void TimelineArea::resizeEvent(QResizeEvent *)
{
  scrollbar_->setPageStep(view_->height());
}

void TimelineArea::setScrollMaximum(int maximum)
{
  scrollbar_->setMaximum(qMax(0, maximum - view_->height()));
  label_container_->setMinimumHeight(maximum);
//  label_container_->setFixedHeight(maximum);
}
