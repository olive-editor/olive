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

#ifndef PROJECTEXPLORERLISTVIEWBASE_H
#define PROJECTEXPLORERLISTVIEWBASE_H

#include <QListView>

/**
 * @brief A QListView derivative that contains functionality used by both List view and Icon view (which are both based
 * on QListView)
 */
class ProjectExplorerListViewBase : public QListView
{
  Q_OBJECT
public:
  ProjectExplorerListViewBase(QWidget* parent);

protected:
  /**
   * @brief Double click event override
   *
   * Function that signals DoubleClickedView().
   *
   * FIXME: This code is the same as the code in ProjectExplorerTreeView. Is there a way to merge these two through
   * subclassing?
   */
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
  /**
   * @brief Unconditional double click signal
   *
   * Emits a signal when the view is double clicked but not on any particular item
   */
  void DoubleClickedEmptyArea();
};

#endif // PROJECTEXPLORERLISTVIEWBASE_H
