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

#ifndef NODEPARAMVIEWWIDGETBRIDGE_H
#define NODEPARAMVIEWWIDGETBRIDGE_H

#include <QObject>

#include "node/input.h"
#include "node/inputdragger.h"
#include "widget/slider/sliderbase.h"
#include "widget/timetarget/timetarget.h"

OLIVE_NAMESPACE_ENTER

class NodeParamViewWidgetBridge : public QObject, public TimeTargetObject
{
  Q_OBJECT
public:
  NodeParamViewWidgetBridge(NodeInput* input, QObject* parent);

  void SetTime(const rational& time);

  const QList<QWidget*>& widgets() const;

private:
  void CreateWidgets();

  void SetInputValue(const QVariant& value, int track);

  void SetInputValueInternal(const QVariant& value, int track, QUndoCommand* command);

  void ProcessSlider(SliderBase* slider, const QVariant& value);

  void CreateSliders(int count);

  void UpdateWidgetValues();

  rational GetCurrentTimeAsNodeTime() const;

  NodeInput* input_;

  QList<QWidget*> widgets_;

  rational time_;

  NodeInputDragger dragger_;

private slots:
  void WidgetCallback();

  void InputValueChanged(const TimeRange& range);

  void PropertyChanged(const QString& key, const QVariant& value);

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
