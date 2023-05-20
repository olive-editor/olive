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

#include "node/swizzlemap.h"
#include "widget/nodeview/curvedconnectoritem.h"

namespace olive {

class SwizzleConnectorItem : public CurvedConnectorItem
{
public:
  SwizzleConnectorItem(QGraphicsItem *parent = nullptr);

  size_t from() const { return from_index_; }
  size_t to() const { return to_index_; }
  void set_from(size_t f) { from_index_ = f; }
  void set_to(size_t t) { to_index_ = t; }

private:
  size_t from_index_;
  size_t to_index_;

};

class ValueSwizzleWidget : public QGraphicsView
{
  Q_OBJECT
public:
  enum Labels
  {
    kNumberLabels,
    kXYZWLabels,
    kRGBALabels,
  };

  explicit ValueSwizzleWidget(QWidget *parent = nullptr);

  bool delete_selected();

  void set(const SwizzleMap &map);
  void set_channels(size_t from, size_t to);

  void set_label_type(Labels l) { labels_ = l; }

protected:
  virtual void drawBackground(QPainter *p, const QRectF &r) override;

  virtual void mousePressEvent(QMouseEvent *e) override;
  virtual void mouseMoveEvent(QMouseEvent *e) override;
  virtual void mouseReleaseEvent(QMouseEvent *e) override;

  virtual void resizeEvent(QResizeEvent *e) override;

signals:
  void value_changed(const SwizzleMap &map);

private:
  QString get_label_text(size_t index) const;

  void draw_channel(QPainter *p, size_t i, int x);
  inline bool channel_is_from(int x) const { return x < get_left_channel_bound(); }
  inline bool channel_is_to(int x) const { return x >= get_right_channel_bound(); }
  inline bool is_inside_bounds(int x) const { return channel_is_from(x) || channel_is_to(x); }
  inline int get_left_channel_bound() const { return channel_width_; }
  inline int get_right_channel_bound() const { return viewport()->width() - channel_width_ - 1; }
  inline size_t get_channel_index_from_y(int y) const { return y / channel_height_; }

  QPoint get_connect_point_of_channel(bool from, size_t index);

  void adjust_all();
  void make_item(size_t from, size_t to);
  void clear_all();
  SwizzleMap get_map_from_connectors() const;

  QGraphicsScene *scene_;
  int channel_width_;
  int channel_height_;
  size_t from_count_;
  size_t to_count_;
  Labels labels_;
  bool drag_from_;
  size_t drag_index_;
  bool new_item_connected_;

  SwizzleMap cached_map_;
  SwizzleConnectorItem *new_item_;
  QPoint drag_start_;

  std::vector<SwizzleConnectorItem *> connectors_;

};

}

#endif // VALUESWIZZLEWIDGET_H
