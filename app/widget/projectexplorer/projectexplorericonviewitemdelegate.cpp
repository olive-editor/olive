#include "projectexplorericonviewitemdelegate.h"

#include <QPainter>

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
}

