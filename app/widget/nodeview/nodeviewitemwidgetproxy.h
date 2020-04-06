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

#ifndef NODEVIEWITEMWIDGETPROXY_H
#define NODEVIEWITEMWIDGETPROXY_H

#include <QWidget>

/**
 * @brief A proxy object to allow NodeViewItem access to CSS functions
 *
 * QGraphicsItems can't take Q_PROPERTYs for CSS stylesheet input, but QWidgets can. This is a hack to allow CSS
 * properties to be set from CSS and then read by NodeViewItem.
 */
class NodeViewItemWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
public:
  NodeViewItemWidget() = default;

  QColor TitleBarColor();
  void SetTitleBarColor(QColor color);

  QColor BorderColor();
  void SetBorderColor(QColor color);
private:
  QColor title_bar_color_;
  QColor border_color_;
};

#endif // NODEVIEWITEMWIDGETPROXY_H
