#include "nodeitemdock.h"


OLIVE_NAMESPACE_ENTER

NodeItemDock::NodeItemDock(Node* node, QWidget *parent) :
  QDockWidget(parent),
  node_(node)
{
  titlebar_ = new NodeItemDockTitle(node_, this);
  this->setTitleBarWidget(titlebar_);

  connect(titlebar_->ReturnCloseButton(), SIGNAL(clicked()), this, SLOT(Close()));
}

NodeItemDockTitle* NodeItemDock::GetTitleBar()
{
    return titlebar_;
}

void NodeItemDock::Close() 
{
  emit Closed(node_);
  this->close();
}

NodeItemDockTitle::NodeItemDockTitle(Node* node, QWidget* parent) :
  QWidget(parent),
  node_(node)
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setSpacing(0);
  main_layout->setMargin(0);

  // Create title bar widget
  title_bar_ = new NodeParamViewItemTitleBar(this);
  title_bar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  QHBoxLayout* title_bar_layout = new QHBoxLayout(title_bar_);

  title_bar_collapse_btn_ = new CollapseButton();
  title_bar_layout->addWidget(title_bar_collapse_btn_);

  title_bar_lbl_ = new QLabel(title_bar_);
  title_bar_layout->addWidget(title_bar_lbl_);

  title_bar_layout->addStretch();

  close_button_ = new QPushButton("x", this);
  title_bar_layout->addWidget(close_button_);

  // Add title bar to widget
  main_layout->addWidget(title_bar_);
  Retranslate();
}

QPushButton* NodeItemDockTitle::ReturnCloseButton()
{
  return close_button_;
}

CollapseButton* NodeItemDockTitle::ReturnCollapseButton() 
{
    return title_bar_collapse_btn_;
}

Node* NodeItemDockTitle::GetNode()
{
  return node_;
}

void NodeItemDockTitle::Retranslate() {
  node_->Retranslate();

  if (node_->GetLabel().isEmpty()) {
    title_bar_lbl_->setText(node_->Name());
  } else {
    title_bar_lbl_->setText(tr("%1 (%2)").arg(node_->GetLabel(), node_->Name()));
  }
}

bool NodeItemDockTitle::eventFilter(QObject* pObject, QEvent* pEvent)
{
  pEvent->ignore();
  return false;
}

OLIVE_NAMESPACE_EXIT
