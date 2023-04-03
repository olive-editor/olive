/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include "widget/nodeparamview/paramwidget/abstractparamwidget.h"
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

  const std::vector<QWidget*>& widgets() const
  {
    return widget_->GetWidgets();
  }

  bool has_widgets() const { return widget_; }

  // Set the timebase of certain Timebased widgets
  void SetTimebase(const rational& timebase);

signals:
  void ArrayWidgetDoubleClicked();

  void WidgetsRecreated(const NodeInput& input);

  void RequestEditTextInViewer();

protected:
  virtual void TimeTargetDisconnectEvent(ViewerOutput *v) override;
  virtual void TimeTargetConnectEvent(ViewerOutput *v) override;

private:
  void CreateWidgets();

  void SetInputValue(const value_t::component_t &value, size_t track);

  void SetInputValueInternal(const value_t::component_t &value, size_t track, MultiUndoCommand *command, bool insert_on_all_tracks_if_no_key);

  void SetProperty(const QString &key, const value_t &value);

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

  QString GetCommandName() const;

  type_t GetDataType() const
  {
    return GetOuterInput().GetDataType();
  }

  size_t GetChannelCount() const
  {
    return GetOuterInput().GetChannelCount();
  }

  void UpdateProperties();

  QVector<NodeInput> input_hierarchy_;

  AbstractParamWidget *widget_;

  NodeInputDragger dragger_;

  NodeParamViewScrollBlocker scroll_filter_;

private slots:
  void InputValueChanged(const NodeInput& input, const TimeRange& range);

  void InputDataTypeChanged(const QString& input, type_t type);

  void PropertyChanged(const QString &input, const QString &key, const value_t &value);

  void ProcessSlider(NumericSliderBase* slider, size_t track);

  void ValueChanged(size_t track, const value_t::component_t &val);

};

}

#endif // NODEPARAMVIEWWIDGETBRIDGE_H
