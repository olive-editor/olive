/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "gizmoselection.h"
#include "node/gizmo/point.h"

#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainterPath>


namespace olive {

GizmoSelection::GizmoSelection(QWidget *owner,
                               QTransform & last_draw_transform_) :
  QObject(owner),
  owner_(owner),
  gizmo_last_draw_transform_(last_draw_transform_),
  current_gizmo_(nullptr),
  hovered_gizmo_(nullptr),
  gizmos_(nullptr),
  lasso_path_( new QPainterPath()),
  draw_lasso_flag_(false)
{
  owner->setMouseTracking( true);
//  owner->installEventFilter(this);
}

void olive::GizmoSelection::startLassoSelection(QMouseEvent *event)
{
  delete lasso_path_;  // method 'clear' is available since QT 5.13 only
  lasso_path_ = new QPainterPath();
  lasso_path_->moveTo( gizmo_last_draw_transform_.inverted().map(event->pos()));
  draw_lasso_flag_ = true;
}

void GizmoSelection::OnMouseLeftPress(QMouseEvent *event)
{
  if (gizmos_)
  {
    if (event->modifiers() & Qt::ControlModifier) {
      // CTRL + click selects all points wherever the click happens
      selectAllGizmos();
    } else {

      QList<NodeGizmo *> pressed_gizmos = tryGizmoPress(gizmo_last_draw_transform_.inverted().map(event->pos()));

      if (pressed_gizmos.size() > 0) {
        // if more gizmos are selected, choose main point over control point, unless ALT was pressed
        if (event->modifiers() & Qt::AltModifier) {
          current_gizmo_ = pressed_gizmos.last();
        } else {
          current_gizmo_ = pressed_gizmos.first();
        }
      } else {
        current_gizmo_ = nullptr;
        startLassoSelection(event);
      }

      if (current_gizmo_ != nullptr) {
        if (selected_gizmos_.contains(current_gizmo_)) {
          // click on a gizmo already selected. Deselect the point if shift is pressed
          if (event->modifiers() & Qt::ShiftModifier) {
            current_gizmo_->SetSelected( false);
            selected_gizmos_.removeAll( current_gizmo_);
          }
        } else {
          // click on a new gizmo. When SHIFT, add to selection; otherwise replace selection
          if ((event->modifiers() & Qt::ShiftModifier) == 0) {
            deselectAllGizmos();
          }
          addToSelection( current_gizmo_);
        }
      } else {
        deselectAllGizmos();
      }
    }
  }

  owner_->update();
}

void GizmoSelection::OnMouseRelease(QMouseEvent * /*event*/)
{
  // TODO_ extract frunction
  for (auto it=gizmos_->GetGizmos().crbegin(); it!=gizmos_->GetGizmos().crend(); it++) {
    NodeGizmo *gizmo = *it;
    if (gizmo->IsVisible()) {
      if (PointGizmo *point = dynamic_cast<PointGizmo*>(gizmo)) {
        if (lasso_path_->contains(point->GetPoint())) {
          addToSelection(point);
        }
      }
    }
  }

  static int i = 0;
  qDebug() << "release " << ++i;

  draw_lasso_flag_ = false;
  owner_->update();
}

void GizmoSelection::OnMouseMove(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    lasso_path_->lineTo( gizmo_last_draw_transform_.inverted().map(event->pos()));
    owner_->update();
  }

  if (event->buttons() == Qt::NoButton) {
    onMouseHover( event);
  }
}

void GizmoSelection::DrawSelection(QPainter &painter)
{
  if (gizmos_ && draw_lasso_flag_)
  {
    painter.setPen( QPen(Qt::red, 4));
    //painter.setBrush(QBrush(Qt::darkYellow, Qt::BDiagPattern));
    painter.setBrush( QColor(0x80, 0x80,0x20, 0x60));
    painter.drawPath( *lasso_path_);
  }
}


void GizmoSelection::onMouseHover(QMouseEvent *event)
{
  QList<NodeGizmo *> hovered_list = tryGizmoPress(gizmo_last_draw_transform_.inverted().map(event->pos()));

  if (hovered_gizmo_) {
    hovered_gizmo_->SetHovered( false);
    hovered_gizmo_ = nullptr;
    owner_->update();
  }

  if (hovered_list.size() > 0) {
    hovered_gizmo_ = hovered_list.first();
    hovered_gizmo_->SetHovered( true);
    owner_->update();
  }
}

void GizmoSelection::deselectAllGizmos()
{
  std::for_each( selected_gizmos_.begin(), selected_gizmos_.end(),
                 [](NodeGizmo * g){ g->SetSelected(false);});
  selected_gizmos_.clear();

  owner_->update();
}

void GizmoSelection::selectAllGizmos()
{
  for (auto it=gizmos_->GetGizmos().crbegin(); it!=gizmos_->GetGizmos().crend(); it++) {
    NodeGizmo *gizmo = *it;
    if (gizmo->IsVisible()) {
      if (PointGizmo *point = dynamic_cast<PointGizmo*>(gizmo)) {
        addToSelection(point);
      }
    }
  }

  owner_->update();
}

void GizmoSelection::addToSelection(NodeGizmo *gizmo, bool prepend)
{
  if ( ! selected_gizmos_.contains(gizmo)) {
    gizmo->SetSelected( true);

    if (prepend) {
      selected_gizmos_.prepend(gizmo);
    } else {
      selected_gizmos_.append(gizmo);
    }
  }
}

QList<NodeGizmo *> GizmoSelection::tryGizmoPress(const QPointF &p)
{
  QList<NodeGizmo *> pressed_gizmos;

  if (gizmos_) {

    for (auto it=gizmos_->GetGizmos().crbegin(); it!=gizmos_->GetGizmos().crend(); it++) {
      NodeGizmo *gizmo = *it;
      if (gizmo->IsVisible()) {
        if (PointGizmo *point = dynamic_cast<PointGizmo*>(gizmo)) {
          if (point->GetClickingRect(gizmo_last_draw_transform_).contains(p)) {
            // insert control points at the end and main points at the begin
            if (point->GetSmaller()) {
              pressed_gizmos.append(point);
            } else {
              pressed_gizmos.prepend(point);
            }
          }
        }
      }
    }
  }

  return pressed_gizmos;
}

}  // namespace olive

