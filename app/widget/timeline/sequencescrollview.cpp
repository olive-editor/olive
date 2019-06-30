#include "widget/timeline/sequencescrollview.h"

#include "project/sequence/sequence.h"

namespace olive {

SequenceScrollView::SequenceScrollView(
  QWidget* const parent,
  Sequence* const sequence) :
  QWidget(parent),
  sequence_(sequence) {

  update();
}

void SequenceScrollView::update() {
  // Todo : implement
  startTime_ = 0;
  endTime_ = sequence_->duration();
  pxPerFrame_ = 1.0f;
  emit onDidUpdate();
}

int SequenceScrollView::getTimeRelativeToView(int pixel) const {
  return roundf(startTime_ + pixel / pxPerFrame_);
}

int SequenceScrollView::getTimeAmountRelativeToView(int pixel) const {
  return roundf(pixel / pxPerFrame_);
}

int SequenceScrollView::getPositionRelativeToView(int time) const {
  return floorf((time - startTime_) * pxPerFrame_);
}

int SequenceScrollView::getPixelAmountRelativeToView(int time) const {
  return roundf(time * pxPerFrame_);
}

}