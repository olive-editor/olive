#include "widget/timeline/sequenceview.h"

#include "project/sequence/track.h"
#include "project/sequence/sequence.h"
#include "widget/timeline/trackview.h"
#include "widget/timeline/sequencescrollview.h"
#include "platform/theme/themeservice.h"

#include <QPainter>
#include <iostream>

namespace olive {

TimelineWidgetSequenceView::TimelineWidgetSequenceView(
  QWidget* parent,
  Sequence* const sequence,
  SequenceScrollView* const scrollView,
  IThemeService* const themeService) :
  QWidget(parent),
  sequence_(sequence),
  scrollView_(scrollView),
  theme_service_(themeService) {

  auto tracks = sequence->tracks();
  for (int i = 0; i < tracks.size(); i ++) handleDidAddTrack(tracks[i], i);
  QObject::connect(sequence, &Sequence::onDidAddTrack, this, &TimelineWidgetSequenceView::handleDidAddTrack);
}

void TimelineWidgetSequenceView::handleDidAddTrack(Track* const track, int index) {
  // Create and add Track view
  std::cout << "Add Track View\n";
  auto view = new TimelineWidgetTrackView(this, track, scrollView_, theme_service_);
  track_views_.emplace(track_views_.begin() + index, view);
}

void TimelineWidgetSequenceView::handleWillRemoveTrack(Track* const track, int index) {
  // Delete Track view
  track_views_.erase(track_views_.begin() + index);
}

void TimelineWidgetSequenceView::resizeEvent(QResizeEvent* event) {
  for (int i = 0; i < track_views_.size(); i ++) {
    auto& track_view = track_views_[i];
    track_view->resize(width(), 29);
    track_view->move(0, i * 30);
  }

  QWidget::resizeEvent(event);
}

void TimelineWidgetSequenceView::paintEvent(QPaintEvent* event) {
  auto& theme = theme_service_->getTheme();
  QPainter p(this);
  p.setPen(theme.surfaceBrightColor());
  // Draw track border
  p.drawLine(0, 0, width(), 0);
  for (int i = 0; i < track_views_.size(); i ++) {
    auto& track_view = track_views_[i];
    p.drawLine(0, (i + 1) * 30 - 1, width(), (i + 1) * 30 - 1);
  }
  QWidget::paintEvent(event);
}


}