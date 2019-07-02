#ifndef PROJECTEXPLORERLISTVIEWITEMDELEGATE_H
#define PROJECTEXPLORERLISTVIEWITEMDELEGATE_H

#include <QStyledItemDelegate>

class ProjectExplorerListViewItemDelegate : public QStyledItemDelegate
{
public:
  ProjectExplorerListViewItemDelegate(QObject *parent = nullptr);

  virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // PROJECTEXPLORERLISTVIEWITEMDELEGATE_H
