/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "sourceiconview.h"

#include <QMimeData>
#include <QImage>

#include "panels/project.h"
#include "project/media.h"
#include "project/sourcescommon.h"
#include "global/debug.h"
#include "global/math.h"

SourceIconView::SourceIconView(SourcesCommon &commons) :
  commons_(commons)
{
  setMovement(QListView::Free);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setResizeMode(QListView::Adjust);
  setContextMenuPolicy(Qt::CustomContextMenu);
  setItemDelegate(&delegate_);
  connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(item_click(const QModelIndex&)));
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
}

void SourceIconView::show_context_menu() {
  commons_.show_context_menu(this, selectedIndexes());
}

void SourceIconView::item_click(const QModelIndex& index) {
  if (selectedIndexes().size() == 1 && index.column() == 0) {
    commons_.item_click(project_parent->item_to_media(index), index);
  }
}

void SourceIconView::mousePressEvent(QMouseEvent* event) {
  commons_.mousePressEvent(event);
  if (!indexAt(event->pos()).isValid()) selectionModel()->clear();
  QListView::mousePressEvent(event);
}

void SourceIconView::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  } else {
    QListView::dragEnterEvent(event);
  }
}

void SourceIconView::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  } else {
    QListView::dragMoveEvent(event);
  }
}

void SourceIconView::dropEvent(QDropEvent* event) {
  QModelIndex drop_item = indexAt(event->pos());
  if (!drop_item.isValid()) drop_item = rootIndex();
  commons_.dropEvent(this, event, drop_item, selectedIndexes());
}

void SourceIconView::mouseDoubleClickEvent(QMouseEvent *) {
  if (selectedIndexes().size() == 1) {
    Media* m = project_parent->item_to_media(selectedIndexes().at(0));
    if (m->get_type() == MEDIA_TYPE_FOLDER) {
      setRootIndex(selectedIndexes().at(0));
      emit changed_root();
      return;
    }
  }

  // Double click was not a folder, so we perform the default behavior (sending the double click to SourcesCommon)
  commons_.mouseDoubleClickEvent(selectedIndexes());
}

SourceIconDelegate::SourceIconDelegate(QObject *parent) :
  QStyledItemDelegate (parent)
{
}

QSize SourceIconDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
  if (option.decorationPosition == QStyleOptionViewItem::Top) { // Icon Mode

    return QSize(256, 256);

  } else {

    return QSize(option.decorationSize.height(), option.decorationSize.height());

  }
}

void SourceIconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QFontMetrics fm = painter->fontMetrics();
  QRect img_rect = option.rect;

  if (option.decorationPosition == QStyleOptionViewItem::Top) { // Icon Mode

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
      int timecode_width = fm.width(duration_str);
      int max_name_width = option.rect.width();

      if (timecode_width < option.rect.width() / 2) {
        painter->drawText(text_rect, Qt::AlignBottom | Qt::AlignRight, index.data(Qt::UserRole).toString());
        max_name_width -= timecode_width;
      }

      painter->drawText(text_rect,
                        Qt::AlignBottom | Qt::AlignLeft,
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
  } else if (option.decorationPosition == QStyleOptionViewItem::Left) { // List Mode

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
}
