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

NodeParamViewWidgetBridge::NodeParamViewWidgetBridge(QObject* parent, NodeInput* input) :
  QObject(parent),
  input_(input)
{
  Q_ASSERT(parent != nullptr && input_ != nullptr);

  CreateWidgets();
}

const QList<QWidget *> &NodeParamViewWidgetBridge::widgets()
{
  return widgets_;
}

void NodeParamViewWidgetBridge::CreateWidgets()
{
  // Return empty list if the NodeInput has no actual input data types
  if (input_->inputs().isEmpty()) {
    return;
  }

  // We assume the first data type is the "primary" type
  switch (input_->inputs().first()) {
  // None of these inputs have applicable UI widgets
  case NodeParam::kNone:
  case NodeParam::kAny:
  case NodeParam::kBlock:
  case NodeParam::kTexture:
  case NodeParam::kMatrix:
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
    footage_combobox->SetFootage(Node::ValueToPtr<Footage>(input_->get_value(Now())));
    connect(footage_combobox, SIGNAL(FootageChanged(Footage*)), this, SLOT(WidgetCallback()));
    // End test code

    widgets_.append(footage_combobox);

    break;
  }
  }
}

rational NodeParamViewWidgetBridge::Now()
{
  // FIXME: Actually implement this
  return 0;
}

void NodeParamViewWidgetBridge::WidgetCallback()
{
  switch (input_->inputs().first()) {
  // None of these inputs have applicable UI widgets
  case NodeParam::kNone:
  case NodeParam::kAny:
  case NodeParam::kBlock:
  case NodeParam::kTexture:
  case NodeParam::kMatrix:
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
    input_->set_value(Now(), Node::PtrToValue(footage_combobox->SelectedFootage()));
    break;
  }
  }
}
