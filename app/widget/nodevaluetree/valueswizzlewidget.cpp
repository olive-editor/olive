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

#include "valueswizzlewidget.h"

#include <QDebug>
#include <QMouseEvent>
#include <QScrollBar>

#include "config/config.h"
#include "core.h"
#include "node/node.h"
#include "node/nodeundo.h"
#include "widget/menu/menu.h"

namespace olive {

#define super QGraphicsView

ValueSwizzleWidget::ValueSwizzleWidget(QWidget *parent) :
  super{parent}
{
  setFrameShape(QFrame::NoFrame);
  setFrameShadow(QFrame::Plain);
  setRenderHint(QPainter::Antialiasing);
  setBackgroundRole(QPalette::Base);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  setDragMode(RubberBandDrag);

  scene_ = new QGraphicsScene(this);
  setScene(scene_);

  new_item_ = nullptr;

  channel_height_ = this->fontMetrics().height() * 3 / 2;
  channel_width_ = channel_height_ * 2;

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

bool ValueSwizzleWidget::DeleteSelected()
{
  auto sel = scene_->selectedItems();
  if (sel.isEmpty()) {
    return false;
  }

  auto map = get_map_from_connectors();

  for (auto s : sel) {
    auto conn = static_cast<SwizzleConnectorItem*>(s);
    map.remove(conn->to());
  }

  set_map(map);
  modify_input(map);

  return true;
}

void ValueSwizzleWidget::set(const ValueParams &g, const NodeInput &input)
{
  if (input_.IsValid()) {
    disconnect(input_.node(), &Node::InputValueHintChanged, this, &ValueSwizzleWidget::hint_changed);
    disconnect(input_.node(), &Node::InputConnected, this, &ValueSwizzleWidget::connection_changed);
    disconnect(input_.node(), &Node::InputDisconnected, this, &ValueSwizzleWidget::connection_changed);
  }

  input_ = input;
  from_.clear();
  outputs_.clear();
  value_params_ = g;

  if (input_.IsValid()) {
    Node *n = input_.node();

    refresh_outputs();

    set_map(n->GetValueHintForInput(input.input(), input.element()).swizzle());

    connect(input_.node(), &Node::InputValueHintChanged, this, &ValueSwizzleWidget::hint_changed);
    connect(input_.node(), &Node::InputConnected, this, &ValueSwizzleWidget::connection_changed);
    connect(input_.node(), &Node::InputDisconnected, this, &ValueSwizzleWidget::connection_changed);
  }
}

void ValueSwizzleWidget::drawBackground(QPainter *gp, const QRectF &r)
{
  gp->drawPixmap(0, 0, pixmap_);
}

void ValueSwizzleWidget::mousePressEvent(QMouseEvent *e)
{
  QPoint p = mapToScene(e->pos()).toPoint();

  bool found = false;

  for (auto it = output_rects_.cbegin(); it != output_rects_.cend(); it++) {
    const OutputRect &orr = *it;
    if (orr.rect.contains(p)) {
      const NodeOutput &o = orr.output;

      o.node()->Retranslate();

      Menu m(this);

      m.setProperty("oldout", QVariant::fromValue(o));

      for (auto jt = o.node()->outputs().cbegin(); jt != o.node()->outputs().cend(); jt++) {
        m.AddActionWithData(jt->name, jt->id, o.output());
      }

      connect(&m, &Menu::triggered, this, &ValueSwizzleWidget::output_menu_selection);

      m.exec(QCursor::pos());

      found = true;
      break;
    }
  }

  if (!found) {
    for (size_t i = 0; i < from_rects_.size(); i++) {
      const std::vector<QRect> &l = from_rects_.at(i);
      for (size_t j = 0; j < l.size(); j++) {
        if (l.at(j).contains(p)) {
          drag_from_ = true;
          drag_source_.from = SwizzleMap::From(i, j);
          drag_start_ = get_connect_point_of_from(drag_source_.from);
          new_item_connected_ = false;

          found = true;
          break;
        }
      }
    }

    if (!found) {
      for (size_t j = 0; j < to_rects_.size(); j++) {
        const QRect &r = to_rects_.at(j);

        if (r.contains(p)) {
          drag_from_ = false;
          drag_source_.to = j;
          drag_start_ = get_connect_point_of_to(j);
          new_item_connected_ = true;

          // "to"s can only have one connection, so if there's already a connection here, we'll remove it
          for (auto it = connectors_.cbegin(); it != connectors_.cend(); it++) {
            SwizzleConnectorItem *s = *it;
            if (s->to() == drag_source_.to) {
              // Grab this item
              new_item_ = s;

              // Flip so we're dragging the connector from its old position
              drag_from_ = true;
              drag_source_.from = new_item_->from();
              drag_start_ = get_connect_point_of_from(drag_source_.from);
              new_item_connected_ = true;

              // Remove from list because we'll delete this later
              connectors_.erase(it);
              break;
            }
          }
        }
      }
    }

    if (found) {
      if (!new_item_) {
        new_item_ = new SwizzleConnectorItem();
        if (drag_from_) {
          new_item_->set_from(drag_source_.from);
        } else {
          new_item_->set_to(drag_source_.to);
        }
        scene_->addItem(new_item_);
      }
    } else {
      super::mousePressEvent(e);
    }
  }
}

void ValueSwizzleWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (new_item_) {
    QPoint p = mapToScene(e->pos()).toPoint();
    QPoint end_point;

    new_item_connected_ = false;

    if (drag_from_) {
      // Only register with "to" rects
      for (size_t j = 0; j < to_rects_.size(); j++) {
        const QRect &r = to_rects_.at(j);
        if (r.contains(p)) {
          end_point = get_connect_point_of_to(j);
          new_item_connected_ = true;
          new_item_->set_to(j);
          break;
        }
      }
    } else {
      // Only register with "from" rects
      for (size_t i = 0; i < from_rects_.size(); i++) {
        const std::vector<QRect> &l = from_rects_.at(i);
        for (size_t j = 0; j < l.size(); j++) {
          const QRect &r = l.at(j);
          if (r.contains(p)) {
            end_point = get_connect_point_of_from(SwizzleMap::From(i, j));
            new_item_connected_ = true;
            new_item_->set_from(SwizzleMap::From(i, j));
            break;
          }
        }
      }
    }

    if (!new_item_connected_) {
      end_point = e->pos();
    }
    new_item_->SetConnected(new_item_connected_);

    new_item_->SetPoints(drag_start_, end_point);
  } else {
    super::mouseMoveEvent(e);
  }
}

void ValueSwizzleWidget::mouseReleaseEvent(QMouseEvent *e)
{
  if (new_item_) {
    SwizzleMap map = get_map_from_connectors();

    if (new_item_connected_) {
      map.insert(new_item_->to(), new_item_->from());
    }

    delete new_item_;
    new_item_ = nullptr;

    set_map(map);
    modify_input(map);
  } else {
    super::mouseReleaseEvent(e);
  }
}

void ValueSwizzleWidget::resizeEvent(QResizeEvent *e)
{
  this->setSceneRect(this->viewport()->rect());

  refresh_pixmap();

  super::resizeEvent(e);
}

QRect ValueSwizzleWidget::draw_channel(QPainter *p, size_t i, int x, int y, const type_t &name)
{
  static const size_t kChannelColorCount = 4;
  static const QColor kDefaultColor = QColor(16, 16, 16);
  static const QColor kRGBAChannelColors[kChannelColorCount] = {
    QColor(160, 32, 32),
    QColor(32, 160, 32),
    QColor(32, 32, 160),
    QColor(64, 64, 64)
  };

  static const QColor kGrayChannelColors[kChannelColorCount] = {
    QColor(104, 104, 104),
    QColor(80, 80, 80),
    QColor(56, 56, 56),
    QColor(32, 32, 32),
  };

  QRect r(x, y, channel_width_, channel_height_);

  //const QColor *kChannelColors = type_ == TYPE_TEXTURE || type_ == TYPE_COLOR ? kRGBAChannelColors : kGrayChannelColors;
  const QColor *kChannelColors = kGrayChannelColors;

  QColor main_col;
  if (name == "r") {
    main_col = kRGBAChannelColors[0];
  } else if (name == "g") {
    main_col = kRGBAChannelColors[1];
  } else if (name == "b") {
    main_col = kRGBAChannelColors[2];
  } else if (name == "a") {
    main_col = kRGBAChannelColors[3];
  } else {
    main_col = i < kChannelColorCount ? kChannelColors[i] : kDefaultColor;
  }

  if (OLIVE_CONFIG("UseGradients").toBool()) {
    QLinearGradient lg(r.topLeft(), r.bottomLeft());

    lg.setColorAt(0.0, main_col.lighter());
    lg.setColorAt(1.0, main_col);

    p->setBrush(lg);
  } else {
    p->setBrush(main_col);
  }

  p->setPen(Qt::black);
  p->drawRect(r);

  p->setPen(Qt::white);

  QString t = (name == type_t()) ? QString::number(i) : name.toString();
  p->drawText(r, Qt::AlignCenter, t);

  return r;
}

void ValueSwizzleWidget::set_map(const SwizzleMap &map)
{
  clear_all();

  cached_map_ = map;

  if (cached_map_.empty()) {
    // Connect everything directly for empty maps
    size_t lim = std::min(from_count(), to_count());
    for (size_t i = 0; i < lim; i++) {
      make_item(SwizzleMap::From(0, i), i);
    }
  } else {
    for (auto it = cached_map_.cbegin(); it != cached_map_.cend(); it++) {
      make_item(it->second, it->first);
    }
  }

  adjust_all();
}

size_t ValueSwizzleWidget::to_count() const
{
  return cached_map_.empty() ? from_count() : cached_map_.size();
}

QPoint ValueSwizzleWidget::get_connect_point_of_from(const SwizzleMap::From &from)
{
  if (from.output() < from_rects_.size()) {
    const std::vector<QRect> &l = from_rects_.at(from.output());
    if (from.element() < l.size()) {
      const QRect &r = l.at(from.element());
      return QPoint(r.right(), r.center().y());
    }
  }

  return QPoint();
}

QPoint ValueSwizzleWidget::get_connect_point_of_to(size_t index)
{
  if (index < to_rects_.size()) {
    const QRect &r = to_rects_.at(index);
    return QPoint(r.left(), r.center().y());
  }

  return QPoint();
}

void ValueSwizzleWidget::adjust_all()
{
  for (auto it = connectors_.cbegin(); it != connectors_.cend(); it++) {
    SwizzleConnectorItem *s = *it;
    s->SetPoints(get_connect_point_of_from(s->from()), get_connect_point_of_to(s->to()));
  }
}

void ValueSwizzleWidget::make_item(const SwizzleMap::From &from, size_t to)
{
  SwizzleConnectorItem *s = new SwizzleConnectorItem();
  s->set_from(from);
  s->set_to(to);
  s->SetConnected(true);
  scene_->addItem(s);
  connectors_.push_back(s);
}

void ValueSwizzleWidget::clear_all()
{
  qDeleteAll(connectors_);
  connectors_.clear();
}

SwizzleMap ValueSwizzleWidget::get_map_from_connectors() const
{
  SwizzleMap map;
  for (auto it = connectors_.cbegin(); it != connectors_.cend(); it++) {
    SwizzleConnectorItem *s = *it;
    map.insert(s->to(), s->from());
  }
  return map;
}

void ValueSwizzleWidget::refresh_outputs()
{
  if (input_.IsValid()) {
    Node *n = input_.node();

    outputs_ = n->GetConnectedOutputs(input_);

    from_.resize(outputs_.size());
    for (size_t i = 0; i < from_.size(); i++) {
      from_[i] = n->GetFakeConnectedValue(value_params_, outputs_[i], input_.input(), input_.element());
    }

    refresh_pixmap();
  }
}

void ValueSwizzleWidget::refresh_pixmap()
{
  if (outputs_.empty()) {
    return;
  }

  QFontMetrics fm = fontMetrics();
  int to_height = channel_height_ * to_count();
  int from_height = 0;

  for (size_t i = 0; i < outputs_.size(); i++) {
    from_height += fm.height() + channel_height_ * from_.at(i).size();
  }

  int required_height = std::max(from_height, to_height);
  setFixedHeight(required_height);

  pixmap_ = QPixmap(viewport()->width(), required_height);
  pixmap_.fill(Qt::transparent);

  QPainter p(&pixmap_);

  int from_y = 0;

  from_rects_.resize(outputs_.size());
  output_rects_.resize(outputs_.size());

  for (size_t i = 0; i < outputs_.size(); i++) {
    const NodeOutput &output = outputs_.at(i);

    output.node()->Retranslate();

    QString lbl = QStringLiteral("%1 - %2 %3").arg(output.node()->GetLabelAndName(), output.node()->GetOutputName(output.output()), QChar(0x25BE));
    QRect lbl_rect(0, from_y, fm.horizontalAdvance(lbl), fm.height());

    const value_t &from = from_.at(i);

    output_rects_[i] = {output, lbl_rect};

    p.setPen(palette().text().color());
    p.drawText(lbl_rect, Qt::AlignLeft | Qt::AlignTop, lbl);

    from_y += fm.height();

    from_rects_[i].resize(from.size());

    for (size_t j = 0; j < from.size(); j++) {
      QRect rct = draw_channel(&p, j, 0, from_y, from.at(j).id());
      from_rects_[i][j] = rct;
      from_y += rct.height();
    }
  }

  int to_y;
  if (outputs_.size() == 1) {
    to_y = fm.height();
  } else {
    to_y = std::max(0, viewport()->height()/2 - (channel_height_ * int(to_count()))/2);
  }

  to_rects_.resize(to_count());
  for (size_t j = 0; j < to_count(); j++) {
    // FIXME: Get naming scheme for this...
    QRect rct = draw_channel(&p, j, width() - channel_width_ - 1, to_y, type_t());
    to_rects_[j] = rct;
    to_y += rct.height();
  }

  adjust_all();

  viewport()->update();
}

void ValueSwizzleWidget::hint_changed(const NodeInput &input)
{
  if (input_ == input) {
    Node::ValueHint vh = input.node()->GetValueHintForInput(input.input(), input.element());
    set_map(vh.swizzle());
  }
}

void ValueSwizzleWidget::connection_changed(const NodeOutput &output, const NodeInput &input)
{
  if (input_ == input) {
    refresh_outputs();
  }
}

void ValueSwizzleWidget::output_menu_selection(QAction *action)
{
  NodeOutput old_out = static_cast<Menu*>(sender())->property("oldout").value<NodeOutput>();
  NodeOutput new_out(old_out.node(), action->data().toString());

  MultiUndoCommand *c = new MultiUndoCommand();

  c->add_child(new NodeEdgeRemoveCommand(old_out, input_));
  c->add_child(new NodeEdgeAddCommand(new_out, input_, old_out.node()->GetInputConnectionIndex(old_out, input_)));

  Core::instance()->undo_stack()->push(c, tr("Switched connection output from %1 to %2").arg(old_out.GetName(), new_out.GetName()));
}

void ValueSwizzleWidget::modify_input(const SwizzleMap &map)
{
  Node::ValueHint hint = input_.node()->GetValueHintForInput(input_.input(), input_.element());
  hint.set_swizzle(map);
  Core::instance()->undo_stack()->push(new NodeSetValueHintCommand(input_, hint), tr("Edited Channel Swizzle For Input"));
}

SwizzleConnectorItem::SwizzleConnectorItem(QGraphicsItem *parent) :
  CurvedConnectorItem(parent)
{
}

}
