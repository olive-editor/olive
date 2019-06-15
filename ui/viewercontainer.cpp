/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "viewercontainer.h"

#include <QWidget>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QApplication>

#include "viewerwidget.h"
#include "panels/viewer.h"
#include "timeline/sequence.h"
#include "global/debug.h"

// enforces aspect ratio
ViewerContainer::ViewerContainer(QWidget *parent) :
  QWidget(parent),
  fit(true),
  child(nullptr)
{
  horizontal_scrollbar = new QScrollBar(Qt::Horizontal, this);
  vertical_scrollbar = new QScrollBar(Qt::Vertical, this);

  horizontal_scrollbar->setVisible(false);
  vertical_scrollbar->setVisible(false);

  horizontal_scrollbar->setSingleStep(20);
  vertical_scrollbar->setSingleStep(20);

  child = new ViewerWidget(this);
  child->container = this;

  connect(horizontal_scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scroll_changed()));
  connect(vertical_scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scroll_changed()));
}

ViewerContainer::~ViewerContainer() {}

void ViewerContainer::dragScrollPress(const QPoint &p) {
  drag_start_x = p.x();
  drag_start_y = p.y();

  horiz_start = horizontal_scrollbar->value();
  vert_start = vertical_scrollbar->value();
}

void ViewerContainer::dragScrollMove(const QPoint &p) {
  int this_x = p.x();
  int this_y = p.y();

  horizontal_scrollbar->setValue(horiz_start + (drag_start_x-this_x));
  vertical_scrollbar->setValue(vert_start + (drag_start_y-this_y));
}

void ViewerContainer::parseWheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::AltModifier) {
    QApplication::sendEvent(horizontal_scrollbar, event);
  } else {
    QApplication::sendEvent(vertical_scrollbar, event);
  }
}

void ViewerContainer::adjust() {
  if (viewer->seq != nullptr) {
    if (child->waveform) {
      child->move(0, 0);
      child->resize(size());
    } else {
      horizontal_scrollbar->setVisible(false);
      vertical_scrollbar->setVisible(false);

      int zoomed_width = qRound(double(viewer->seq->width())*zoom);
      int zoomed_height = qRound(double(viewer->seq->height())*zoom);

      if (fit || zoomed_width > width() || zoomed_height > height()) {
        // if the zoom size is greater than or equal to the available area, only use the available area

        double aspect_ratio = double(viewer->seq->width())/double(viewer->seq->height());

        int widget_x = 0;
        int widget_y = 0;
        int widget_width = width();
        int widget_height = height();

        if (!fit) {
          widget_width -= vertical_scrollbar->sizeHint().width();
          widget_height -= horizontal_scrollbar->sizeHint().height();
        }

        double widget_ar = double(widget_width) / double(widget_height);

        bool widget_is_wider_than_sequence = widget_ar > aspect_ratio;

        if (widget_is_wider_than_sequence) {
          widget_width = widget_height * aspect_ratio;
          widget_x = (width() / 2) - (widget_width / 2);
        } else {
          widget_height = widget_width / aspect_ratio;
          widget_y = (height() / 2) - (widget_height / 2);
        }

        child->move(widget_x, widget_y);
        child->resize(widget_width, widget_height);

        if (fit) {
          zoom = double(widget_width) / double(viewer->seq->width());
        } else if (zoomed_width > width() || zoomed_height > height()) {
          horizontal_scrollbar->setVisible(true);
          vertical_scrollbar->setVisible(true);

          horizontal_scrollbar->setMaximum(zoomed_width - width());
          vertical_scrollbar->setMaximum(zoomed_height - height());

          horizontal_scrollbar->setValue(horizontal_scrollbar->maximum()/2);
          vertical_scrollbar->setValue(vertical_scrollbar->maximum()/2);

          adjust_scrollbars();
        }
      } else {
        // if the zoom size is smaller than the available area, scale the surface down

        int zoomed_x = 0;
        int zoomed_y = 0;

        if (zoomed_width < width()) zoomed_x = (width()>>1)-(zoomed_width>>1);
        if (zoomed_height < height()) zoomed_y = (height()>>1)-(zoomed_height>>1);

        child->move(zoomed_x, zoomed_y);
        child->resize(zoomed_width, zoomed_height);
      }
    }
  }
}

void ViewerContainer::adjust_scrollbars() {
  horizontal_scrollbar->move(0, height()-horizontal_scrollbar->height());
  horizontal_scrollbar->setFixedWidth(qMax(0, width()-vertical_scrollbar->width()));
  horizontal_scrollbar->setPageStep(width());

  vertical_scrollbar->move(width() - vertical_scrollbar->width(), 0);
  vertical_scrollbar->setFixedHeight(qMax(0, height()-horizontal_scrollbar->height()));
  vertical_scrollbar->setPageStep(height());
}

void ViewerContainer::resizeEvent(QResizeEvent *event) {
  event->accept();
  adjust();
}

void ViewerContainer::scroll_changed() {
  child->set_scroll(
        double(horizontal_scrollbar->value())/double(horizontal_scrollbar->maximum()),
        double(vertical_scrollbar->value())/double(vertical_scrollbar->maximum())
        );
}
