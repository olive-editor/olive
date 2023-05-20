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

#include "nodeparamviewwidgetbridge.h"

#include <QCheckBox>
#include <QFontComboBox>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "common/qtutils.h"
#include "core.h"
#include "node/group/group.h"
#include "node/node.h"
#include "node/nodeundo.h"
#include "node/project/sequence/sequence.h"
#include "nodeparamviewarraywidget.h"
#include "nodeparamviewtextedit.h"
#include "undo/undostack.h"
#include "widget/colorbutton/colorbutton.h"
#include "widget/filefield/filefield.h"
#include "widget/nodeparamview/paramwidget/arrayparamwidget.h"
#include "widget/nodeparamview/paramwidget/bezierparamwidget.h"
#include "widget/nodeparamview/paramwidget/boolparamwidget.h"
#include "widget/nodeparamview/paramwidget/colorparamwidget.h"
#include "widget/nodeparamview/paramwidget/comboparamwidget.h"
#include "widget/nodeparamview/paramwidget/fileparamwidget.h"
#include "widget/nodeparamview/paramwidget/floatsliderparamwidget.h"
#include "widget/nodeparamview/paramwidget/fontparamwidget.h"
#include "widget/nodeparamview/paramwidget/integersliderparamwidget.h"
#include "widget/nodeparamview/paramwidget/rationalsliderparamwidget.h"
#include "widget/nodeparamview/paramwidget/textparamwidget.h"

namespace olive {

NodeParamViewWidgetBridge::NodeParamViewWidgetBridge(NodeInput input, QObject *parent) :
  QObject(parent),
  widget_(nullptr)
{
  do {
    input_hierarchy_.append(input);

    connect(input.node(), &Node::ValueChanged, this, &NodeParamViewWidgetBridge::InputValueChanged);
    connect(input.node(), &Node::InputPropertyChanged, this, &NodeParamViewWidgetBridge::PropertyChanged);
    connect(input.node(), &Node::InputDataTypeChanged, this, &NodeParamViewWidgetBridge::InputDataTypeChanged);
  } while (NodeGroup::GetInner(&input));

  CreateWidgets();
}

void NodeParamViewWidgetBridge::CreateWidgets()
{
  QWidget *parent = dynamic_cast<QWidget*>(this->parent());

  if (GetInnerInput().IsArray() && GetInnerInput().element() == -1) {

    widget_ = new ArrayParamWidget(GetInnerInput().node(), GetInnerInput().input(), parent);
    connect(static_cast<ArrayParamWidget*>(widget_), &ArrayParamWidget::DoubleClicked, this, &NodeParamViewWidgetBridge::ArrayWidgetDoubleClicked);

  } else {

    // We assume the first data type is the "primary" type
    type_t t = GetDataType();
    QString type = GetOuterInput().GetProperty(QStringLiteral("subtype")).toString();

    if (t == TYPE_INTEGER) {
      if (type == QStringLiteral("bool")) {
        widget_ = new BoolParamWidget(this);
      } else if (type == QStringLiteral("combo")) {
        widget_ = new ComboParamWidget(this);
      } else {
        widget_ = new IntegerSliderParamWidget(this);
      }
    } else if (t == TYPE_RATIONAL) {
      widget_ = new RationalSliderParamWidget(this);
    } else if (t == TYPE_DOUBLE) {
      if (type == QStringLiteral("color")) {
        widget_ = new ColorParamWidget(this);
      } else if (type == QStringLiteral("bezier")) {
        widget_ = new BezierParamWidget(this);
      } else {
        widget_ = new FloatSliderParamWidget(this);
      }
    } else if (t == TYPE_STRING) {
      if (type == QStringLiteral("file")) {
        widget_ = new FileParamWidget(this);
      } else if (type == QStringLiteral("font")) {
        widget_ = new FontParamWidget(this);
      } else {
        widget_ = new TextParamWidget(this);
        connect(static_cast<TextParamWidget*>(widget_), &TextParamWidget::RequestEditInViewer, this, &NodeParamViewWidgetBridge::RequestEditTextInViewer);
      }
    }

  }

  if (widget_) {
    // Whatever we created, initialize it
    widget_->Initialize(parent, GetChannelCount());

    value_t def = GetInnerInput().GetDefaultValue();
    if (def.isValid()) {
      widget_->SetDefaultValue(def);
    }

    // Install event filter to disable widgets picking up scroll events
    foreach (QWidget* w, widget_->GetWidgets()) {
      w->installEventFilter(&scroll_filter_);
    }

    // Connect signals
    connect(widget_, &AbstractParamWidget::SliderDragged, this, &NodeParamViewWidgetBridge::ProcessSlider);
    connect(widget_, &AbstractParamWidget::ChannelValueChanged, this, &NodeParamViewWidgetBridge::ValueChanged);

    // Check all properties
    UpdateProperties();

    UpdateWidgetValues();
  }
}

void NodeParamViewWidgetBridge::SetInputValue(const value_t::component_t &value, size_t track)
{
  MultiUndoCommand* command = new MultiUndoCommand();

  SetInputValueInternal(value, track, command, true);

  Core::instance()->undo_stack()->push(command, GetCommandName());
}

void NodeParamViewWidgetBridge::SetInputValueInternal(const value_t::component_t &value, size_t track, MultiUndoCommand *command, bool insert_on_all_tracks_if_no_key)
{
  Node::SetValueAtTime(GetInnerInput(), GetCurrentTimeAsNodeTime(), value, track, command, insert_on_all_tracks_if_no_key);
}

void NodeParamViewWidgetBridge::ProcessSlider(NumericSliderBase *slider, size_t slider_track)
{
  value_t::component_t value = slider->GetValueInternal().at(0);

  if (slider->IsDragging()) {

    // While we're dragging, we block the input's normal signalling and create our own
    if (!dragger_.IsStarted()) {
      rational node_time = GetCurrentTimeAsNodeTime();

      dragger_.Start(NodeKeyframeTrackReference(GetInnerInput(), slider_track), node_time);
    }

    dragger_.Drag(value);

  } else if (dragger_.IsStarted()) {

    MultiUndoCommand *command = new MultiUndoCommand();
    dragger_.End(command);
    Core::instance()->undo_stack()->push(command, GetCommandName());

  } else {

    // No drag was involved, we can just push the value
    SetInputValue(value, slider_track);

  }
}

void NodeParamViewWidgetBridge::ValueChanged(size_t track, const value_t::component_t &val)
{
  SetInputValue(val, track);
}

void NodeParamViewWidgetBridge::UpdateWidgetValues()
{
  if (GetInnerInput().IsArray() && GetInnerInput().element() == -1) {
    return;
  }

  if (widget_) {
    rational node_time;
    if (GetInnerInput().IsKeyframing()) {
      node_time = GetCurrentTimeAsNodeTime();
    }

    widget_->SetValue(GetInnerInput().GetValueAtTime(node_time));
  }
}

rational NodeParamViewWidgetBridge::GetCurrentTimeAsNodeTime() const
{
  if (GetTimeTarget()) {
    return GetAdjustedTime(GetTimeTarget(), GetInnerInput().node(), GetTimeTarget()->GetPlayhead(), Node::kTransformTowardsInput);
  } else {
    return 0;
  }
}

QString NodeParamViewWidgetBridge::GetCommandName() const
{
  NodeInput i = GetInnerInput();
  return tr("Edited Value Of %1 - %2").arg(i.node()->GetLabelAndName(), i.node()->GetInputName(i.input()));
}

void NodeParamViewWidgetBridge::SetTimebase(const rational& timebase)
{
  if (widget_) {
    widget_->SetTimebase(timebase);
  }
}

void NodeParamViewWidgetBridge::TimeTargetDisconnectEvent(ViewerOutput *v)
{
  disconnect(v, &ViewerOutput::PlayheadChanged, this, &NodeParamViewWidgetBridge::UpdateWidgetValues);
}

void NodeParamViewWidgetBridge::TimeTargetConnectEvent(ViewerOutput *v)
{
  connect(v, &ViewerOutput::PlayheadChanged, this, &NodeParamViewWidgetBridge::UpdateWidgetValues);
}

void NodeParamViewWidgetBridge::InputValueChanged(const NodeInput &input, const TimeRange &range)
{
  if (GetTimeTarget()
      && GetInnerInput() == input
      && !dragger_.IsStarted()
      && range.in() <= GetTimeTarget()->GetPlayhead() && range.out() >= GetTimeTarget()->GetPlayhead()) {
    // We'll need to update the widgets because the values have changed on our current time
    UpdateWidgetValues();
  }
}

void NodeParamViewWidgetBridge::SetProperty(const QString &key, const value_t &value)
{
  if (widget_) {
    widget_->SetProperty(key, value);
  }
}

void NodeParamViewWidgetBridge::InputDataTypeChanged(const QString &input, type_t type)
{
  if (sender() == GetOuterInput().node() && input == GetOuterInput().input()) {
    // Delete all widgets
    delete widget_;

    // Create new widgets
    CreateWidgets();

    // Signal that widgets are new
    emit WidgetsRecreated(GetOuterInput());
  }
}

void NodeParamViewWidgetBridge::PropertyChanged(const QString &input, const QString &key, const value_t &value)
{
  bool found = false;

  for (auto it=input_hierarchy_.cbegin(); it!=input_hierarchy_.cend(); it++) {
    if (it->input() == input) {
      found = true;
      break;
    }
  }

  if (found) {
    UpdateProperties();
  }
}

void NodeParamViewWidgetBridge::UpdateProperties()
{
  // Set properties from the last entry (the innermost input) to the first (the outermost)
  for (auto it=input_hierarchy_.crbegin(); it!=input_hierarchy_.crend(); it++) {
    auto input_properties = it->node()->GetInputProperties(it->input());
    for (auto jt=input_properties.cbegin(); jt!=input_properties.cend(); jt++) {
      SetProperty(jt.key(), jt.value());
    }
  }
}

bool NodeParamViewScrollBlocker::eventFilter(QObject *watched, QEvent *event)
{
  Q_UNUSED(watched)

  if (event->type() == QEvent::Wheel) {
    // Block this event
    return true;
  }

  return false;
}

}
