#include "codeeditordialog.h"
#include "searchtextbar.h"

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QMenuBar>
#include <QClipboard>
#include <QGuiApplication>
#include <QTextDocumentFragment>


namespace olive {

CodeEditorDialog::CodeEditorDialog(const QString &start, QWidget* parent) :
  QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Create text edit widget
  text_edit_ = new CodeEditor( this);
  text_edit_->document()->setPlainText(start);
  text_edit_->gotoLineNumber(1);
  layout->addWidget(text_edit_);

  // Create buttons
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, &CodeEditorDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &CodeEditorDialog::reject);

  // this does not work on all platforms
  setWindowFlag( Qt::WindowMaximizeButtonHint, true);

  QMenuBar * menu_bar = new QMenuBar( text_edit_);
  QMenu * edit_menu = new QMenu(tr("Edit"), text_edit_);
  QMenu * edit_addSnippet_menu = new QMenu(tr("add input"), text_edit_);

  edit_menu->addMenu( edit_addSnippet_menu);
  menu_bar->addMenu( edit_menu);

  QAction * add_texture_input = edit_addSnippet_menu->addAction(tr("texture"));
  QAction * add_color_input = edit_addSnippet_menu->addAction(tr("color"));
  QAction * add_float_input = edit_addSnippet_menu->addAction(tr("float"));
  QAction * add_int_input = edit_addSnippet_menu->addAction(tr("integer"));
  QAction * add_boolean_input = edit_addSnippet_menu->addAction(tr("boolean"));

  connect( add_texture_input, &QAction::triggered, this, &CodeEditorDialog::OnActionAddInputTexture);
  connect( add_color_input, &QAction::triggered, this, &CodeEditorDialog::OnActionAddInputColor);
  connect( add_float_input, &QAction::triggered, this, &CodeEditorDialog::OnActionAddInputFloat);
  connect( add_int_input, &QAction::triggered, this, &CodeEditorDialog::OnActionAddInputInt);
  connect( add_boolean_input, &QAction::triggered, this, &CodeEditorDialog::OnActionAddInputBoolean);

  search_bar_ = new SearchTextBar( this);
  search_bar_->hide();
  layout->addWidget( search_bar_);

  QAction * show_find_dialog = edit_menu->addAction(tr("find/replace"));
  show_find_dialog->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_F));

  connect( show_find_dialog, &QAction::triggered, this, &CodeEditorDialog::OnFindRequest);

  connect( search_bar_, &SearchTextBar::searchTextForward, text_edit_, &CodeEditor::onSearchForwardRequest);
  connect( search_bar_, &SearchTextBar::searchTextBackward, text_edit_, &CodeEditor::onSearchBackwardRequest);
  connect( search_bar_, &SearchTextBar::replaceText, text_edit_, &CodeEditor::onReplaceTextRequest);

  connect( text_edit_, &CodeEditor::textNotFound, search_bar_, &SearchTextBar::onTextNotFound);

  layout->setMenuBar( menu_bar);
}

void CodeEditorDialog::OnActionAddInputTexture()
{
  text_edit_->insertPlainText(
        "//OVE name: my input\n"
        "//OVE type: TEXTURE\n"
        "//OVE flag: NOT_KEYFRAMABLE\n"
        "//OVE description: \n"
        "uniform sampler2D my_input;\n");
}

void CodeEditorDialog::OnActionAddInputColor()
{
  text_edit_->insertPlainText(
        "//OVE name: my color\n"
        "//OVE type: COLOR\n"
        "//OVE default: RGBA(0.5, 0.5, 1,  1)\n"
        "//OVE description: \n"
        "uniform vec4 my_color;\n");
}

void CodeEditorDialog::OnActionAddInputFloat()
{
  text_edit_->insertPlainText(
        "//OVE name: my float\n"
        "//OVE type: FLOAT\n"
        "//OVE flag: NOT_CONNECTABLE\n"
        "//OVE min: 0.0\n"
        "//OVE max: 3.0\n"
        "//OVE default: 1.0\n"
        "//OVE description: \n"
        "uniform float my_float;\n");
}

void CodeEditorDialog::OnActionAddInputInt()
{
  text_edit_->insertPlainText(
        "//OVE name: my int\n"
        "//OVE type: INTEGER\n"
        "//OVE flag: NOT_CONNECTABLE\n"
        "//OVE min: -10\n"
        "//OVE max: 10\n"
        "//OVE default: 0\n"
        "//OVE description:\n"
        "uniform float my_int;\n");
}

void CodeEditorDialog::OnActionAddInputBoolean()
{
  text_edit_->insertPlainText(
        "//OVE name: my boolean\n"
        "//OVE type: BOOLEAN\n"
        "//OVE flag: NOT_CONNECTABLE, NOT_KEYFRAMABLE\n"
        "//OVE default: false\n"
        "//OVE description: when True, the input is passed to output as is.\n"
        "uniform bool disable;\n");
}

void CodeEditorDialog::OnFindRequest()
{
  QString text_to_find = text_edit_->textCursor().selection().toPlainText();
  search_bar_->showBar( text_to_find);
}


}  // namespace olive

