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

#ifndef VALUESWIZZLEWIDGET_H
#define VALUESWIZZLEWIDGET_H

#include <QGraphicsView>
#include <QGridLayout>
#include <QPushButton>
#include <QWidget>

#include "node/globals.h"
#include "node/swizzlemap.h"
#include "widget/nodeview/curvedconnectoritem.h"

namespace olive {

class SwizzleConnectorItem : public CurvedConnectorItem
{
public:
  SwizzleConnectorItem(QGraphicsItem *parent = nullptr);

  const SwizzleMap::From &from() const { return from_; }
  size_t to() const { return to_index_; }
  void set_from(const SwizzleMap::From &f) { from_ = f; }
  void set_to(size_t t) { to_index_ = t; }

private:
  SwizzleMap::From from_;
  size_t to_index_;

};

class ValueSwizzleWidget : public QGraphicsView
{
  Q_OBJECT
public:
  explicit ValueSwizzleWidget(QWidget *parent = nullptr);

  bool DeleteSelected();

  void set(const ValueParams &g, const NodeInput &input);

protected:
  virtual void drawBackground(QPainter *p, const QRectF &r) override;

  virtual void mousePressEvent(QMouseEvent *e) override;
  virtual void mouseMoveEvent(QMouseEvent *e) override;
  virtual void mouseReleaseEvent(QMouseEvent *e) override;

  virtual void resizeEvent(QResizeEvent *e) override;

private:
  QRect draw_channel(QPainter *p, size_t i, int x, int y, const type_t &name);

  void set_map(const SwizzleMap &map);

  size_t from_count() const { return from_.empty() ? 0 : from_.at(0).size(); }
  size_t to_count() const;

  QPoint get_connect_point_of_from(const SwizzleMap::From &from);
  QPoint get_connect_point_of_to(size_t index);

  void adjust_all();
  void make_item(const SwizzleMap::From &from, size_t to);
  void clear_all();
  SwizzleMap get_map_from_connectors() const;

  void refresh_outputs();
  void refresh_pixmap();

  QGraphicsScene *scene_;
  int channel_width_;
  int channel_height_;
  bool drag_from_;
  union DragSource
  {
    SwizzleMap::From from;
    size_t to;
  };
  DragSource drag_source_;
  bool new_item_connected_;

  NodeInput input_;
  std::vector<NodeOutput> outputs_;
  std::vector<value_t> from_;
  std::vector< std::vector< QRect > > from_rects_;
  std::vector<QRect> to_rects_;
  ValueParams value_params_;

  struct OutputRect
  {
    NodeOutput output;
    QRect rect;
  };

  std::vector<OutputRect> output_rects_;

  SwizzleMap cached_map_;
  SwizzleConnectorItem *new_item_;
  QPoint drag_start_;

  std::vector<SwizzleConnectorItem *> connectors_;

  QPixmap pixmap_;

private slots:
  void hint_changed(const NodeInput &input);

  void connection_changed(const NodeOutput &output, const NodeInput &input);

  void output_menu_selection(QAction *action);

  void modify_input(const SwizzleMap &map);

};

}

#endif // VALUESWIZZLEWIDGET_H
