#include "viewersizer.h"

ViewerSizer::ViewerSizer(QWidget *parent) :
  QWidget(parent),
  widget_(nullptr),
  aspect_ratio_(0)
{
}

void ViewerSizer::SetWidget(QWidget *widget)
{
  if (widget_ != nullptr) {
    widget_->setParent(nullptr);

    disconnect(widget_, SIGNAL(SizeChanged(int, int)), this, SLOT(SizeChangedSlot(int, int)));
  }

  widget_ = widget;

  if (widget_ != nullptr) {
    widget_->setParent(this);

    connect(widget_, SIGNAL(SizeChanged(int, int)), this, SLOT(SizeChangedSlot(int, int)));

    UpdateSize();
  }
}

void ViewerSizer::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);

  UpdateSize();
}

void ViewerSizer::UpdateSize()
{
  if (widget_ == nullptr) {
    return;
  }

  if (qIsNull(aspect_ratio_)) {
    widget_->setVisible(false);
    return;
  }

  widget_->setVisible(true);

  double our_aspect_ratio = static_cast<double>(width()) / static_cast<double>(height());

  QSize child_size = size();

  if (our_aspect_ratio > aspect_ratio_) {
    // This container is wider than the image, scale by height
    child_size.setWidth(qRound(child_size.height() * aspect_ratio_));
  } else {
    // This container is taller than the image, scale by width
    child_size.setHeight(qRound(child_size.width() / aspect_ratio_));
  }

  widget_->resize(child_size);
}

void ViewerSizer::SizeChangedSlot(int width, int height)
{
  aspect_ratio_ = static_cast<double>(width) / static_cast<double>(height);

  UpdateSize();
}
