/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef PROJECTEXPLORERTREEVIEW_H
#define PROJECTEXPLORERTREEVIEW_H

#include <QTreeView>

#include "common/define.h"

namespace olive {

/**
 * @brief The view widget used when ProjectExplorer is in Tree View
 *
 * A fairly simple subclass of QTreeView that provides a double clicked signal whether the index is valid or not
 * (QAbstractItemView has a doubleClicked() signal but it's only emitted with a valid index).
 */
class ProjectExplorerTreeView : public QTreeView
{
  Q_OBJECT
public:
  ProjectExplorerTreeView(QWidget* parent);

protected:
  /**
   * @brief Double click event override
   *
   * Function that signals DoubleClickedView().
   *
   * FIXME: This code is the same as the code in ProjectExplorerListViewBase. Is there a way to merge these two through
   *
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

}

#endif // PROJECTEXPLORERTREEVIEW_H
