#ifndef PROJECTEXPLORERICONVIEWITEMDELEGATE_H
#define PROJECTEXPLORERICONVIEWITEMDELEGATE_H

#include <QStyledItemDelegate>

class ProjectExplorerIconViewItemDelegate : public QStyledItemDelegate {
public:
  ProjectExplorerIconViewItemDelegate(QObject *parent = nullptr);

  virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // PROJECTEXPLORERICONVIEWITEMDELEGATE_H
