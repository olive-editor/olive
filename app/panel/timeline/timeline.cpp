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

#include "timeline.h"

#include "panel/panelmanager.h"
#include "panel/project/footagemanagementpanel.h"

namespace olive {

TimelinePanel::TimelinePanel(QWidget *parent) :
  TimeBasedPanel(QStringLiteral("TimelinePanel"), parent)
{
  TimelineWidget* tw = new TimelineWidget();
  SetTimeBasedWidget(tw);

  Retranslate();

  connect(tw, &TimelineWidget::BlocksSelected, this, &TimelinePanel::BlocksSelected);
  connect(tw, &TimelineWidget::BlocksDeselected, this, &TimelinePanel::BlocksDeselected);
}

void TimelinePanel::Clear()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->Clear();
}

void TimelinePanel::SplitAtPlayhead()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->SplitAtPlayhead();
}

QByteArray TimelinePanel::SaveSplitterState() const
{
  return static_cast<TimelineWidget*>(GetTimeBasedWidget())->SaveSplitterState();
}

void TimelinePanel::RestoreSplitterState(const QByteArray &state)
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->RestoreSplitterState(state);
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
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DeleteSelected(false);
}

void TimelinePanel::RippleDelete()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DeleteSelected(true);
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
  FootageManagementPanel* project_panel = PanelManager::instance()->MostRecentlyFocused<FootageManagementPanel>();

  if (project_panel) {
    InsertFootageAtPlayhead(project_panel->GetSelectedFootage());
  }
}

void TimelinePanel::Overwrite()
{
  FootageManagementPanel* project_panel = PanelManager::instance()->MostRecentlyFocused<FootageManagementPanel>();

  if (project_panel) {
    OverwriteFootageAtPlayhead(project_panel->GetSelectedFootage());
  }
}

void TimelinePanel::ToggleLinks()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->ToggleLinksOnSelected();
}

void TimelinePanel::CutSelected()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->CopySelected(true);
}

void TimelinePanel::CopySelected()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->CopySelected(false);
}

void TimelinePanel::Paste()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->Paste(false);
}

void TimelinePanel::PasteInsert()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->Paste(true);
}

void TimelinePanel::DeleteInToOut()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DeleteInToOut(false);
}

void TimelinePanel::RippleDeleteInToOut()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->DeleteInToOut(true);
}

void TimelinePanel::ToggleSelectedEnabled()
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->ToggleSelectedEnabled();
}

void TimelinePanel::SetColorLabel(int index)
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->SetColorLabel(index);
}

void TimelinePanel::InsertFootageAtPlayhead(const QVector<ViewerOutput *> &footage)
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->InsertFootageAtPlayhead(footage);
}

void TimelinePanel::OverwriteFootageAtPlayhead(const QVector<ViewerOutput *> &footage)
{
  static_cast<TimelineWidget*>(GetTimeBasedWidget())->OverwriteFootageAtPlayhead(footage);
}

void TimelinePanel::Retranslate()
{
  TimeBasedPanel::Retranslate();

  SetTitle(tr("Timeline"));
}

}
