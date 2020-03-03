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

#include "timeline.h"

#include "panel/panelmanager.h"
#include "panel/project/project.h"

TimelinePanel::TimelinePanel(QWidget *parent) :
  TimeBasedPanel(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("TimelinePanel");

  TimelineWidget* tw = new TimelineWidget();
  SetTimeBasedWidget(tw);

  Retranslate();

  connect(tw, &TimelineWidget::SelectionChanged, this, &TimelinePanel::SelectionChanged);
}

void TimelinePanel::Clear()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->Clear();
}

void TimelinePanel::SplitAtPlayhead()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->SplitAtPlayhead();
}

void TimelinePanel::SelectAll()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->SelectAll();
}

void TimelinePanel::DeselectAll()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DeselectAll();
}

void TimelinePanel::RippleToIn()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->RippleToIn();
}

void TimelinePanel::RippleToOut()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->RippleToOut();
}

void TimelinePanel::EditToIn()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->EditToIn();
}

void TimelinePanel::EditToOut()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->EditToOut();
}

void TimelinePanel::DeleteSelected()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DeleteSelected();
}

void TimelinePanel::IncreaseTrackHeight()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->IncreaseTrackHeight();
}

void TimelinePanel::DecreaseTrackHeight()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DecreaseTrackHeight();
}

void TimelinePanel::Insert()
{
  ProjectPanel* project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();

  if (project_panel) {
    InsertFootageAtPlayhead(project_panel->GetSelectedFootage());
  }
}

void TimelinePanel::Overwrite()
{
  ProjectPanel* project_panel = PanelManager::instance()->MostRecentlyFocused<ProjectPanel>();

  if (project_panel) {
    OverwriteFootageAtPlayhead(project_panel->GetSelectedFootage());
  }
}

void TimelinePanel::InsertFootageAtPlayhead(const QList<Footage *> &footage)
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->InsertFootageAtPlayhead(footage);
}

void TimelinePanel::OverwriteFootageAtPlayhead(const QList<Footage *> &footage)
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->OverwriteFootageAtPlayhead(footage);
}

void TimelinePanel::Retranslate()
{
  TimeBasedPanel::Retranslate();

  SetTitle(tr("Timeline"));
}
