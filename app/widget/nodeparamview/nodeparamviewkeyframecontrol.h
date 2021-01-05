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

#ifndef NODEPARAMVIEWKEYFRAMECONTROL_H
#define NODEPARAMVIEWKEYFRAMECONTROL_H

#include <QPushButton>
#include <QWidget>

#include "node/input.h"
#include "widget/timetarget/timetarget.h"

namespace olive {

class NodeParamViewKeyframeControl : public QWidget, public TimeTargetObject
{
  Q_OBJECT
public:
  NodeParamViewKeyframeControl(bool right_align = true, QWidget* parent = nullptr);

  NodeInput* GetConnectedInput() const
  {
    return input_;
  }

  int GetConnectedElement() const
  {
    return element_;
  }

  void SetInput(NodeInput* input, int element);

  void SetTime(const rational& time);

signals:
  void RequestSetTime(const rational& time);

private:
  QPushButton* CreateNewToolButton(const QIcon &icon) const;

  void SetButtonsEnabled(bool e);

  rational GetCurrentTimeAsNodeTime() const;

  rational ConvertToViewerTime(const rational& r) const;

  QPushButton* prev_key_btn_;
  QPushButton* toggle_key_btn_;
  QPushButton* next_key_btn_;
  QPushButton* enable_key_btn_;

  NodeInput* input_;
  int element_;

  rational time_;

private slots:
  void ShowButtonsFromKeyframeEnable(bool e);

  void ToggleKeyframe(bool e);

  void UpdateState();

  void GoToPreviousKey();

  void GoToNextKey();

  void KeyframeEnableChanged(bool e);

};

}

#endif // NODEPARAMVIEWKEYFRAMECONTROL_H
