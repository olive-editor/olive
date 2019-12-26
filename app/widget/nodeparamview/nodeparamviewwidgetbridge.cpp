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
  input_(input)
{
  CreateWidgets();
}

void NodeParamViewWidgetBridge::SetTime(const rational &time)
{
  time_ = time;

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

    connect(slider, SIGNAL(ValueChanged(int64_t)), this, SLOT(WidgetCallback()));

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

    connect(slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    widgets_.append(slider);
    break;
  }
  case NodeParam::kVec2:
  {
    FloatSlider* x_slider = new FloatSlider();
    widgets_.append(x_slider);
    connect(x_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* y_slider = new FloatSlider();
    widgets_.append(y_slider);
    connect(y_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));
    break;
  }
  case NodeParam::kVec3:
  {
    FloatSlider* x_slider = new FloatSlider();
    widgets_.append(x_slider);
    connect(x_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* y_slider = new FloatSlider();
    widgets_.append(y_slider);
    connect(y_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* z_slider = new FloatSlider();
    widgets_.append(z_slider);
    connect(z_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));
    break;
  }
  case NodeParam::kVec4:
  {
    FloatSlider* x_slider = new FloatSlider();
    widgets_.append(x_slider);
    connect(x_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* y_slider = new FloatSlider();
    widgets_.append(y_slider);
    connect(y_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* z_slider = new FloatSlider();
    widgets_.append(z_slider);
    connect(z_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* w_slider = new FloatSlider();
    widgets_.append(w_slider);
    connect(w_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));
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
    connect(line_edit, SIGNAL(textEdited(const QString &text)), this, SLOT(WidgetCallback()));
    break;
  }
  case NodeParam::kBoolean:
  {
    QCheckBox* check_box = new QCheckBox();
    widgets_.append(check_box);
    connect(check_box, SIGNAL(toggled(bool)), this, SLOT(WidgetCallback()));
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

    connect(footage_combobox, SIGNAL(FootageChanged(StreamPtr)), this, SLOT(WidgetCallback()));

    widgets_.append(footage_combobox);

    break;
  }
  }
}

void NodeParamViewWidgetBridge::SetInputValue(const QVariant &value)
{
  QUndoCommand* command = new QUndoCommand();

  if (input_->is_keyframing()) {
    NodeKeyframePtr existing_key = input_->get_keyframe_at_time(time_);

    if (existing_key) {
      new NodeParamSetKeyframeValueCommand(existing_key, value, command);
    } else {
      // No existing key, create a new one
      NodeKeyframePtr new_key = std::make_shared<NodeKeyframe>(time_,
                                                               value,
                                                               input_->get_best_keyframe_type_for_time(time_));

      new NodeParamInsertKeyframeCommand(input_, new_key, command);
    }
  } else {
    new NodeParamSetStandardValueCommand(input_, value, command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
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
    SetInputValue(static_cast<IntegerSlider*>(sender())->GetValue());
    break;
  }
  case NodeParam::kFloat:
  {
    // Widget is a FloatSlider
    SetInputValue(static_cast<FloatSlider*>(sender())->GetValue());
    break;
  }
  case NodeParam::kVec2:
    SetInputValue(QVector2D(
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(0))->GetValue()),
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(1))->GetValue())
                  ));
    break;
  case NodeParam::kVec3:
    // Widgets are three FloatSliders
    SetInputValue(QVector3D(
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(0))->GetValue()),
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(1))->GetValue()),
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(2))->GetValue())
                  ));
    break;
  case NodeParam::kVec4:
  {
    // Widgets are three FloatSliders
    SetInputValue(QVector4D(
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(0))->GetValue()),
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(1))->GetValue()),
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(2))->GetValue()),
                    static_cast<float>(static_cast<FloatSlider*>(widgets_.at(3))->GetValue())
                  ));
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
    SetInputValue(static_cast<QLineEdit*>(sender())->text());
    break;
  }
  case NodeParam::kBoolean:
  {
    // Widget is a QCheckBox
    SetInputValue(static_cast<QCheckBox*>(sender())->isChecked());
    break;
  }
  case NodeParam::kFont:
  {
    // Widget is a QFontComboBox
    SetInputValue(static_cast<QFontComboBox*>(sender())->currentFont());
    break;
  }
  case NodeParam::kFootage:
  {
    // Widget is a FootageComboBox
    SetInputValue(QVariant::fromValue(static_cast<FootageComboBox*>(sender())->SelectedFootage()));
    break;
  }
  }
}
