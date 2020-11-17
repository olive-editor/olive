/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
  width_(0),
  height_(0),
  pixel_aspect_(1),
  zoom_(0)
{
  horiz_scrollbar_ = new QScrollBar(Qt::Horizontal, this);
  horiz_scrollbar_->setVisible(false);
  connect(horiz_scrollbar_, &QScrollBar::valueChanged, this, &ViewerSizer::ScrollBarMoved);

  vert_scrollbar_ = new QScrollBar(Qt::Vertical, this);
  vert_scrollbar_->setVisible(false);
  connect(vert_scrollbar_, &QScrollBar::valueChanged, this, &ViewerSizer::ScrollBarMoved);
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

  UpdateSize();
}

void ViewerSizer::SetPixelAspectRatio(const rational &pixel_aspect)
{
  pixel_aspect_ = pixel_aspect;

  UpdateSize();
}

void ViewerSizer::SetZoom(int percent)
{
  zoom_ = percent;

  UpdateSize();
}

void ViewerSizer::HandDragMove(int x, int y)
{
  if (horiz_scrollbar_->isVisible()) {
    horiz_scrollbar_->setValue(horiz_scrollbar_->value() - x);
  }

  if (vert_scrollbar_->isVisible()) {
    vert_scrollbar_->setValue(vert_scrollbar_->value() - y);
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

  // If the aspect ratio is 0, the widget is always hidden
  if (!width_ || !height_) {
    widget_->setVisible(false);
    return;
  }

  widget_->setVisible(true);

  QSize child_size;
  QMatrix4x4 child_matrix;

  int available_width = width();
  int available_height = height();
  double sequence_aspect_ratio = static_cast<double>(width_) / static_cast<double>(height_)
      * pixel_aspect_.toDouble();

  if (zoom_ <= 0) {

    // If zoom is zero or negative, we auto-fit
    double our_aspect_ratio = static_cast<double>(available_width) / static_cast<double>(available_height);

    child_size = size();

    if (our_aspect_ratio > sequence_aspect_ratio) {
      // This container is wider than the image, scale by height
      child_size = QSize(qRound(child_size.height() * sequence_aspect_ratio), available_height);
    } else {
      // This container is taller than the image, scale by width
      child_size = QSize(available_width, qRound(child_size.width() / sequence_aspect_ratio));
    }

    // No scrollbars necessary for auto-fit
    horiz_scrollbar_->setVisible(false);
    vert_scrollbar_->setVisible(false);

  } else {

    float x_scale = 1.0f;
    float y_scale = 1.0f;

    int zoomed_width = GetZoomedValue(width_);
    int zoomed_height = GetZoomedValue(height_);

    bool child_exceeds_parent_width = (zoomed_width > available_width);
    bool child_exceeds_parent_height = (zoomed_height > available_height);

    if (child_exceeds_parent_width != child_exceeds_parent_height) {
      // One scrollbar definitely needs to be shown, so it's a matter of determining if the other
      // does too since adding one scrollbar necessary limits the total area

      if (child_exceeds_parent_height) {
        // A vertical scrollbar will need to be shown, which limits the width
        child_exceeds_parent_width = (zoomed_width > available_width - vert_scrollbar_->sizeHint().width());
      } else {
        // A horizontal scrollbar will need to be shown, which limits the height
        child_exceeds_parent_height = (zoomed_height > available_height - horiz_scrollbar_->sizeHint().height());
      }
    }

    horiz_scrollbar_->setVisible(child_exceeds_parent_width);
    vert_scrollbar_->setVisible(child_exceeds_parent_height);

    if (vert_scrollbar_->isVisible()) {
      // Limit available width for further calculations
      available_width -= vert_scrollbar_->sizeHint().width();
    }

    if (horiz_scrollbar_->isVisible()) {
      // Limit available height for further calculations
      available_height -= horiz_scrollbar_->sizeHint().height();
    }

    if (horiz_scrollbar_->isVisible()) {
      x_scale = static_cast<double>(zoomed_width) / static_cast<double>(available_width);

      // Update scrollbar sizes
      int horiz_width = this->width();

      if (vert_scrollbar_->isVisible()) {
        horiz_width -= vert_scrollbar_->sizeHint().width();
      }

      horiz_scrollbar_->resize(horiz_width, horiz_scrollbar_->sizeHint().height());
      horiz_scrollbar_->move(0, this->height() - horiz_scrollbar_->height() - 1);
      horiz_scrollbar_->setMaximum(zoomed_width - available_width);
      horiz_scrollbar_->setPageStep(available_width);

      zoomed_width = available_width;
    }

    if (vert_scrollbar_->isVisible()) {
      y_scale = static_cast<double>(zoomed_height) / static_cast<double>(available_height);

      // Update scrollbar sizes
      int vert_height = this->height();

      if (horiz_scrollbar_->isVisible()) {
        vert_height -= horiz_scrollbar_->sizeHint().height();
      }

      vert_scrollbar_->resize(vert_scrollbar_->sizeHint().width(), vert_height);
      vert_scrollbar_->move(this->width() - vert_scrollbar_->width() - 1, 0);
      vert_scrollbar_->setMaximum(zoomed_height - available_height);
      vert_scrollbar_->setPageStep(available_height);

      zoomed_height = available_height;
    }

    // Rather than make a huge surface, we still crop at our width/height and then signal a matrix
    child_matrix.scale(x_scale, y_scale, 1.0F);

    child_size = QSize(zoomed_width, zoomed_height);
  }

  widget_->resize(child_size);
  widget_->move(available_width / 2 - child_size.width() / 2,
                available_height / 2 - child_size.height() / 2);

  emit RequestScale(child_matrix);

  ScrollBarMoved();
}

int ViewerSizer::GetZoomedValue(int value)
{
  return qRound(value * static_cast<double>(zoom_) * 0.01);
}

void ViewerSizer::ScrollBarMoved()
{
  QMatrix4x4 mat;

  float x_scroll, y_scroll;

  if (horiz_scrollbar_->isVisible()) {
    int zoomed_width = GetZoomedValue(width_);
    x_scroll = (zoomed_width/2 - horiz_scrollbar_->value() - widget_->width() / 2) * (2.0 / zoomed_width);
  } else {
    x_scroll = 0;
  }

  if (vert_scrollbar_->isVisible()) {
    int zoomed_height = GetZoomedValue(height_);
    y_scroll = (zoomed_height/2 - vert_scrollbar_->value() - widget_->height() / 2) * (2.0 / zoomed_height);
  } else {
    y_scroll = 0;
  }

  // Zero translate is centered, so we need to determine how much "off center" we are
  mat.translate(x_scroll, y_scroll);

  emit RequestTranslate(mat);
}

OLIVE_NAMESPACE_EXIT
