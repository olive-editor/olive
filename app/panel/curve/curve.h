/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef CURVEPANEL_H
#define CURVEPANEL_H

#include "panel/timebased/timebased.h"
#include "widget/curvewidget/curvewidget.h"

namespace olive {

class CurvePanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  CurvePanel(QWidget* parent);

  virtual void DeleteSelected() override;

  virtual void SelectAll() override;

  virtual void DeselectAll() override;

public slots:
  void SetNode(Node *node)
  {
    // Convert single pointer to either an empty vector or a vector of one
    QVector<Node *> nodes;

    if (node) {
      nodes.append(node);
    }

    SetNodes(nodes);
  }

  void SetNodes(const QVector<Node *> &nodes);

  virtual void IncreaseTrackHeight() override;

  virtual void DecreaseTrackHeight() override;

protected:
  virtual void Retranslate() override;

};

}

#endif // CURVEPANEL_H
