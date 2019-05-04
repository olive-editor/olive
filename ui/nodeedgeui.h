#ifndef NODEEDGEUI_H
#define NODEEDGEUI_H

#include <QGraphicsPathItem>

#include "nodeui.h"

class NodeEdgeUI : public QGraphicsPathItem {
public:
  NodeEdgeUI(NodeEdge* edge);

  static QPainterPath GetEdgePath(const QPointF& start_pos, const QPointF& end_pos);

  void adjust();
  NodeEdge* edge();

private:
  NodeEdge* edge_;

  NodeUI* output_node_;
  int output_index_;
  NodeUI* input_node_;
  int input_index_;
};

#endif // NODEEDGEUI_H
