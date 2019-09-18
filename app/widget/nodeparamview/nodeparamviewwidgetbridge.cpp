#include "nodeparamviewwidgetbridge.h"

#include <QFontComboBox>
#include <QLineEdit>
#include <QCheckBox>

#include "node/node.h"
#include "widget/footagecombobox/footagecombobox.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/integerslider.h"

// FIXME: Test code only
#include "panel/panelmanager.h"
#include "panel/project/project.h"
// End test code

NodeParamViewWidgetBridge::NodeParamViewWidgetBridge(QObject* parent) :
  QObject(parent)
{
}

void NodeParamViewWidgetBridge::AddInput(NodeInput *input)
{
  inputs_.append(input);

  // If this was the first input, create the widgets
  if (inputs_.size() == 1) {
    CreateWidgets();
  }
}

const QList<QWidget *> &NodeParamViewWidgetBridge::widgets()
{
  return widgets_;
}

void NodeParamViewWidgetBridge::CreateWidgets()
{
  NodeInput* base_input = inputs_.first();

  // Return empty list if the NodeInput has no actual input data types
  if (base_input->inputs().isEmpty()) {
    return;
  }

  // We assume the first data type is the "primary" type
  switch (base_input->data_type()) {
  // None of these inputs have applicable UI widgets
  case NodeParam::kNone:
  case NodeParam::kAny:
  case NodeParam::kBlock:
  case NodeParam::kTexture:
  case NodeParam::kMatrix:
  case NodeParam::kTrack:
  case NodeParam::kRational:
    break;
  case NodeParam::kInt:
  {
    IntegerSlider* slider = new IntegerSlider();
    widgets_.append(slider);
    connect(slider, SIGNAL(ValueChanged(int)), this, SLOT(WidgetCallback()));
    break;
  }
  case NodeParam::kFloat:
  {
    FloatSlider* slider = new FloatSlider();

    slider->SetValue(base_input->get_value(0).toDouble());

    if (base_input->has_minimum()) {
      slider->SetMinimum(base_input->minimum().toDouble());
    }

    if (base_input->has_maximum()) {
      slider->SetMaximum(base_input->maximum().toDouble());
    }

    connect(slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    widgets_.append(slider);
    break;
  }
  case NodeParam::kVec2:
  {
    QVector2D vec2 = base_input->get_value(0).value<QVector2D>();

    FloatSlider* x_slider = new FloatSlider();
    x_slider->SetValue(static_cast<double>(vec2.x()));
    widgets_.append(x_slider);
    connect(x_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* y_slider = new FloatSlider();
    y_slider->SetValue(static_cast<double>(vec2.y()));
    widgets_.append(y_slider);
    connect(y_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));
    break;
  }
  case NodeParam::kVec3:
  {
    QVector3D vec3 = base_input->get_value(0).value<QVector3D>();

    FloatSlider* x_slider = new FloatSlider();
    x_slider->SetValue(static_cast<double>(vec3.x()));
    widgets_.append(x_slider);
    connect(x_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* y_slider = new FloatSlider();
    y_slider->SetValue(static_cast<double>(vec3.y()));
    widgets_.append(y_slider);
    connect(y_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* z_slider = new FloatSlider();
    z_slider->SetValue(static_cast<double>(vec3.z()));
    widgets_.append(z_slider);
    connect(z_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));
    break;
  }
  case NodeParam::kVec4:
  {
    QVector4D vec4 = base_input->get_value(0).value<QVector4D>();

    FloatSlider* x_slider = new FloatSlider();
    x_slider->SetValue(static_cast<double>(vec4.x()));
    widgets_.append(x_slider);
    connect(x_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* y_slider = new FloatSlider();
    y_slider->SetValue(static_cast<double>(vec4.y()));
    widgets_.append(y_slider);
    connect(y_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* z_slider = new FloatSlider();
    z_slider->SetValue(static_cast<double>(vec4.z()));
    widgets_.append(z_slider);
    connect(z_slider, SIGNAL(ValueChanged(double)), this, SLOT(WidgetCallback()));

    FloatSlider* w_slider = new FloatSlider();
    w_slider->SetValue(static_cast<double>(vec4.w()));
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
  case NodeParam::kString:
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

    // FIXME: Test code
    // Pretty hacky way of getting the root folder for this node's sequence
    ProjectPanel* pp = olive::panel_focus_manager->MostRecentlyFocused<ProjectPanel>();
    footage_combobox->SetRoot(pp->project()->root());

    // Use multiple values
    footage_combobox->SetFootage(base_input->get_value(0).value<StreamPtr>());

    connect(footage_combobox, SIGNAL(FootageChanged(StreamPtr)), this, SLOT(WidgetCallback()));
    // End test code

    widgets_.append(footage_combobox);

    break;
  }
  }
}

void NodeParamViewWidgetBridge::WidgetCallback()
{
  foreach (NodeInput* input, inputs_) {
    switch (input->data_type()) {
    // None of these inputs have applicable UI widgets
    case NodeParam::kNone:
    case NodeParam::kAny:
    case NodeParam::kBlock:
    case NodeParam::kTexture:
    case NodeParam::kMatrix:
    case NodeParam::kTrack:
    case NodeParam::kRational:
      break;
    case NodeParam::kInt:
    {
      // Widget is a IntegerSlider
      IntegerSlider* int_slider = static_cast<IntegerSlider*>(sender());
      input->set_value(int_slider->GetValue());
      break;
    }
    case NodeParam::kFloat:
    {
      // Widget is a FloatSlider
      FloatSlider* float_slider = static_cast<FloatSlider*>(sender());
      input->set_value(float_slider->GetValue());
      break;
    }
    case NodeParam::kVec2:
    {
      // Widgets are two FloatSliders
      FloatSlider* slider = static_cast<FloatSlider*>(sender());

      QVector2D val = input->get_value(0).value<QVector2D>();

      if (slider == widgets_.at(0)) {
        // Slider is X slider
        val.setX(static_cast<float>(slider->GetValue()));
      } else {
        // Slider is Y slider
        val.setY(static_cast<float>(slider->GetValue()));
      }

      input->set_value(val);
      break;
    }
    case NodeParam::kVec3:
    {
      // Widgets are three FloatSliders
      FloatSlider* slider = static_cast<FloatSlider*>(sender());

      QVector3D val = input->get_value(0).value<QVector3D>();

      if (slider == widgets_.at(0)) {
        // Slider is X slider
        val.setX(static_cast<float>(slider->GetValue()));
      } else if (slider == widgets_.at(1)) {
        // Slider is Y slider
        val.setY(static_cast<float>(slider->GetValue()));
      } else {
        // Slider is Z slider
        val.setZ(static_cast<float>(slider->GetValue()));
      }

      input->set_value(val);
      break;
    }
    case NodeParam::kVec4:
    {
      // Widgets are three FloatSliders
      FloatSlider* slider = static_cast<FloatSlider*>(sender());

      QVector4D val = input->get_value(0).value<QVector4D>();

      if (slider == widgets_.at(0)) {
        // Slider is X slider
        val.setX(static_cast<float>(slider->GetValue()));
      } else if (slider == widgets_.at(1)) {
        // Slider is Y slider
        val.setY(static_cast<float>(slider->GetValue()));
      } else if (slider == widgets_.at(2)) {
        // Slider is Z slider
        val.setZ(static_cast<float>(slider->GetValue()));
      } else {
        // Slider is W slider
        val.setW(static_cast<float>(slider->GetValue()));
      }

      input->set_value(val);
      break;
    }
    case NodeParam::kFile:
      // FIXME: File selector
      break;
    case NodeParam::kColor:
      // FIXME: Color selector
      break;
    case NodeParam::kString:
    {
      // Sender is a QLineEdit
      QLineEdit* line_edit = static_cast<QLineEdit*>(sender());
      input->set_value(line_edit->text());
      break;
    }
    case NodeParam::kBoolean:
    {
      // Widget is a QCheckBox
      QCheckBox* check_box = static_cast<QCheckBox*>(sender());
      input->set_value(check_box->isChecked());
      break;
    }
    case NodeParam::kFont:
    {
      // Widget is a QFontComboBox
      QFontComboBox* font_combobox = static_cast<QFontComboBox*>(sender());
      input->set_value(font_combobox->currentFont());
      break;
    }
    case NodeParam::kFootage:
    {
      // Widget is a FootageComboBox
      FootageComboBox* footage_combobox = static_cast<FootageComboBox*>(sender());
      input->set_value(QVariant::fromValue(footage_combobox->SelectedFootage()));
      break;
    }
    }
  }
}
