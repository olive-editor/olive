/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef NODEVIEWMINIMAP_H
#define NODEVIEWMINIMAP_H

#include <QGraphicsView>

#include "nodeviewscene.h"

namespace olive {

class NodeViewMiniMap : public QGraphicsView
{
  Q_OBJECT
public:
  NodeViewMiniMap(NodeViewScene *scene, QWidget *parent = nullptr);

public slots:
  void SetViewportRect(const QPolygonF &rect);

signals:
  void Resized();

  void MoveToScenePoint(const QPointF &pos);

protected:
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void resizeEvent(QResizeEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override{}

private slots:
  void SceneChanged(const QRectF &bounding);

  void SetDefaultSize();

private:
  bool MouseInsideResizeTriangle(QMouseEvent *event);

  void EmitMoveSignal(QMouseEvent *event);

  int resize_triangle_sz_;

  QPolygonF viewport_rect_;

  bool resizing_;

  QPoint resize_anchor_;

};

}

#endif // NODEVIEWMINIMAP_H
