#include "widget/timeline/clipview.h"

#include <QPainter>

#include "project/sequence/clip.h"
#include "widget/timeline/sequencescrollview.h"
#include "platform/theme/themeservice.h"

#include <iostream>

namespace olive {

TimelineWidgetClipView::TimelineWidgetClipView(
  QWidget* const parent,
  Clip* const clip,
  SequenceScrollView* const scrollView,
  IThemeService* const theme_service) : 
  QWidget(parent), clip_(clip), focused_(false), scrollView_(scrollView), theme_service_(theme_service) {

  updateView();
  QObject::connect(clip, &Clip::onDidChangeTime, this, [this](int oldStart, int oldEnd) {
    updateView();
  });
  QObject::connect(scrollView, &SequenceScrollView::onDidUpdate, this, [this]() {
    updateView();
  });
}

void TimelineWidgetClipView::mousePressEvent(QMouseEvent* event) {
  focus();
}

void TimelineWidgetClipView::focus() {
  focused_ = true;
  updateView();
  emit onDidFocus();
}

void TimelineWidgetClipView::blur() {
  focused_ = false;
  updateView();
  emit onDidBlur();
}

bool TimelineWidgetClipView::focused() const {
  return focused_;
}

void TimelineWidgetClipView::updateView() {
  int left = scrollView_->getPositionRelativeToView(clip_->in());
  int right = scrollView_->getPositionRelativeToView(clip_->out());
  move(left, 0);
  resize(right - left, height());
  repaint();
}

void TimelineWidgetClipView::resizeEvent(QResizeEvent* event) {
  updateView();

  QWidget::resizeEvent(event);
}

void TimelineWidgetClipView::paintEvent(QPaintEvent* event) {
  auto& theme = theme_service_->getTheme();
  QPainter p(this);
  p.fillRect(0, 0, width(), height(),
    focused_ ? theme.secondaryColor() : theme.primaryColor());
  
  QWidget::paintEvent(event);
}

}