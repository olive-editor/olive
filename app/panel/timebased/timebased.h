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

#ifndef TIMEBASEDPANEL_H
#define TIMEBASEDPANEL_H

#include "widget/panel/panel.h"
#include "widget/timebased/timebased.h"

OLIVE_NAMESPACE_ENTER

class TimeBasedPanel : public PanelWidget
{
  Q_OBJECT
public:
  TimeBasedPanel(QWidget *parent = nullptr);

  void ConnectViewerNode(ViewerOutput* node);

  void DisconnectViewerNode();

  rational GetTime();

  ViewerOutput* GetConnectedViewer() const;

  TimeRuler* ruler() const;

  virtual void ZoomIn() override;

  virtual void ZoomOut() override;

  virtual void GoToStart() override;

  virtual void PrevFrame() override;

  virtual void NextFrame() override;

  virtual void GoToEnd() override;

  virtual void GoToPrevCut() override;

  virtual void GoToNextCut() override;

  virtual void PlayPause() override;

  virtual void ShuttleLeft() override;

  virtual void ShuttleStop() override;

  virtual void ShuttleRight() override;

  virtual void SetIn() override;

  virtual void SetOut() override;

  virtual void ResetIn() override;

  virtual void ResetOut() override;

  virtual void ClearInOut() override;

  virtual void SetMarker() override;

public slots:
  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t& timestamp);

signals:
  void TimeChanged(const int64_t& time);

  void TimebaseChanged(const rational& timebase);

  void PlayPauseRequested();

  void ShuttleLeftRequested();

  void ShuttleStopRequested();

  void ShuttleRightRequested();

protected:
  TimeBasedWidget* GetTimeBasedWidget() const;

  void SetTimeBasedWidget(TimeBasedWidget* widget);

  virtual void Retranslate() override;

private:
  TimeBasedWidget* widget_;

};

OLIVE_NAMESPACE_EXIT

#endif // TIMEBASEDPANEL_H
