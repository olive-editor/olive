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

#ifndef CURVEPANEL_H
#define CURVEPANEL_H

#include "panel/timebased/timebased.h"
#include "widget/curvewidget/curvewidget.h"

OLIVE_NAMESPACE_ENTER

class CurvePanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  CurvePanel(QWidget* parent);

  NodeInput* GetInput() const;

public slots:
  void SetInput(NodeInput* input);

  void SetTimeTarget(Node* target);

  virtual void IncreaseTrackHeight() override;

  virtual void DecreaseTrackHeight() override;

protected:
  virtual void Retranslate() override;

};

OLIVE_NAMESPACE_EXIT

#endif // CURVEPANEL_H
