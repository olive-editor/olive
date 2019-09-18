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

#ifndef TIMELINE_PANEL_H
#define TIMELINE_PANEL_H

#include "widget/panel/panel.h"
#include "widget/timelineview/timelineview.h"
#include "widget/timeruler/timeruler.h"

class TimelinePanel : public PanelWidget
{
  Q_OBJECT
public:
  TimelinePanel(QWidget* parent);

  void Clear();

  void SetTimebase(const rational& timebase);

  TimelineView* view();

  virtual void ZoomIn() override;

  virtual void ZoomOut() override;

protected:
  virtual void changeEvent(QEvent* e) override;

signals:

private:
  void Retranslate();

  TimelineView* view_;

  TimeRuler* ruler_;

  double scale_;

private slots:
  void SetScale(double scale);
};

#endif // TIMELINE_PANEL_H
