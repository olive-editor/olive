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

#include "nodeparamviewwidgetbridge.h"

#include <QCheckBox>
#include <QFontComboBox>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "core.h"
#include "node/node.h"
#include "nodeparamviewarraywidget.h"
#include "nodeparamviewrichtext.h"
#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"
#include "undo/undostack.h"
#include "widget/colorbutton/colorbutton.h"
#include "widget/footagecombobox/footagecombobox.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/integerslider.h"

OLIVE_NAMESPACE_ENTER

NodeParamViewWidgetBridge::NodeParamViewWidgetBridge(NodeInput *input, QObject *parent) :
  QObject(parent),
  input_(input)
{
  CreateWidgets();

  connect(input_, &NodeInput::ValueChanged, this, &NodeParamViewWidgetBridge::InputValueChanged);
  connect(input_, &NodeInput::PropertyChanged, this, &NodeParamViewWidgetBridge::PropertyChanged);
}

void NodeParamViewWidgetBridge::SetTime(const rational &time)
{
  time_ = time;

  if (input_) {
    UpdateWidgetValues();
  }
}

const QList<QWidget *> &NodeParamViewWidgetBridge::widgets() const
{
  return widgets_;
}

void NodeParamViewWidgetBridge::CreateWidgets()
{

  if (input_->IsArray()) {

    NodeParamViewArrayWidget* w = new NodeParamViewArrayWidget(static_cast<NodeInputArray*>(input_));
    widgets_.append(w);

  } else {

    // We assume the first data type is the "primary" type
    switch (input_->data_type()) {
    // None of these inputs have applicable UI widgets
    case NodeParam::kNone:
    case NodeParam::kAny:
    case NodeParam::kTexture:
    case NodeParam::kMatrix:
    case NodeParam::kRational:
    case NodeParam::kSamples:
    case NodeParam::kDecimal:
    case NodeParam::kNumber:
    case NodeParam::kString:
    case NodeParam::kBuffer:
    case NodeParam::kVector:
    case NodeParam::kShaderJob:
    case NodeParam::kSampleJob:
    case NodeParam::kGenerateJob:
      break;
    case NodeParam::kInt:
    {
      IntegerSlider* slider = new IntegerSlider();
      slider->SetDefaultValue(input_->GetDefaultValue());
      slider->SetLadderElementCount(2);
      widgets_.append(slider);
      connect(slider, &IntegerSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeParam::kFloat:
    {
      CreateSliders(1);
      break;
    }
    case NodeParam::kVec2:
    {
      CreateSliders(2);
      break;
    }
    case NodeParam::kVec3:
    {
      CreateSliders(3);
      break;
    }
    case NodeParam::kVec4:
    {
      CreateSliders(4);
      break;
    }
    case NodeParam::kCombo:
    {
      QComboBox* combobox = new QComboBox();

      QStringList items = input_->get_combobox_strings();
      foreach (const QString& s, items) {
        combobox->addItem(s);
      }

      widgets_.append(combobox);
      connect(combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeParam::kFile:
      // FIXME: File selector
      break;
    case NodeParam::kColor:
    {
      // NOTE: Very convoluted way to get back to the project's color manager
      ColorButton* color_button = new ColorButton(static_cast<Sequence*>(input_->parentNode()->parent())->project()->color_manager());
      widgets_.append(color_button);
      connect(color_button, &ColorButton::ColorChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeParam::kText:
    {
      NodeParamViewRichText* line_edit = new NodeParamViewRichText();
      widgets_.append(line_edit);
      connect(line_edit, &NodeParamViewRichText::textEdited, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeParam::kBoolean:
    {
      QCheckBox* check_box = new QCheckBox();
      widgets_.append(check_box);
      connect(check_box, &QCheckBox::clicked, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeParam::kFont:
    {
      QFontComboBox* font_combobox = new QFontComboBox();
      widgets_.append(font_combobox);
      connect(font_combobox, &QFontComboBox::currentFontChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeParam::kFootage:
    {
      FootageComboBox* footage_combobox = new FootageComboBox();
      footage_combobox->SetRoot(static_cast<Sequence*>(input_->parentNode()->parent())->project()->root());

      connect(footage_combobox, &FootageComboBox::FootageChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

      widgets_.append(footage_combobox);

      break;
    }
    }

    // Check all properties
    QHash<QString, QVariant>::const_iterator iterator;

    for (iterator=input_->properties().begin();iterator!=input_->properties().end();iterator++) {
      PropertyChanged(iterator.key(), iterator.value());
    }

    UpdateWidgetValues();

  }
}

void NodeParamViewWidgetBridge::SetInputValue(const QVariant &value, int track)
{
  QUndoCommand* command = new QUndoCommand();

  SetInputValueInternal(value, track, command);

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewWidgetBridge::SetInputValueInternal(const QVariant &value, int track, QUndoCommand *command)
{
  rational node_time = GetCurrentTimeAsNodeTime();

  if (input_->is_keyframing()) {
    NodeKeyframePtr existing_key = input_->get_keyframe_at_time_on_track(node_time, track);

    if (existing_key) {
      new NodeParamSetKeyframeValueCommand(existing_key, value, command);
    } else {
      // No existing key, create a new one
      NodeKeyframePtr new_key = NodeKeyframe::Create(node_time,
                                                     value,
                                                     input_->get_best_keyframe_type_for_time(node_time, track),
                                                     track);

      new NodeParamInsertKeyframeCommand(input_, new_key, command);
    }
  } else {
    new NodeParamSetStandardValueCommand(input_, track, value, command);
  }
}

void NodeParamViewWidgetBridge::ProcessSlider(SliderBase *slider, const QVariant &value)
{
  rational node_time = GetCurrentTimeAsNodeTime();

  int slider_track = widgets_.indexOf(slider);

  if (slider->IsDragging()) {

    // While we're dragging, we block the input's normal signalling and create our own
    if (!dragger_.IsStarted()) {
      dragger_.Start(input_, node_time, slider_track);
    }

    dragger_.Drag(value);

    //input_->parentNode()->InvalidateVisible(input_, input_);

  } else if (dragger_.IsStarted()) {

    // We were dragging and just stopped
    dragger_.Drag(value);
    dragger_.End();

  } else {
    // No drag was involved, we can just push the value
    SetInputValue(value, slider_track);
  }
}

void NodeParamViewWidgetBridge::WidgetCallback()
{
  switch (input_->data_type()) {
  // None of these inputs have applicable UI widgets
  case NodeParam::kNone:
  case NodeParam::kAny:
  case NodeParam::kTexture:
  case NodeParam::kMatrix:
  case NodeParam::kSamples:
  case NodeParam::kRational:
  case NodeParam::kDecimal:
  case NodeParam::kNumber:
  case NodeParam::kString:
  case NodeParam::kVector:
  case NodeParam::kBuffer:
  case NodeParam::kShaderJob:
  case NodeParam::kSampleJob:
  case NodeParam::kGenerateJob:
    break;
  case NodeParam::kInt:
  {
    // Widget is a IntegerSlider
    IntegerSlider* slider = static_cast<IntegerSlider*>(sender());

    ProcessSlider(slider, QVariant::fromValue(slider->GetValue()));
    break;
  }
  case NodeParam::kFloat:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeParam::kVec2:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeParam::kVec3:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeParam::kVec4:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeParam::kFile:
    // FIXME: File selector
    break;
  case NodeParam::kColor:
  {
    // Sender is a ColorButton
    ManagedColor c = static_cast<ColorButton*>(sender())->GetColor();

    QUndoCommand* command = new QUndoCommand();

    SetInputValueInternal(c.red(), 0, command);
    SetInputValueInternal(c.green(), 1, command);
    SetInputValueInternal(c.blue(), 2, command);
    SetInputValueInternal(c.alpha(), 3, command);

    input_->blockSignals(true);
    input_->set_property(QStringLiteral("col_input"), c.color_input());
    input_->set_property(QStringLiteral("col_display"), c.color_output().display());
    input_->set_property(QStringLiteral("col_view"), c.color_output().view());
    input_->set_property(QStringLiteral("col_look"), c.color_output().look());
    input_->blockSignals(false);

    Core::instance()->undo_stack()->pushIfHasChildren(command);
    break;
  }
  case NodeParam::kText:
  {
    // Sender is a NodeParamViewRichText
    SetInputValue(static_cast<NodeParamViewRichText*>(sender())->text(), 0);
    break;
  }
  case NodeParam::kBoolean:
  {
    // Widget is a QCheckBox
    SetInputValue(static_cast<QCheckBox*>(sender())->isChecked(), 0);
    break;
  }
  case NodeParam::kFont:
  {
    // Widget is a QFontComboBox
    SetInputValue(static_cast<QFontComboBox*>(sender())->currentFont().family(), 0);
    break;
  }
  case NodeParam::kFootage:
  {
    // Widget is a FootageComboBox
    SetInputValue(QVariant::fromValue(static_cast<FootageComboBox*>(sender())->SelectedFootage()), 0);
    break;
  }
  case NodeParam::kCombo:
  {
    // Widget is a QComboBox
    SetInputValue(static_cast<QComboBox*>(widgets_.first())->currentIndex(), 0);
    break;
  }
  }
}

void NodeParamViewWidgetBridge::CreateSliders(int count)
{
  for (int i=0;i<count;i++) {
    FloatSlider* fs = new FloatSlider();
    fs->SetDefaultValue(input_->GetDefaultValueForTrack(i));
    fs->SetLadderElementCount(2);
    widgets_.append(fs);
    connect(fs, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
  }
}

void NodeParamViewWidgetBridge::UpdateWidgetValues()
{
  if (input_->IsArray()) {
    return;
  }

  rational node_time = GetCurrentTimeAsNodeTime();

  // We assume the first data type is the "primary" type
  switch (input_->data_type()) {
  // None of these inputs have applicable UI widgets
  case NodeParam::kNone:
  case NodeParam::kAny:
  case NodeParam::kTexture:
  case NodeParam::kMatrix:
  case NodeParam::kRational:
  case NodeParam::kSamples:
  case NodeParam::kDecimal:
  case NodeParam::kNumber:
  case NodeParam::kString:
  case NodeParam::kBuffer:
  case NodeParam::kShaderJob:
  case NodeParam::kSampleJob:
  case NodeParam::kGenerateJob:
  case NodeParam::kVector:
    break;
  case NodeParam::kInt:
    static_cast<IntegerSlider*>(widgets_.first())->SetValue(input_->get_value_at_time(node_time).toLongLong());
    break;
  case NodeParam::kFloat:
    static_cast<FloatSlider*>(widgets_.first())->SetValue(input_->get_value_at_time(node_time).toDouble());
    break;
  case NodeParam::kVec2:
  {
    QVector2D vec2 = input_->get_value_at_time(node_time).value<QVector2D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec2.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec2.y()));
    break;
  }
  case NodeParam::kVec3:
  {
    QVector3D vec3 = input_->get_value_at_time(node_time).value<QVector3D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec3.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec3.y()));
    static_cast<FloatSlider*>(widgets_.at(2))->SetValue(static_cast<double>(vec3.z()));
    break;
  }
  case NodeParam::kVec4:
  {
    QVector4D vec4 = input_->get_value_at_time(node_time).value<QVector4D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec4.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec4.y()));
    static_cast<FloatSlider*>(widgets_.at(2))->SetValue(static_cast<double>(vec4.z()));
    static_cast<FloatSlider*>(widgets_.at(3))->SetValue(static_cast<double>(vec4.w()));
    break;
  }
  case NodeParam::kFile:
    // FIXME: File selector
    break;
  case NodeParam::kColor:
  {
    ManagedColor mc = input_->get_value_at_time(node_time).value<Color>();

    mc.set_color_input(input_->get_property(QStringLiteral("col_input")).toString());

    QString d = input_->get_property(QStringLiteral("col_display")).toString();
    QString v = input_->get_property(QStringLiteral("col_view")).toString();
    QString l = input_->get_property(QStringLiteral("col_look")).toString();

    mc.set_color_output(ColorTransform(d, v, l));

    static_cast<ColorButton*>(widgets_.first())->SetColor(mc);
    break;
  }
  case NodeParam::kText:
  {
    NodeParamViewRichText* e = static_cast<NodeParamViewRichText*>(widgets_.first());
    e->setTextPreservingCursor(input_->get_value_at_time(node_time).toString());
    break;
  }
  case NodeParam::kBoolean:
    static_cast<QCheckBox*>(widgets_.first())->setChecked(input_->get_value_at_time(node_time).toBool());
    break;
  case NodeParam::kFont:
  {
    QFontComboBox* fc = static_cast<QFontComboBox*>(widgets_.first());
    fc->blockSignals(true);
    fc->setCurrentFont(input_->get_value_at_time(node_time).toString());
    fc->blockSignals(false);
    break;
  }
  case NodeParam::kCombo:
  {
    QComboBox* cb = static_cast<QComboBox*>(widgets_.first());
    cb->blockSignals(true);
    cb->setCurrentIndex(input_->get_value_at_time(node_time).toInt());
    cb->blockSignals(false);
    break;
  }
  case NodeParam::kFootage:
    static_cast<FootageComboBox*>(widgets_.first())->SetFootage(input_->get_value_at_time(node_time).value<StreamPtr>());
    break;
  }
}

rational NodeParamViewWidgetBridge::GetCurrentTimeAsNodeTime() const
{
  return GetAdjustedTime(GetTimeTarget(), input_->parentNode(), time_, NodeParam::kInput);
}

void NodeParamViewWidgetBridge::InputValueChanged(const TimeRange &range)
{
  if (!dragger_.IsStarted() && range.in() <= time_ && range.out() >= time_) {
    // We'll need to update the widgets because the values have changed on our current time
    UpdateWidgetValues();
  }
}

void NodeParamViewWidgetBridge::PropertyChanged(const QString &key, const QVariant &value)
{
  // Parameters for vectors only
  if (input_->data_type() & NodeParam::kVector) {
    if (key == QStringLiteral("disablex")) {
      static_cast<FloatSlider*>(widgets_.at(0))->setEnabled(!value.toBool());
    } else if (key == QStringLiteral("disabley")) {
      static_cast<FloatSlider*>(widgets_.at(1))->setEnabled(!value.toBool());
    } else if (widgets_.size() > 2 && key == QStringLiteral("disablez")) {
      static_cast<FloatSlider*>(widgets_.at(2))->setEnabled(!value.toBool());
    } else if (widgets_.size() > 3 && key == QStringLiteral("disablew")) {
      static_cast<FloatSlider*>(widgets_.at(3))->setEnabled(!value.toBool());
    }
  }

  // Parameters for integers, floats, and vectors
  if (input_->data_type() & NodeParam::kNumber || input_->data_type() & NodeParam::kVector) {
    if (key == QStringLiteral("min")) {
      switch (input_->data_type()) {
      case NodeParam::kInt:
        static_cast<IntegerSlider*>(widgets_.first())->SetMinimum(value.value<int64_t>());
        break;
      case NodeParam::kFloat:
        static_cast<FloatSlider*>(widgets_.first())->SetMinimum(value.toDouble());
        break;
      case NodeParam::kRational:
        // FIXME: Rational doesn't have a UI implementation yet
        break;
      case NodeParam::kVec2:
      {
        QVector2D min = value.value<QVector2D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMinimum(min.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMinimum(min.y());
        break;
      }
      case NodeParam::kVec3:
      {
        QVector3D min = value.value<QVector3D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMinimum(min.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMinimum(min.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetMinimum(min.z());
        break;
      }
      case NodeParam::kVec4:
      {
        QVector4D min = value.value<QVector4D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMinimum(min.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMinimum(min.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetMinimum(min.z());
        static_cast<FloatSlider*>(widgets_.at(3))->SetMinimum(min.w());
        break;
      }
      default:
        break;
      }
    } else if (key == QStringLiteral("max")) {
      switch (input_->data_type()) {
      case NodeParam::kInt:
        static_cast<IntegerSlider*>(widgets_.first())->SetMaximum(value.value<int64_t>());
        break;
      case NodeParam::kFloat:
        static_cast<FloatSlider*>(widgets_.first())->SetMaximum(value.toDouble());
        break;
      case NodeParam::kRational:
        // FIXME: Rational doesn't have a UI implementation yet
        break;
      case NodeParam::kVec2:
      {
        QVector2D max = value.value<QVector2D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMaximum(max.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMaximum(max.y());
        break;
      }
      case NodeParam::kVec3:
      {
        QVector3D max = value.value<QVector3D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMaximum(max.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMaximum(max.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetMaximum(max.z());
        break;
      }
      case NodeParam::kVec4:
      {
        QVector4D max = value.value<QVector4D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMaximum(max.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMaximum(max.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetMaximum(max.z());
        static_cast<FloatSlider*>(widgets_.at(3))->SetMaximum(max.w());
        break;
      }
      default:
        break;
      }
    }
  }

  // ComboBox strings changing
  if (input_->data_type() & NodeParam::kCombo) {
    QComboBox* cb = static_cast<QComboBox*>(widgets_.first());

    int old_index = cb->currentIndex();

    // Block the combobox changed signals since we anticipate the index will be the same and not require a re-render
    cb->blockSignals(true);

    cb->clear();

    QStringList items = input_->get_combobox_strings();
    foreach (const QString& s, items) {
      if (s.isEmpty()) {
        cb->insertSeparator(cb->count());
      } else {
        cb->addItem(s);
      }
    }

    cb->setCurrentIndex(old_index);

    cb->blockSignals(false);

    // In case the amount of items is LESS and the previous index cannot be set, NOW we trigger a re-cache since the
    // value has changed
    if (cb->currentIndex() != old_index) {
      WidgetCallback();
    }
  }

  // Parameters for floats and vectors only
  if (input_->data_type() & NodeParam::kFloat || input_->data_type() & NodeParam::kVector) {
    if (key == QStringLiteral("view")) {
      FloatSlider::DisplayType display_type;

      if (value == QStringLiteral("percent")) {
        display_type = FloatSlider::kPercentage;
      } else if (value == QStringLiteral("db")) {
        display_type = FloatSlider::kDecibel;
      } else {
        // Avoid undefined behavior
        return;
      }

      foreach (QWidget* w, widgets_) {
        static_cast<FloatSlider*>(w)->SetDisplayType(display_type);
      }
    } else if (key == QStringLiteral("decimalplaces")) {
      int dec_places = value.toInt();

      foreach (QWidget* w, widgets_) {
        static_cast<FloatSlider*>(w)->SetDecimalPlaces(dec_places);
      }
    } else if (key == QStringLiteral("autotrim")) {
      bool autotrim = value.toBool();

      foreach (QWidget* w, widgets_) {
        static_cast<FloatSlider*>(w)->SetAutoTrimDecimalPlaces(autotrim);
      }
    }
  }
}

OLIVE_NAMESPACE_EXIT
