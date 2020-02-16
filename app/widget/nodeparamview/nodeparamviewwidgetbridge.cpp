#include "nodeparamviewwidgetbridge.h"

#include <QCheckBox>
#include <QFontComboBox>
#include <QLineEdit>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "core.h"
#include "node/node.h"
#include "nodeparamviewundo.h"
#include "project/item/sequence/sequence.h"
#include "undo/undostack.h"
#include "widget/footagecombobox/footagecombobox.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/integerslider.h"

NodeParamViewWidgetBridge::NodeParamViewWidgetBridge(NodeInput *input, QObject *parent) :
  QObject(parent),
  input_(input),
  dragging_(false),
  drag_created_keyframe_(false)
{
  CreateWidgets();

  connect(input_, &NodeInput::ValueChanged, this, &NodeParamViewWidgetBridge::InputValueChanged);
}

void NodeParamViewWidgetBridge::SetTime(const rational &time)
{
  time_ = time;

  if (!input_) {
    return;
  }

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
  case NodeParam::kWholeNumber:
  case NodeParam::kNumber:
  case NodeParam::kString:
  case NodeParam::kBuffer:
  case NodeParam::kVector:
    break;
  case NodeParam::kInt:
    static_cast<IntegerSlider*>(widgets_.first())->SetValue(input_->get_value_at_time(time).toLongLong());
    break;
  case NodeParam::kFloat:
    static_cast<FloatSlider*>(widgets_.first())->SetValue(input_->get_value_at_time(time).toDouble());
    break;
  case NodeParam::kVec2:
  {
    QVector2D vec2 = input_->get_value_at_time(time).value<QVector2D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec2.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec2.y()));
    break;
  }
  case NodeParam::kVec3:
  {
    QVector3D vec3 = input_->get_value_at_time(time).value<QVector3D>();

    static_cast<FloatSlider*>(widgets_.at(0))->SetValue(static_cast<double>(vec3.x()));
    static_cast<FloatSlider*>(widgets_.at(1))->SetValue(static_cast<double>(vec3.y()));
    static_cast<FloatSlider*>(widgets_.at(2))->SetValue(static_cast<double>(vec3.z()));
    break;
  }
  case NodeParam::kVec4:
  {
    QVector4D vec4 = input_->get_value_at_time(time).value<QVector4D>();

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
    // FIXME: Color selector
    break;
  case NodeParam::kText:
  {
    static_cast<QLineEdit*>(widgets_.first())->setText(input_->get_value_at_time(time).toString());
    break;
  }
  case NodeParam::kBoolean:
    static_cast<QCheckBox*>(widgets_.first())->setChecked(input_->get_value_at_time(time).toBool());
    break;
  case NodeParam::kFont:
  {
    // FIXME: Implement this
    break;
  }
  case NodeParam::kFootage:
    static_cast<FootageComboBox*>(widgets_.first())->SetFootage(input_->get_value_at_time(time).value<StreamPtr>());
    break;
  }
}

const QList<QWidget *> &NodeParamViewWidgetBridge::widgets()
{
  return widgets_;
}

void NodeParamViewWidgetBridge::CreateWidgets()
{
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
  case NodeParam::kWholeNumber:
  case NodeParam::kNumber:
  case NodeParam::kString:
  case NodeParam::kBuffer:
  case NodeParam::kVector:
    break;
  case NodeParam::kInt:
  {
    IntegerSlider* slider = new IntegerSlider();

    if (input_->has_minimum()) {
      slider->SetMinimum(input_->minimum().toLongLong());
    }

    if (input_->has_maximum()) {
      slider->SetMaximum(input_->maximum().toLongLong());
    }

    connect(slider, &IntegerSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    widgets_.append(slider);
    break;
  }
  case NodeParam::kFloat:
  {
    FloatSlider* slider = new FloatSlider();

    if (input_->has_minimum()) {
      slider->SetMinimum(input_->minimum().toDouble());
    }

    if (input_->has_maximum()) {
      slider->SetMaximum(input_->maximum().toDouble());
    }

    connect(slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    widgets_.append(slider);
    break;
  }
  case NodeParam::kVec2:
  {
    FloatSlider* x_slider = new FloatSlider();
    widgets_.append(x_slider);
    connect(x_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    FloatSlider* y_slider = new FloatSlider();
    widgets_.append(y_slider);
    connect(y_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
    break;
  }
  case NodeParam::kVec3:
  {
    FloatSlider* x_slider = new FloatSlider();
    widgets_.append(x_slider);
    connect(x_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    FloatSlider* y_slider = new FloatSlider();
    widgets_.append(y_slider);
    connect(y_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    FloatSlider* z_slider = new FloatSlider();
    widgets_.append(z_slider);
    connect(z_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
    break;
  }
  case NodeParam::kVec4:
  {
    FloatSlider* x_slider = new FloatSlider();
    widgets_.append(x_slider);
    connect(x_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    FloatSlider* y_slider = new FloatSlider();
    widgets_.append(y_slider);
    connect(y_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    FloatSlider* z_slider = new FloatSlider();
    widgets_.append(z_slider);
    connect(z_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);

    FloatSlider* w_slider = new FloatSlider();
    widgets_.append(w_slider);
    connect(w_slider, &FloatSlider::ValueChanged, this, &NodeParamViewWidgetBridge::WidgetCallback);
    break;
  }
  case NodeParam::kFile:
    // FIXME: File selector
    break;
  case NodeParam::kColor:
    // FIXME: Color selector
    break;
  case NodeParam::kText:
  {
    QLineEdit* line_edit = new QLineEdit();
    widgets_.append(line_edit);
    connect(line_edit, &QLineEdit::textEdited, this, &NodeParamViewWidgetBridge::WidgetCallback);
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
}

void NodeParamViewWidgetBridge::SetInputValue(const QVariant &value, int track)
{
  QUndoCommand* command = new QUndoCommand();

  if (input_->is_keyframing()) {
    NodeKeyframePtr existing_key = input_->get_keyframe_at_time_on_track(time_, track);

    if (existing_key) {
      new NodeParamSetKeyframeValueCommand(existing_key, value, command);
    } else {
      // No existing key, create a new one
      NodeKeyframePtr new_key = NodeKeyframe::Create(time_,
                                                     value,
                                                     input_->get_best_keyframe_type_for_time(time_, track),
                                                     track);

      new NodeParamInsertKeyframeCommand(input_, new_key, command);
    }
  } else {
    new NodeParamSetStandardValueCommand(input_, track, value, command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewWidgetBridge::ProcessSlider(SliderBase *slider, const QVariant &value)
{
  int slider_track = widgets_.indexOf(slider);

  if (slider->IsDragging()) {

    // While we're dragging, we block the input's normal signalling and create our own
    input_->blockSignals(true);

    if (!dragging_) {
      // Set up new drag
      dragging_ = true;

      // Cache current value
      drag_old_value_ = input_->get_value_at_time_for_track(time_, slider_track);

      // Determine whether we are creating a keyframe or not
      if (input_->is_keyframing()) {
        dragging_keyframe_ = input_->get_keyframe_at_time_on_track(time_, slider_track);
        drag_created_keyframe_ = !dragging_keyframe_;

        if (drag_created_keyframe_) {
          dragging_keyframe_ = NodeKeyframe::Create(time_,
                                                    value,
                                                    input_->get_best_keyframe_type_for_time(time_, slider_track),
                                                    slider_track);

          input_->insert_keyframe(dragging_keyframe_);

          // We re-enable signals temporarily to emit the keyframe added signal
          input_->blockSignals(false);
          emit input_->KeyframeAdded(dragging_keyframe_);
          input_->blockSignals(true);
        }
      }
    }

    if (input_->is_keyframing()) {
      dragging_keyframe_->set_value(value);
    } else {
      input_->set_standard_value(value, slider_track);
    }

    input_->blockSignals(false);

    input_->parentNode()->InvalidateVisible(input_);

  } else {
    if (dragging_) {
      // We were dragging and just stopped
      dragging_ = false;

      QUndoCommand* command = new QUndoCommand();

      if (input_->is_keyframing()) {
        if (drag_created_keyframe_) {
          // We created a keyframe in this process
          new NodeParamInsertKeyframeCommand(input_, dragging_keyframe_, true, command);
        }

        // We just set a keyframe's value
        // We do this even when inserting a keyframe because we don't actually perform an insert in this undo command
        // so this will ensure the ValueChanged() signal is sent correctly
        new NodeParamSetKeyframeValueCommand(dragging_keyframe_, value, drag_old_value_, command);
      } else {
        // We just set the standard value
        new NodeParamSetStandardValueCommand(input_, slider_track, value, drag_old_value_, command);
      }

      Core::instance()->undo_stack()->push(command);

    } else {
      // No drag was involved, we can just push the value
      SetInputValue(value, slider_track);
    }
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
  case NodeParam::kWholeNumber:
  case NodeParam::kNumber:
  case NodeParam::kString:
  case NodeParam::kVector:
  case NodeParam::kBuffer:
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
    // FIXME: Color selector
    break;
  case NodeParam::kText:
  {
    // Sender is a QLineEdit
    SetInputValue(static_cast<QLineEdit*>(sender())->text(), 0);
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
    SetInputValue(static_cast<QFontComboBox*>(sender())->currentFont(), 0);
    break;
  }
  case NodeParam::kFootage:
  {
    // Widget is a FootageComboBox
    SetInputValue(QVariant::fromValue(static_cast<FootageComboBox*>(sender())->SelectedFootage()), 0);
    break;
  }
  }
}

void NodeParamViewWidgetBridge::InputValueChanged(const rational &start, const rational &end)
{
  if (!dragging_ && start <= time_ && end >= time_) {
    // We'll need to update the widgets because the values have changed on our current time
    SetTime(time_);
  }
}
