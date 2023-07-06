/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef CURVEDCONNECTORITEM_H
#define CURVEDCONNECTORITEM_H

#include <QGraphicsRectItem>

#include "nodeviewcommon.h"

namespace olive {

class CurvedConnectorItem : public QGraphicsPathItem
{
public:
  CurvedConnectorItem(QGraphicsItem* parent = nullptr);

  /**
   * @brief Set the connected state of this line
   *
   * When the edge is not connected, it visually depicts this by coloring the line grey. When an edge is connected or
   * a potential connection is valid, the line is colored white. This function sets whether the line should be grey
   * (false) or white (true).
   *
   * Using SetEdge() automatically sets this to true. Under most circumstances this should be left alone, and only
   * be set when an edge is being created/dragged.
   */
  void SetConnected(bool c);

  bool IsConnected() const
  {
    return connected_;
  }

  /**
   * @brief Set points to create curve from
   */
  void SetPoints(const QPointF& start, const QPointF& end);

  /**
   * @brief Set highlighted state
   *
   * Changes color of edge.
   */
  void SetHighlighted(bool e);

  /**
   * @brief Set whether edges should be drawn as curved or as straight lines
   */
  void SetCurved(bool e);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual NodeViewCommon::FlowDirection GetFromDirection() const;
  virtual NodeViewCommon::FlowDirection GetToDirection() const;

private:
  void UpdateCurve();

  bool connected_;

  bool highlighted_;

  bool curved_;

  int edge_width_;

  QPointF cached_start_;
  QPointF cached_end_;

};

}

#endif // CURVEDCONNECTORITEM_H
