#include "viewersizer.h"

ViewerSizer::ViewerSizer(QWidget *parent) :
  QWidget(parent),
  widget_(nullptr),
  aspect_ratio_(0)
{
}

void ViewerSizer::SetWidget(QWidget *widget)
{
  // Delete any previous widgets occupying this space
  delete widget_;

  widget_ = widget;

  if (widget_ != nullptr) {
    widget_->setParent(this);

    UpdateSize();
  }
}

void ViewerSizer::SetChildSize(int width, int height)
{
  if (height == 0) {
    aspect_ratio_ = 0;
  } else {
    aspect_ratio_ = static_cast<double>(width) / static_cast<double>(height);
  }

  UpdateSize();
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

  // If the aspect ratio is 0, the widget is always hidden
  if (qIsNull(aspect_ratio_)) {
    widget_->setVisible(false);
    return;
  }

  widget_->setVisible(true);

  double our_aspect_ratio = static_cast<double>(width()) / static_cast<double>(height());

  QPoint child_pos;
  QSize child_size = size();

  if (our_aspect_ratio > aspect_ratio_) {
    // This container is wider than the image, scale by height
    child_size.setWidth(qRound(child_size.height() * aspect_ratio_));
    child_pos.setX(width() / 2 - child_size.width() / 2);
  } else {
    // This container is taller than the image, scale by width
    child_size.setHeight(qRound(child_size.width() / aspect_ratio_));
    child_pos.setY(height() / 2 - child_size.height() / 2);
  }

  widget_->resize(child_size);
  widget_->move(child_pos);
}
