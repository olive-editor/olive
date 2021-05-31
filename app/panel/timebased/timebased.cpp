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

#include "timebased.h"

namespace olive {

TimeBasedPanel::TimeBasedPanel(const QString &object_name, QWidget *parent) :
  PanelWidget(object_name, parent),
  widget_(nullptr),
  show_and_raise_on_connect_(false)
{
}

rational TimeBasedPanel::GetTime()
{
  return widget_->GetTime();
}

const rational& TimeBasedPanel::timebase()
{
  return widget_->timebase();
}

void TimeBasedPanel::GoToStart()
{
  widget_->GoToStart();
}

void TimeBasedPanel::PrevFrame()
{
  widget_->PrevFrame();
}

void TimeBasedPanel::NextFrame()
{
  widget_->NextFrame();
}

void TimeBasedPanel::GoToEnd()
{
  widget_->GoToEnd();
}

void TimeBasedPanel::ZoomIn()
{
  widget_->ZoomIn();
}

void TimeBasedPanel::ZoomOut()
{
  widget_->ZoomOut();
}

void TimeBasedPanel::SetTimebase(const rational &timebase)
{
  widget_->SetTimebase(timebase);
}

void TimeBasedPanel::SetTimestamp(const int64_t &timestamp)
{
  widget_->SetTimestamp(timestamp);
}

void TimeBasedPanel::GoToPrevCut()
{
  widget_->GoToPrevCut();
}

void TimeBasedPanel::GoToNextCut()
{
  widget_->GoToNextCut();
}

void TimeBasedPanel::PlayPause()
{
  emit PlayPauseRequested();
}

void TimeBasedPanel::PlayInToOut()
{
  emit PlayInToOutRequested();
}

void TimeBasedPanel::ShuttleLeft()
{
  emit ShuttleLeftRequested();
}

void TimeBasedPanel::ShuttleStop()
{
  emit ShuttleStopRequested();
}

void TimeBasedPanel::ShuttleRight()
{
  emit ShuttleRightRequested();
}

void TimeBasedPanel::ConnectViewerNode(ViewerOutput *node)
{
  widget_->ConnectViewerNode(node);
}

void TimeBasedPanel::SetTimeBasedWidget(TimeBasedWidget *widget)
{
  if (widget_) {
    disconnect(widget_, &TimeBasedWidget::TimeChanged, this, &TimeBasedPanel::TimeChanged);
    disconnect(widget_, &TimeBasedWidget::TimebaseChanged, this, &TimeBasedPanel::TimebaseChanged);
    disconnect(widget_, &TimeBasedWidget::ConnectedNodeChanged, this, &TimeBasedPanel::ConnectedNodeChanged);
  }

  widget_ = widget;

  if (widget_) {
    connect(widget_, &TimeBasedWidget::TimeChanged, this, &TimeBasedPanel::TimeChanged);
    connect(widget_, &TimeBasedWidget::TimebaseChanged, this, &TimeBasedPanel::TimebaseChanged);
    connect(widget_, &TimeBasedWidget::ConnectedNodeChanged, this, &TimeBasedPanel::ConnectedNodeChanged);
  }

  SetWidgetWithPadding(widget_);
}

void TimeBasedPanel::Retranslate()
{
  if (GetTimeBasedWidget()->GetConnectedNode()) {
    SetSubtitle(GetTimeBasedWidget()->GetConnectedNode()->GetLabel());
  } else {
    SetSubtitle(tr("(none)"));
  }
}

void TimeBasedPanel::ConnectedNodeChanged(ViewerOutput *old, ViewerOutput *now)
{
  if (old) {
    disconnect(old, &ViewerOutput::LabelChanged, this, &TimeBasedPanel::SetSubtitle);
  }

  if (now) {
    connect(now, &ViewerOutput::LabelChanged, this, &TimeBasedPanel::SetSubtitle);

    if (show_and_raise_on_connect_) {
      this->show();
      this->raise();
    }
  }

  // Update strings
  Retranslate();
}

void TimeBasedPanel::SetIn()
{
  GetTimeBasedWidget()->SetInAtPlayhead();
}

void TimeBasedPanel::SetOut()
{
  GetTimeBasedWidget()->SetOutAtPlayhead();
}

void TimeBasedPanel::ResetIn()
{
  GetTimeBasedWidget()->ResetIn();
}

void TimeBasedPanel::ResetOut()
{
  GetTimeBasedWidget()->ResetOut();
}

void TimeBasedPanel::ClearInOut()
{
  GetTimeBasedWidget()->ClearInOutPoints();
}

void TimeBasedPanel::SetMarker()
{
  GetTimeBasedWidget()->SetMarker();
}

void TimeBasedPanel::ToggleShowAll()
{
  GetTimeBasedWidget()->ToggleShowAll();
}

void TimeBasedPanel::GoToIn()
{
  GetTimeBasedWidget()->GoToIn();
}

void TimeBasedPanel::GoToOut()
{
  GetTimeBasedWidget()->GoToOut();
}

}
