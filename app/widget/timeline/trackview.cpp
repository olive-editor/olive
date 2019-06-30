#include "widget/timeline/trackview.h"

#include <QPainter>

#include "project/sequence/track.h"
#include "project/sequence/clip.h"
#include "platform/theme/themeservice.h"
#include "widget/timeline/sequencescrollview.h"

#include <iostream>

namespace olive {

TimelineWidgetTrackView::TimelineWidgetTrackView(
  QWidget* const parent,
  Track* const track,
  SequenceScrollView* const scrollView,
  IThemeService* const theme_service) : 
  QWidget(parent), track_(track), scrollView_(scrollView), theme_service_(theme_service) {

  auto& clips = track->clips();
  for (auto& clip : clips) handleDidAddClip(clip);
  QObject::connect(track, &Track::onDidAddClip, this, &TimelineWidgetTrackView::handleDidAddClip);
  QObject::connect(track, &Track::onWillRemoveClip, this, &TimelineWidgetTrackView::handleWillRemoveClip);

}

void TimelineWidgetTrackView::handleDidAddClip(Clip* const clip) {
  // Create and add Clip view
  auto view = new TimelineWidgetClipView(this, clip, scrollView_, theme_service_);
  clipViews_.emplace(clip->id(), view);
}

void TimelineWidgetTrackView::handleWillRemoveClip(Clip* const clip) {
  // Delete Track view
  clipViews_.erase(clip->id());
}

void TimelineWidgetTrackView::paintEvent(QPaintEvent* event) {
  auto& theme = theme_service_->getTheme();
  QPainter p(this);
  // p.fillRect(0, 0, width(), height(), theme.primaryColor());
  
  QWidget::paintEvent(event);
}

}