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

TimelinePanel::TimelinePanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("TimelinePanel");

  timeline_widget_ = new TimelineWidget();
  setWidget(timeline_widget_);

  connect(timeline_widget_, SIGNAL(TimeChanged(const int64_t&)), this, SIGNAL(TimeChanged(const int64_t&)));

  Retranslate();
}

void TimelinePanel::Clear()
{
  timeline_widget_->Clear();
}

void TimelinePanel::SetTimebase(const rational &timebase)
{
  timeline_widget_->SetTimebase(timebase);
}

void TimelinePanel::SetTime(const int64_t &timestamp)
{
  timeline_widget_->SetTime(timestamp);
}

void TimelinePanel::ConnectTimelineNode(TimelineOutput *node)
{
  timeline_widget_->ConnectTimelineNode(node);
}

void TimelinePanel::DisconnectTimelineNode()
{
  timeline_widget_->DisconnectTimelineNode();
}

void TimelinePanel::SplitAtPlayhead()
{
  timeline_widget_->SplitAtPlayhead();
}

void TimelinePanel::ZoomIn()
{
  timeline_widget_->ZoomIn();
}

void TimelinePanel::ZoomOut()
{
  timeline_widget_->ZoomOut();
}

void TimelinePanel::SelectAll()
{
  timeline_widget_->SelectAll();
}

void TimelinePanel::DeselectAll()
{
  timeline_widget_->DeselectAll();
}

void TimelinePanel::RippleToIn()
{
  timeline_widget_->RippleToIn();
}

void TimelinePanel::RippleToOut()
{
  timeline_widget_->RippleToOut();
}

void TimelinePanel::EditToIn()
{
  timeline_widget_->EditToIn();
}

void TimelinePanel::EditToOut()
{
  timeline_widget_->EditToOut();
}

void TimelinePanel::GoToPrevCut()
{
  timeline_widget_->GoToPrevCut();
}

void TimelinePanel::GoToNextCut()
{
  timeline_widget_->GoToNextCut();
}

void TimelinePanel::DeleteSelected()
{
  timeline_widget_->DeleteSelected();
}

void TimelinePanel::IncreaseTrackHeight()
{
  timeline_widget_->IncreaseTrackHeight();
}

void TimelinePanel::DecreaseTrackHeight()
{
  timeline_widget_->DecreaseTrackHeight();
}

void TimelinePanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  PanelWidget::changeEvent(e);
}

void TimelinePanel::Retranslate()
{
  SetTitle(tr("Timeline"));
  SetSubtitle(tr("(none)"));
}
