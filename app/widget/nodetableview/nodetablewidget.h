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

#ifndef NODETABLEWIDGET_H
#define NODETABLEWIDGET_H

#include "nodetableview.h"
#include "widget/timebased/timebased.h"

OLIVE_NAMESPACE_ENTER

class NodeTableWidget : public TimeBasedWidget
{
public:
  NodeTableWidget(QWidget* parent = nullptr);

  void SelectNodes(const QList<Node*>& nodes)
  {
    view_->SelectNodes(nodes);
  }

  void DeselectNodes(const QList<Node*>& nodes)
  {
    view_->DeselectNodes(nodes);
  }

protected:
  virtual void TimeChangedEvent(const int64_t& ts) override
  {
    UpdateView();
  }

private:
  void UpdateView()
  {
    view_->SetTime(GetTime());
  }

  NodeTableView* view_;

};

OLIVE_NAMESPACE_EXIT

#endif // NODETABLEWIDGET_H
