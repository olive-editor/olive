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

#ifndef NODEPARAMVIEWWIDGETBRIDGE_H
#define NODEPARAMVIEWWIDGETBRIDGE_H

#include <QObject>

#include "node/inputdragger.h"
#include "widget/slider/base/numericsliderbase.h"
#include "widget/timetarget/timetarget.h"

namespace olive {

class NodeParamViewScrollBlocker : public QObject
{
  Q_OBJECT
public:
  virtual bool eventFilter(QObject* watched, QEvent* event) override;
};

class NodeParamViewWidgetBridge : public QObject, public TimeTargetObject
{
  Q_OBJECT
public:
  NodeParamViewWidgetBridge(NodeInput input, QObject* parent);

  void SetTime(const rational& time);

  const QVector<QWidget*>& widgets() const
  {
    return widgets_;
  }

  // Set the timebase of certain Timebased widgets
  void SetTimebase(const rational& timebase);

signals:
  void ArrayWidgetDoubleClicked();

  void WidgetsRecreated(const NodeInput& input);

private:
  void CreateWidgets();

  void SetInputValue(const QVariant& value, int track);

  void SetInputValueInternal(const QVariant& value, int track, MultiUndoCommand *command, bool insert_on_all_tracks_if_no_key);

  void ProcessSlider(NumericSliderBase* slider, int slider_track, const QVariant& value);
  void ProcessSlider(NumericSliderBase* slider, const QVariant& value)
  {
    ProcessSlider(slider, widgets_.indexOf(slider), value);
  }

  void SetProperty(const QString &key, const QVariant &value);

  template <typename T>
  void CreateSliders(int count);

  void UpdateWidgetValues();

  rational GetCurrentTimeAsNodeTime() const;

  const NodeInput &GetOuterInput() const
  {
    return input_hierarchy_.first();
  }

  const NodeInput &GetInnerInput() const
  {
    return input_hierarchy_.last();
  }

  NodeValue::Type GetDataType() const
  {
    return GetOuterInput().GetDataType();
  }

  void UpdateProperties();

  QVector<NodeInput> input_hierarchy_;

  QVector<QWidget*> widgets_;

  rational time_;

  NodeInputDragger dragger_;

  NodeParamViewScrollBlocker scroll_filter_;

private slots:
  void WidgetCallback();

  void InputValueChanged(const NodeInput& input, const TimeRange& range);

  void InputDataTypeChanged(const QString& input, NodeValue::Type type);

  void PropertyChanged(const QString &input, const QString &key, const QVariant &value);

};

}

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
