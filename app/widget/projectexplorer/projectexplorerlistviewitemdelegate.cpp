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

#include "projectexplorerlistviewitemdelegate.h"

#include <QPainter>

ProjectExplorerListViewItemDelegate::ProjectExplorerListViewItemDelegate(QObject *parent) :
  QStyledItemDelegate(parent)
{

}

QSize ProjectExplorerListViewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
  return QSize(option.decorationSize.height(), option.decorationSize.height());
}

void ProjectExplorerListViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QFontMetrics fm = painter->fontMetrics();
  QRect img_rect = option.rect;

  if (option.state & QStyle::State_Selected) {
    painter->fillRect(option.rect, option.palette.highlight());
  }

  img_rect.setWidth(qMin(img_rect.width(), img_rect.height()));

  QIcon ico = index.data(Qt::DecorationRole).value<QIcon>();
  QSize icon_size = ico.actualSize(img_rect.size());
  img_rect = QRect(img_rect.x() + (img_rect.width() / 2 - icon_size.width() / 2),
                   img_rect.y() + (img_rect.height() / 2 - icon_size.height() / 2),
                   icon_size.width(),
                   icon_size.height());
  painter->drawPixmap(img_rect, ico.pixmap(icon_size));

  QRect text_rect = option.rect;
  text_rect.setLeft(text_rect.left() + option.rect.height());

  int maximum_line_count = qMax(1, option.rect.height() / fm.height() - 1);
  QString text;
  if (maximum_line_count == 1) {
    text = index.data(Qt::DisplayRole).toString();
  } else {
    text = index.data(Qt::ToolTipRole).toString();
    if (text.isEmpty()) {
      text = index.data(Qt::DisplayRole).toString();
    } else {
      QStringList strings = text.split("\n");
      while (strings.size() > maximum_line_count) {
        strings.removeLast();
      }
      text = strings.join("\n");
    }
  }

  painter->setPen(option.state & QStyle::State_Selected ?
                    option.palette.highlightedText().color() : option.palette.text().color());

  painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, text);
}
