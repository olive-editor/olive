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

#ifndef NODETABLEPANEL_H
#define NODETABLEPANEL_H

#include "panel/timebased/timebased.h"
#include "widget/nodetableview/nodetablewidget.h"

namespace olive {

class NodeTablePanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  NodeTablePanel(QWidget* parent);

public slots:
  void SelectNodes(const QVector<Node*>& nodes)
  {
    static_cast<NodeTableWidget*>(GetTimeBasedWidget())->SelectNodes(nodes);
  }

  void DeselectNodes(const QVector<Node*>& nodes)
  {
    static_cast<NodeTableWidget*>(GetTimeBasedWidget())->DeselectNodes(nodes);
  }

private:
  virtual void Retranslate() override;

};

}

#endif // NODETABLEPANEL_H
