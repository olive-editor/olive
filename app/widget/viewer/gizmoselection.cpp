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
#include "node/gizmo/polygon.h"
#include "node/gizmo/path.h"

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
  pressed_gizmo_(nullptr),
  hovered_gizmo_(nullptr),
  gizmos_(nullptr),
  lasso_path_( new QPainterPath()),
  draw_lasso_flag_(false)
{
}


void olive::GizmoSelection::startLassoSelection(QMouseEvent *event)
{
  delete lasso_path_;  // method 'QPainterPath::clear' is only available since QT 5.13
  lasso_path_ = new QPainterPath();
  lasso_path_->moveTo( gizmo_last_draw_transform_.inverted().map(event->pos()));
  draw_lasso_flag_ = true;
}

void GizmoSelection::OnMouseLeftPress(QMouseEvent *event)
{
  if (gizmos_)
  {
    QList<NodeGizmo *> pressed_gizmos = tryGizmoPress(gizmo_last_draw_transform_.inverted().map(event->pos()));

    if (pressed_gizmos.size() > 0) {
      // if more gizmos are selected, choose main point over control point, unless ALT was pressed
      if (event->modifiers() & Qt::AltModifier) {
        pressed_gizmo_ = pressed_gizmos.last();
      } else {
        pressed_gizmo_ = pressed_gizmos.first();
      }
      draw_lasso_flag_ = false;
    } else {
      pressed_gizmo_ = nullptr;
      startLassoSelection(event);
    }

    if (pressed_gizmo_ != nullptr) {
      if (selected_gizmos_.contains(pressed_gizmo_)) {
        // click on a gizmo already selected. Deselect the point if shift is pressed
        if (event->modifiers() & Qt::ShiftModifier) {
          pressed_gizmo_->SetSelected( false);
          selected_gizmos_.removeAll( pressed_gizmo_);
        }
      } else {
        // click on a new gizmo. When SHIFT, add to selection; otherwise replace selection
        if ((event->modifiers() & Qt::ShiftModifier) == 0) {
          deselectAllGizmos();
        }
        addToSelection( pressed_gizmo_);
      }
    } else if ((event->modifiers() & Qt::ShiftModifier) == 0) {
      // when SHIF is pressed, this may begin a lasso-append selection
      deselectAllGizmos();
    }
  }

  owner_->update();
}


void GizmoSelection::OnMouseRelease(QMouseEvent * event)
{
  bool toggle = ((event->modifiers() & Qt::ShiftModifier) != 0);
  selectGizmosInsideLasso( toggle);
  draw_lasso_flag_ = false;
  owner_->update();
}


void olive::GizmoSelection::selectGizmosInsideLasso( bool toggle)
{
  if (gizmos_ && draw_lasso_flag_) {
    for (auto it=gizmos_->GetGizmos().crbegin(); it!=gizmos_->GetGizmos().crend(); it++) {
      NodeGizmo *gizmo = *it;
      if (gizmo->IsVisible()) {
        if (PointGizmo *point = dynamic_cast<PointGizmo*>(gizmo)) {
          if (lasso_path_->contains(point->GetPoint())) {
            if (toggle) {
              toggleSelection(point);
            } else {
              addToSelection( point);
            }
          }
        }
      }
    }
  }
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
    painter.setBrush( QColor(0x80, 0x80,0x20, 0x60));
    painter.drawPath( *lasso_path_);
  }
}

bool GizmoSelection::CanMoveGizmo(NodeGizmo *gizmo) const
{
  return  (pressed_gizmo_ == gizmo) ||
      ((pressed_gizmo_ != nullptr) && (gizmo->CanBeDraggedInGroup()));
}


bool GizmoSelection::CanStartDrag() const {
  // if last "LeftPress" was not on a point, do not
  // drag points on a mouse move
  return (pressed_gizmo_ != nullptr);
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


void GizmoSelection::addToSelection(NodeGizmo *gizmo)
{
  if ( ! selected_gizmos_.contains(gizmo)) {
    gizmo->SetSelected( true);
    selected_gizmos_.append(gizmo);
  }
}

void GizmoSelection::toggleSelection(NodeGizmo *gizmo)
{
  if (selected_gizmos_.contains(gizmo)) {
    gizmo->SetSelected( false);
    selected_gizmos_.removeAll( gizmo);
  } else {
    gizmo->SetSelected( true);
    selected_gizmos_.append( gizmo);
  }
}

QList<NodeGizmo *> GizmoSelection::tryGizmoPress(const QPointF &p)
{
  QList<NodeGizmo *> pressed_main_points;
  QList<NodeGizmo *> pressed_control_points;
  QList<NodeGizmo *> pressed_path;

  if (gizmos_) {

    for (auto it=gizmos_->GetGizmos().crbegin(); it!=gizmos_->GetGizmos().crend(); it++) {
      NodeGizmo *gizmo = *it;
      if (gizmo->IsVisible()) {
        if (PointGizmo *point = dynamic_cast<PointGizmo*>(gizmo)) {
          if (point->GetClickingRect(gizmo_last_draw_transform_).contains(p)) {
            if (point->GetSmaller()) {
              pressed_control_points.append(point);
            } else {
              pressed_main_points.append(point);
            }
          }
        } else if (PolygonGizmo *poly = dynamic_cast<PolygonGizmo*>(gizmo)) {
          if (poly->GetPolygon().containsPoint(p, Qt::OddEvenFill)) {
            pressed_path.append(poly);
          }
        } else if (PathGizmo *path = dynamic_cast<PathGizmo*>(gizmo)) {
          if (path->GetPath().contains(p)) {
            pressed_path.append(path);
          }
        }
      }
    }
  }

  // This order is such that main point take the precedence over control points and full path
  return QList<NodeGizmo *>() << pressed_main_points << pressed_path << pressed_control_points;
}

}  // namespace olive

