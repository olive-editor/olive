#include "widget/timeline/timelinewidget.h"

#include <QStyle>
#include <QPainter>
#include <QStyleOption>

#include "project/sequence/sequence.h"
#include "widget/timeline/sequencescrollview.h"
#include "widget/timeline/sequenceview.h"
#include "platform/theme/themeservice.h"
#include "widget/panel/panel.h"

#include <iostream>

namespace olive {

TimelineWidget::TimelineWidget(
  QWidget* parent,
  IThemeService* const theme_service) :
  QWidget(parent),
  sequence_(nullptr),
  sequence_view_(nullptr),
  sequence_scroll_view_(nullptr),
  theme_service_(theme_service) {

}

void TimelineWidget::resizeEvent(QResizeEvent* event) {
  if (sequence_scroll_view_ != nullptr) {
    sequence_scroll_view_->resize(width(), height());
    sequence_view_->resize(width(), height());
  }

  QWidget::resizeEvent(event);
}

void TimelineWidget::paintEvent(QPaintEvent* event) {
  QStyleOption opt;
  opt.init(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

  QWidget::paintEvent(event);
}

void TimelineWidget::setSequence(Sequence* sequence) {
  if (sequence_view_ != nullptr) {
    sequence_ = nullptr;
    delete sequence_view_;
    delete sequence_scroll_view_;
  }
  if (sequence == nullptr) return;
  sequence_ = sequence_;
  sequence_scroll_view_ = new SequenceScrollView(this, sequence);
  sequence_view_ = new TimelineWidgetSequenceView(sequence_scroll_view_, sequence, sequence_scroll_view_, theme_service_);
}

}