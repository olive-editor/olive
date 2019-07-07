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

#ifndef PROJECTEXPLORERICONVIEWITEMDELEGATE_H
#define PROJECTEXPLORERICONVIEWITEMDELEGATE_H

#include <QStyledItemDelegate>

/**
 * @brief The delegate that's used to draw items when ProjectExplorer is in Icon view
 */
class ProjectExplorerIconViewItemDelegate : public QStyledItemDelegate {
public:
  ProjectExplorerIconViewItemDelegate(QObject *parent = nullptr);

  virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // PROJECTEXPLORERICONVIEWITEMDELEGATE_H
