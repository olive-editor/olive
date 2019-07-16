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

#ifndef NODEVIEWITEM_H
#define NODEVIEWITEM_H

#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QLinearGradient>

#include "node/node.h"

class NodeViewItem : public QGraphicsRectItem
{
public:
  NodeViewItem(QGraphicsItem* parent = nullptr);

  void SetColor(const QColor& color);

  void SetNode(Node* n);
  Node* node();

  bool IsExpanded();
  void SetExpanded(bool e);

  QRectF GetParameterConnectorRect(int index);

  QPointF GetParameterTextPoint(int index);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
  void UpdateGradient();

  QRectF expand_hitbox_;

  Node* node_;

  QColor color_;

  QRectF title_bar_rect_;

  QRectF content_rect_;

  QFont font;

  QFontMetrics font_metrics;

  int node_connector_size_;

  bool expanded_;

  bool standard_click_;
};

#endif // NODEVIEWITEM_H
