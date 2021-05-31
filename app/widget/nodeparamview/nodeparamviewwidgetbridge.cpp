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

#include "nodeparamviewwidgetbridge.h"

#include <QCheckBox>
#include <QFontComboBox>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "core.h"
#include "node/node.h"
#include "node/project/sequence/sequence.h"
#include "nodeparamviewarraywidget.h"
#include "nodeparamviewtextedit.h"
#include "nodeparamviewundo.h"
#include "undo/undostack.h"
#include "widget/colorbutton/colorbutton.h"
#include "widget/filefield/filefield.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/integerslider.h"
#include "widget/slider/rationalslider.h"
#include "widget/videoparamedit/videoparamedit.h"

namespace olive {

NodeParamViewWidgetBridge::NodeParamViewWidgetBridge(const NodeInput &input, QObject *parent) :
  QObject(parent),
  input_(input)
{
  CreateWidgets();

  connect(input_.node(), &Node::ValueChanged, this, &NodeParamViewWidgetBridge::InputValueChanged);
  connect(input_.node(), &Node::InputPropertyChanged, this, &NodeParamViewWidgetBridge::PropertyChanged);
  connect(input_.node(), &Node::InputDataTypeChanged, this, &NodeParamViewWidgetBridge::InputDataTypeChanged);
}

void NodeParamViewWidgetBridge::SetTime(const rational &time)
{
  time_ = time;

  if (input_.IsValid()) {
    UpdateWidgetValues();
  }
}

void NodeParamViewWidgetBridge::CreateWidgets()
{
  if (input_.IsArray() && input_.element() == -1) {

    NodeParamViewArrayWidget* w = new NodeParamViewArrayWidget(input_.node(), input_.input());
    connect(w, &NodeParamViewArrayWidget::DoubleClicked, this, &NodeParamViewWidgetBridge::ArrayWidgetDoubleClicked);
    widgets_.append(w);

  } else {

    // We assume the first data type is the "primary" type
    switch (input_.GetDataType()) {
    // None of these inputs have applicable UI widgets
    case NodeValue::kNone:
    case NodeValue::kTexture:
    case NodeValue::kMatrix:
    case NodeValue::kSamples:
    case NodeValue::kFootageJob:
    case NodeValue::kShaderJob:
    case NodeValue::kSampleJob:
    case NodeValue::kGenerateJob:
      break;
    case NodeValue::kInt:
    {
      CreateSliders<IntegerSlider>(1);
      break;
    }
    case NodeValue::kFloat:
    {
      CreateSliders<FloatSlider>(1);
      break;
    }
    case NodeValue::kRational:
    {
      CreateSliders<RationalSlider>(1);
      break;
    }
    case NodeValue::kVec2:
    {
      CreateSliders<FloatSlider>(2);
      break;
    }
    case NodeValue::kVec3:
    {
      CreateSliders<FloatSlider>(3);
      break;
    }
    case NodeValue::kVec4:
    {
      CreateSliders<FloatSlider>(4);
      break;
    }
    case NodeValue::kCombo:
    {
      QComboBox* combobox = new QComboBox();

      QStringList items = input_.GetComboBoxStrings();
      foreach (const QString& s, items) {
        combobox->addItem(s);
      }

      widgets_.append(combobox);
      connect(combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kFile:
    {
      FileField* file_field = new FileField();
      widgets_.append(file_field);
      connect(file_field, &FileField::FilenameChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kColor:
    {
      ColorButton* color_button = new ColorButton(input_.node()->project()->color_manager());
      widgets_.append(color_button);
      connect(color_button, &ColorButton::ColorChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kText:
    {
      NodeParamViewTextEdit* line_edit = new NodeParamViewTextEdit();
      widgets_.append(line_edit);
      connect(line_edit, &NodeParamViewTextEdit::textEdited, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kBoolean:
    {
      QCheckBox* check_box = new QCheckBox();
      widgets_.append(check_box);
      connect(check_box, &QCheckBox::clicked, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kFont:
    {
      QFontComboBox* font_combobox = new QFontComboBox();
      widgets_.append(font_combobox);
      connect(font_combobox, &QFontComboBox::currentFontChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kVideoParams:
    {
      VideoParamEdit* edit = new VideoParamEdit();
      edit->SetColorManager(input_.node()->project()->color_manager());
      widgets_.append(edit);
      connect(edit, &VideoParamEdit::Changed, this, &NodeParamViewWidgetBridge::WidgetCallback);
      break;
    }
    case NodeValue::kAudioParams:
    {
      // FIXME: Create audio param widget
      break;
    }
    }

    // Check all properties
    auto input_properties = input_.node()->GetInputProperties(input_.input());
    for (auto it=input_properties.cbegin(); it!=input_properties.cend(); it++) {
      PropertyChanged(input_.input(), it.key(), it.value());
    }

    UpdateWidgetValues();

    // Install event filter to disable widgets picking up scroll events
    foreach (QWidget* w, widgets_) {
      w->installEventFilter(&scroll_filter_);
    }

  }
}

void NodeParamViewWidgetBridge::SetInputValue(const QVariant &value, int track)
{
  MultiUndoCommand* command = new MultiUndoCommand();

  SetInputValueInternal(value, track, command, true);

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewWidgetBridge::SetInputValueInternal(const QVariant &value, int track, MultiUndoCommand *command, bool insert_on_all_tracks_if_no_key)
{
  rational node_time = GetCurrentTimeAsNodeTime();

  if (input_.IsKeyframing()) {
    NodeKeyframe* existing_key = input_.GetKeyframeAtTimeOnTrack(node_time, track);

    if (existing_key) {
      command->add_child(new NodeParamSetKeyframeValueCommand(existing_key, value));
    } else {
      // No existing key, create a new one
      int nb_tracks = NodeValue::get_number_of_keyframe_tracks(input_.node()->GetInputDataType(input_.input()));
      for (int i=0; i<nb_tracks; i++) {
        QVariant track_value;

        if (i == track) {
          track_value = value;
        } else if (!insert_on_all_tracks_if_no_key) {
          continue;
        } else {
          track_value = input_.node()->GetSplitValueAtTimeOnTrack(input_.input(), node_time, i, input_.element());
        }

        NodeKeyframe* new_key = new NodeKeyframe(node_time,
                                                 track_value,
                                                 input_.node()->GetBestKeyframeTypeForTimeOnTrack(NodeKeyframeTrackReference(input_, i), node_time),
                                                 i,
                                                 input_.element(),
                                                 input_.input());

        command->add_child(new NodeParamInsertKeyframeCommand(input_.node(), new_key));
      }
    }
  } else {
    command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(input_, track), value));
  }
}

void NodeParamViewWidgetBridge::ProcessSlider(NumericSliderBase *slider, const QVariant &value)
{
  rational node_time = GetCurrentTimeAsNodeTime();

  int slider_track = widgets_.indexOf(slider);

  if (slider->IsDragging()) {

    // While we're dragging, we block the input's normal signalling and create our own
    if (!dragger_.IsStarted()) {
      dragger_.Start(NodeKeyframeTrackReference(input_, slider_track), node_time);
    }

    dragger_.Drag(value);

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
  switch (input_.GetDataType()) {
  // None of these inputs have applicable UI widgets
  case NodeValue::kNone:
  case NodeValue::kTexture:
  case NodeValue::kMatrix:
  case NodeValue::kSamples:
  case NodeValue::kFootageJob:
  case NodeValue::kShaderJob:
  case NodeValue::kSampleJob:
  case NodeValue::kGenerateJob:
    break;
  case NodeValue::kInt:
  {
    // Widget is a IntegerSlider
    IntegerSlider* slider = static_cast<IntegerSlider*>(sender());

    ProcessSlider(slider, QVariant::fromValue(slider->GetValue()));
    break;
  }
  case NodeValue::kFloat:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeValue::kRational:
  {
    // Widget is a RationalSlider
    RationalSlider* slider = static_cast<RationalSlider*>(sender());
    ProcessSlider(slider, QVariant::fromValue(slider->GetValue()));
    break;
  }
  case NodeValue::kVec2:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeValue::kVec3:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeValue::kVec4:
  {
    // Widget is a FloatSlider
    FloatSlider* slider = static_cast<FloatSlider*>(sender());

    ProcessSlider(slider, slider->GetValue());
    break;
  }
  case NodeValue::kFile:
  {
    SetInputValue(static_cast<FileField*>(sender())->GetFilename(), 0);
    break;
  }
  case NodeValue::kColor:
  {
    // Sender is a ColorButton
    ManagedColor c = static_cast<ColorButton*>(sender())->GetColor();

    MultiUndoCommand* command = new MultiUndoCommand();

    SetInputValueInternal(c.red(), 0, command, false);
    SetInputValueInternal(c.green(), 1, command, false);
    SetInputValueInternal(c.blue(), 2, command, false);
    SetInputValueInternal(c.alpha(), 3, command, false);

    Node* n = input_.node();
    n->blockSignals(true);
    n->SetInputProperty(input_.input(), QStringLiteral("col_input"), c.color_input());
    n->SetInputProperty(input_.input(), QStringLiteral("col_display"), c.color_output().display());
    n->SetInputProperty(input_.input(), QStringLiteral("col_view"), c.color_output().view());
    n->SetInputProperty(input_.input(), QStringLiteral("col_look"), c.color_output().look());
    n->blockSignals(false);

    Core::instance()->undo_stack()->pushIfHasChildren(command);
    break;
  }
  case NodeValue::kText:
  {
    // Sender is a NodeParamViewRichText
    SetInputValue(static_cast<NodeParamViewTextEdit*>(sender())->text(), 0);
    break;
  }
  case NodeValue::kBoolean:
  {
    // Widget is a QCheckBox
    SetInputValue(static_cast<QCheckBox*>(sender())->isChecked(), 0);
    break;
  }
  case NodeValue::kFont:
  {
    // Widget is a QFontComboBox
    SetInputValue(static_cast<QFontComboBox*>(sender())->currentFont().family(), 0);
    break;
  }
  case NodeValue::kCombo:
  {
    // Widget is a QComboBox
    QComboBox* cb = static_cast<QComboBox*>(widgets_.first());
    int index = cb->currentIndex();

    // Subtract any splitters up until this point
    for (int i=index-1; i>=0; i--) {
      if (cb->itemData(i, Qt::AccessibleDescriptionRole).toString() == QStringLiteral("separator")) {
        index--;
      }
    }

    SetInputValue(index, 0);
    break;
  }
  case NodeValue::kVideoParams:
  {
    VideoParamEdit* edit = static_cast<VideoParamEdit*>(sender());
    SetInputValue(QVariant::fromValue(edit->GetVideoParams()), 0);
    break;
  }
  case NodeValue::kAudioParams:
    // FIXME: No audio param widget yet
    break;
  }
}

template <typename T>
void NodeParamViewWidgetBridge::CreateSliders(int count)
{
  for (int i=0;i<count;i++) {
    T* fs = new T();
    fs->SliderBase::SetDefaultValue(input_.GetSplitDefaultValueForTrack(i));
    fs->SetLadderElementCount(2);
    widgets_.append(fs);
    connect(fs, &T::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
  }
}

void NodeParamViewWidgetBridge::UpdateWidgetValues()
{
  if (input_.IsArray() && input_.element() == -1) {
    return;
  }

  rational node_time = GetCurrentTimeAsNodeTime();

  // We assume the first data type is the "primary" type
  switch (input_.GetDataType()) {
  // None of these inputs have applicable UI widgets
  case NodeValue::kNone:
  case NodeValue::kTexture:
  case NodeValue::kMatrix:
  case NodeValue::kSamples:
  case NodeValue::kFootageJob:
  case NodeValue::kShaderJob:
  case NodeValue::kSampleJob:
  case NodeValue::kGenerateJob:
    break;
  case NodeValue::kInt:
  {
    static_cast<IntegerSlider*>(widgets_.first())->SetValue(input_.GetValueAtTime(node_time).toLongLong());
    break;
  }
  case NodeValue::kFloat:
  {
    static_cast<FloatSlider*>(widgets_.first())->SetValue(input_.GetValueAtTime(node_time).toDouble());
    break;
  }
  case NodeValue::kRational:
  {
    static_cast<RationalSlider*>(widgets_.first())->SetValue(input_.GetValueAtTime(node_time).value<rational>());
    break;
  }
  case NodeValue::kVec2:
  {
    QVector2D vec2 = input_.GetValueAtTime(node_time).value<QVector2D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec2.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec2.y()));
    break;
  }
  case NodeValue::kVec3:
  {
    QVector3D vec3 = input_.GetValueAtTime(node_time).value<QVector3D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec3.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec3.y()));
    static_cast<FloatSlider*>(widgets_.at(2))->SetValue(static_cast<double>(vec3.z()));
    break;
  }
  case NodeValue::kVec4:
  {
    QVector4D vec4 = input_.GetValueAtTime(node_time).value<QVector4D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec4.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec4.y()));
    static_cast<FloatSlider*>(widgets_.at(2))->SetValue(static_cast<double>(vec4.z()));
    static_cast<FloatSlider*>(widgets_.at(3))->SetValue(static_cast<double>(vec4.w()));
    break;
  }
  case NodeValue::kFile:
  {
    FileField* ff = static_cast<FileField*>(widgets_.first());
    ff->SetFilename(input_.GetValueAtTime(node_time).toString());
    break;
  }
  case NodeValue::kColor:
  {
    ManagedColor mc = input_.GetValueAtTime(node_time).value<Color>();

    mc.set_color_input(input_.GetProperty("col_input").toString());

    QString d = input_.GetProperty("col_display").toString();
    QString v = input_.GetProperty("col_view").toString();
    QString l = input_.GetProperty("col_look").toString();

    mc.set_color_output(ColorTransform(d, v, l));

    static_cast<ColorButton*>(widgets_.first())->SetColor(mc);
    break;
  }
  case NodeValue::kText:
  {
    NodeParamViewTextEdit* e = static_cast<NodeParamViewTextEdit*>(widgets_.first());
    e->setTextPreservingCursor(input_.GetValueAtTime(node_time).toString());
    break;
  }
  case NodeValue::kBoolean:
    static_cast<QCheckBox*>(widgets_.first())->setChecked(input_.GetValueAtTime(node_time).toBool());
    break;
  case NodeValue::kFont:
  {
    QFontComboBox* fc = static_cast<QFontComboBox*>(widgets_.first());
    fc->blockSignals(true);
    fc->setCurrentFont(input_.GetValueAtTime(node_time).toString());
    fc->blockSignals(false);
    break;
  }
  case NodeValue::kCombo:
  {
    QComboBox* cb = static_cast<QComboBox*>(widgets_.first());
    cb->blockSignals(true);
    cb->setCurrentIndex(input_.GetValueAtTime(node_time).toInt());
    cb->blockSignals(false);
    break;
  }
  case NodeValue::kVideoParams:
  {
    VideoParamEdit* edit = static_cast<VideoParamEdit*>(widgets_.first());
    edit->SetVideoParams(input_.GetValueAtTime(node_time).value<VideoParams>());
    break;
  }
  case NodeValue::kAudioParams:
    // FIXME: No audio param widget
    break;
  }
}

rational NodeParamViewWidgetBridge::GetCurrentTimeAsNodeTime() const
{
  return GetAdjustedTime(GetTimeTarget(), input_.node(), time_, true);
}

void NodeParamViewWidgetBridge::SetTimebase(const rational& timebase)
{
  if (input_.GetDataType() == NodeValue::kRational) {
    static_cast<RationalSlider*>(widgets_.first())->SetTimebase(timebase);
  }
}

void NodeParamViewWidgetBridge::InputValueChanged(const NodeInput &input, const TimeRange &range)
{
  if (input_ == input
      && !dragger_.IsStarted()
      && range.in() <= time_ && range.out() >= time_) {
    // We'll need to update the widgets because the values have changed on our current time
    UpdateWidgetValues();
  }
}

void NodeParamViewWidgetBridge::PropertyChanged(const QString& input, const QString &key, const QVariant &value)
{
  if (input != input_.input() || (input_.IsArray() && input_.element() == -1)) {
    return;
  }

  NodeValue::Type data_type = input_.GetDataType();

  // Parameters for all types
  if (key == QStringLiteral("enabled")) {
    foreach (QWidget* w, widgets_) {
      w->setEnabled(value.toBool());
    }
  }

  // Parameters for vectors only
  if (NodeValue::type_is_vector(data_type)) {
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
  if (NodeValue::type_is_numeric(data_type) || NodeValue::type_is_vector(data_type)) {
    if (key == QStringLiteral("min")) {
      switch (data_type) {
      case NodeValue::kInt:
        static_cast<IntegerSlider*>(widgets_.first())->SetMinimum(value.value<int64_t>());
        break;
      case NodeValue::kFloat:
        static_cast<FloatSlider*>(widgets_.first())->SetMinimum(value.toDouble());
        break;
      case NodeValue::kRational:
        static_cast<RationalSlider*>(widgets_.first())->SetMinimum(value.value<rational>());
        break;
      case NodeValue::kVec2:
      {
        QVector2D min = value.value<QVector2D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMinimum(min.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMinimum(min.y());
        break;
      }
      case NodeValue::kVec3:
      {
        QVector3D min = value.value<QVector3D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMinimum(min.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMinimum(min.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetMinimum(min.z());
        break;
      }
      case NodeValue::kVec4:
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
      switch (data_type) {
      case NodeValue::kInt:
        static_cast<IntegerSlider*>(widgets_.first())->SetMaximum(value.value<int64_t>());
        break;
      case NodeValue::kFloat:
        static_cast<FloatSlider*>(widgets_.first())->SetMaximum(value.toDouble());
        break;
      case NodeValue::kRational:
        static_cast<RationalSlider*>(widgets_.first())->SetMaximum(value.value<rational>());
        break;
      case NodeValue::kVec2:
      {
        QVector2D max = value.value<QVector2D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMaximum(max.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMaximum(max.y());
        break;
      }
      case NodeValue::kVec3:
      {
        QVector3D max = value.value<QVector3D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetMaximum(max.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetMaximum(max.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetMaximum(max.z());
        break;
      }
      case NodeValue::kVec4:
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
    } else if (key == QStringLiteral("offset")) {
      switch (data_type) {
      case NodeValue::kInt:
        static_cast<IntegerSlider*>(widgets_.first())->SetOffset(value);
        break;
      case NodeValue::kFloat:
        static_cast<FloatSlider*>(widgets_.first())->SetOffset(value);
        break;
      case NodeValue::kRational:
        static_cast<RationalSlider*>(widgets_.first())->SetOffset(value);
        break;
      case NodeValue::kVec2:
      {
        QVector2D offs = value.value<QVector2D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetOffset(offs.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetOffset(offs.y());
        break;
      }
      case NodeValue::kVec3:
      {
        QVector3D offs = value.value<QVector3D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetOffset(offs.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetOffset(offs.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetOffset(offs.z());
        break;
      }
      case NodeValue::kVec4:
      {
        QVector4D offs = value.value<QVector4D>();
        static_cast<FloatSlider*>(widgets_.at(0))->SetOffset(offs.x());
        static_cast<FloatSlider*>(widgets_.at(1))->SetOffset(offs.y());
        static_cast<FloatSlider*>(widgets_.at(2))->SetOffset(offs.z());
        static_cast<FloatSlider*>(widgets_.at(3))->SetOffset(offs.w());
        break;
      }
      default:
        break;
      }

      UpdateWidgetValues();
    }
  }

  // ComboBox strings changing
  if (data_type == NodeValue::kCombo) {
    QComboBox* cb = static_cast<QComboBox*>(widgets_.first());

    int old_index = cb->currentIndex();

    // Block the combobox changed signals since we anticipate the index will be the same and not require a re-render
    cb->blockSignals(true);

    cb->clear();

    QStringList items = input_.GetComboBoxStrings();
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
  if (data_type == NodeValue::kFloat || NodeValue::type_is_vector(data_type)) {
    if (key == QStringLiteral("view")) {
      FloatSlider::DisplayType display_type = static_cast<FloatSlider::DisplayType>(value.toInt());

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

  if (data_type == NodeValue::kRational) {
    if (key == QStringLiteral("view")) {
      RationalSlider::DisplayType display_type = static_cast<RationalSlider::DisplayType>(value.toInt());

      foreach (QWidget* w, widgets_) {
        static_cast<RationalSlider*>(w)->SetDisplayType(display_type);
      }
    } else if (key == QStringLiteral("viewlock")) {
      bool locked = value.toBool();

      foreach (QWidget* w, widgets_) {
        static_cast<RationalSlider*>(w)->SetLockDisplayType(locked);
      }
    }
  }

  // Parameters for files
  if (data_type == NodeValue::kFile) {
    FileField* ff = static_cast<FileField*>(widgets_.first());

    if (key == QStringLiteral("placeholder")) {
      ff->SetPlaceholder(value.toString());
    } else if (key == QStringLiteral("directory")) {
      ff->SetDirectoryMode(value.toBool());
    }
  }

  // Parameters for video param objects
  if (data_type == NodeValue::kVideoParams) {
    VideoParamEdit* edit = static_cast<VideoParamEdit*>(widgets_.first());

    if (key == QStringLiteral("mask")) {
      edit->SetParameterMask(value.toULongLong());
    }
  }
}

void NodeParamViewWidgetBridge::InputDataTypeChanged(const QString &input, NodeValue::Type type)
{
  Q_UNUSED(type)
  if (input == this->input_.input()) {
    // Delete all widgets
    qDeleteAll(widgets_);
    widgets_.clear();

    // Create new widgets
    CreateWidgets();

    // Signal that widgets are new
    emit WidgetsRecreated(input_);
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
