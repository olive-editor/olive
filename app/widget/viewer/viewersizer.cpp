/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "viewersizer.h"

#include <QMatrix4x4>

OLIVE_NAMESPACE_ENTER

ViewerSizer::ViewerSizer(QWidget *parent) :
  QWidget(parent),
  widget_(nullptr),
  aspect_ratio_(0),
  zoom_(0)
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
  width_ = width;
  height_ = height;

  if (!width_ || !height_) {
    aspect_ratio_ = 0;
  } else {
    aspect_ratio_ = static_cast<double>(width_) / static_cast<double>(height_);
  }

  UpdateSize();
}

void ViewerSizer::SetZoom(int percent)
{
  zoom_ = percent;

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

  QSize child_size;
  QMatrix4x4 child_matrix;

  if (zoom_ <= 0) {

    // If zoom is 0, we auto-fit
    double our_aspect_ratio = static_cast<double>(width()) / static_cast<double>(height());

    child_size = size();

    if (our_aspect_ratio > aspect_ratio_) {
      // This container is wider than the image, scale by height
      child_size = QSize(qRound(child_size.height() * aspect_ratio_), height());
    } else {
      // This container is taller than the image, scale by width
      child_size = QSize(width(), qRound(child_size.width() / aspect_ratio_));
    }

  } else {

    float x_scale = 1.0f;
    float y_scale = 1.0f;

    int zoomed_width = qRound(width_ * static_cast<double>(zoom_) * 0.01);
    int zoomed_height = qRound(height_ * static_cast<double>(zoom_) * 0.01);

    if (zoomed_width > width()) {
      x_scale = static_cast<double>(zoomed_width) / static_cast<double>(width());
      zoomed_width = width();
    }

    if (zoomed_height > height()) {
      y_scale = static_cast<double>(zoomed_height) / static_cast<double>(height());
      zoomed_height = height();
    }

    // Rather than make a huge surface, we still crop at our width/height and then signal a matrix
    child_matrix.scale(x_scale, y_scale, 1.0F);

    child_size = QSize(zoomed_width, zoomed_height);

  }

  widget_->resize(child_size);
  widget_->move(width() / 2 - child_size.width() / 2, height() / 2 - child_size.height() / 2);

  emit RequestMatrix(child_matrix);
}

OLIVE_NAMESPACE_EXIT
