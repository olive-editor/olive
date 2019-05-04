#include "nodeedgeui.h"

#include <QPen>
#include <QGraphicsScene>

#include "global/math.h"
#include "ui/effectui.h"

NodeEdgeUI::NodeEdgeUI(NodeEdge* edge) :
  edge_(edge),
  output_node_(nullptr),
  input_node_(nullptr)
{
  setPen(QPen(Qt::white, 2));
}

void NodeEdgeUI::adjust()
{
  if (output_node_ == nullptr) {
    // Locate the output and input nodes' UIs

    QList<QGraphicsItem*> all_items = scene()->items();

    for (int j=0;j<all_items.size();j++) {
      NodeUI* node = dynamic_cast<NodeUI*>(all_items.at(j));
      if (node != nullptr) {

        // Check if this node has the output row
        int row_index = node->GetNode()->IndexOfRow(edge_->output());

        if (row_index > -1) {
          output_node_ = node;
          output_index_ = row_index;
        }

        // Check if this node has the input row
        row_index = node->GetNode()->IndexOfRow(edge_->input());

        if (row_index > -1) {
          input_node_ = node;
          input_index_ = row_index;
        }
      }
    }
  }

  setPath(GetEdgePath(output_node_->pos() + output_node_->GetNodeSocketRects().at(output_index_).center(),
                      input_node_->pos() + input_node_->GetNodeSocketRects().at(input_index_).center()));
}

NodeEdge *NodeEdgeUI::edge()
{
  return edge_;
}

QPainterPath NodeEdgeUI::GetEdgePath(const QPointF &start_pos, const QPointF &end_pos)
{
  double mid_x = double_lerp(start_pos.x(), end_pos.x(), 0.5);

  QPainterPath path_;
  path_.moveTo(start_pos);
  path_.cubicTo(QPointF(mid_x, start_pos.y()),
                QPointF(mid_x, end_pos.y()),
                end_pos);

  return path_;
}
