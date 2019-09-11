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

#include "projectexplorericonviewitemdelegate.h"

#include <QPainter>

#include "common/qtversionabstraction.h"

ProjectExplorerIconViewItemDelegate::ProjectExplorerIconViewItemDelegate(QObject *parent) :
  QStyledItemDelegate (parent)
{
}

QSize ProjectExplorerIconViewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
  Q_UNUSED(option)

  return QSize(256, 256);
}

void ProjectExplorerIconViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QFontMetrics fm = painter->fontMetrics();
  QRect img_rect = option.rect;

  // Draw Text
  if (fm.height() < option.rect.height() / 2) {
    img_rect.setHeight(img_rect.height()-fm.height());

    QRect text_rect = option.rect;
    text_rect.setTop(text_rect.top() + option.rect.height() - fm.height());

    QColor text_bgcolor;
    QColor text_fgcolor;

    if (option.state & QStyle::State_Selected) {
      text_bgcolor = option.palette.highlight().color();
      text_fgcolor = option.palette.highlightedText().color();
    } else {
      text_bgcolor = Qt::white;
      text_fgcolor = Qt::black;
    }

    painter->fillRect(text_rect, text_bgcolor);
    painter->setPen(text_fgcolor);

    QString duration_str = index.data(Qt::UserRole).toString();

    int timecode_width = QFontMetricsWidth(&fm, duration_str);

    int max_name_width = option.rect.width();

    if (timecode_width < option.rect.width() / 2) {
      painter->drawText(text_rect, Qt::AlignBottom | Qt::AlignRight, index.data(Qt::UserRole).toString());
      max_name_width -= timecode_width;
    }

    painter->drawText(text_rect,
                      static_cast<int>(Qt::AlignBottom | Qt::AlignLeft),
                      fm.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, max_name_width));

  }

  // Draw image
  QIcon ico = index.data(Qt::DecorationRole).value<QIcon>();
  QSize icon_size = ico.actualSize(img_rect.size());
  img_rect = QRect(img_rect.x() + (img_rect.width() / 2 - icon_size.width() / 2),
                   img_rect.y() + (img_rect.height() / 2 - icon_size.height() / 2),
                   icon_size.width(),
                   icon_size.height());
  painter->drawPixmap(img_rect, ico.pixmap(icon_size));

  if (option.state & QStyle::State_Selected) {
    QColor highlight_color = option.palette.highlight().color();
    highlight_color.setAlphaF(0.5);

    painter->setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter->fillRect(img_rect, highlight_color);
  }
}

