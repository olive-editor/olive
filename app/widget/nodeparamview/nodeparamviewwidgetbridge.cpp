#include "nodeparamviewwidgetbridge.h"

#include <QFontComboBox>
#include <QLineEdit>
#include <QCheckBox>

#include "node/node.h"
#include "widget/footagecombobox/footagecombobox.h"

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
  switch (base_input->inputs().first()) {
  // None of these inputs have applicable UI widgets
  case NodeParam::kNone:
  case NodeParam::kAny:
  case NodeParam::kBlock:
  case NodeParam::kTexture:
  case NodeParam::kMatrix:
  case NodeParam::kTrack:
    break;
  case NodeParam::kInt:
    // FIXME: LabelSlider in INTEGER mode
    break;
  case NodeParam::kFloat:
    // FIXME: LabelSlider in FLOAT mode
    break;
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
    footage_combobox->SetFootage(Node::ValueToPtr<Footage>(base_input->get_value()));

    connect(footage_combobox, SIGNAL(FootageChanged(Footage*)), this, SLOT(WidgetCallback()));
    // End test code

    widgets_.append(footage_combobox);

    break;
  }
  }
}

void NodeParamViewWidgetBridge::WidgetCallback()
{
  foreach (NodeInput* input, inputs_) {
    switch (input->inputs().first()) {
    // None of these inputs have applicable UI widgets
    case NodeParam::kNone:
    case NodeParam::kAny:
    case NodeParam::kBlock:
    case NodeParam::kTexture:
    case NodeParam::kMatrix:
    case NodeParam::kTrack:
      break;
    case NodeParam::kInt:
      // FIXME: LabelSlider in INTEGER mode
      break;
    case NodeParam::kFloat:
      // FIXME: LabelSlider in FLOAT mode
      break;
    case NodeParam::kFile:
      // FIXME: File selector
      break;
    case NodeParam::kColor:
      // FIXME: Color selector
      break;
    case NodeParam::kString:
    {
      // Sender is a QLineEdit
      //QLineEdit* line_edit = static_cast<QLineEdit*>(sender());
      break;
    }
    case NodeParam::kBoolean:
    {
      // Widget is a QCheckBox
      //QCheckBox* check_box = static_cast<QCheckBox*>(sender());
      break;
    }
    case NodeParam::kFont:
    {
      // Widget is a QFontComboBox
      //QFontComboBox* font_combobox = static_cast<QFontComboBox*>(sender());
      break;
    }
    case NodeParam::kFootage:
    {
      // Widget is a FootageComboBox
      FootageComboBox* footage_combobox = static_cast<FootageComboBox*>(sender());
      input->set_value(Node::PtrToValue(footage_combobox->SelectedFootage()));
      break;
    }
    }
  }
}
