#include "nodeitemdock.h"
#include "config/config.h"

#include <QPainter>
#include <QEvent>


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
  main_layout->setMargin(2);

  // Create title bar widget
  title_bar_ = new QWidget(this);
  title_bar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  // Probably better to do in the style sheet
  this->setStyleSheet("background-color: #404040");

  QHBoxLayout* title_bar_layout = new QHBoxLayout(title_bar_);
  title_bar_layout->setMargin(0);

  title_bar_collapse_btn_ = new CollapseButton(title_bar_);
  title_bar_collapse_btn_->setContentsMargins(0, 0, 0, 0);
  title_bar_layout->addWidget(title_bar_collapse_btn_);

  title_bar_lbl_ = new QLabel(title_bar_);
  title_bar_lbl_->setMargin(0);
  title_bar_layout->addWidget(title_bar_lbl_);

  title_bar_layout->addStretch();

  close_button_ = new QPushButton("x", title_bar_);
  close_button_->setContentsMargins(0, 0, 0, 0);
  close_button_->setFixedSize(20, 20);
  title_bar_layout->addWidget(close_button_);

  // Add title bar to widget
  main_layout->addWidget(title_bar_);
  Retranslate();
}

void NodeItemDockTitle::paintEvent(QPaintEvent* event) {
  //QWidget::paintEvent(event);
  QPainter p(this);
  //Node* node = static_cast<NodeItemDockTitle*>(parent())->GetNode();
  Color node_color = Config::Current()[QStringLiteral("NodeCatColor%1").arg(node_->Category().first())].value<Color>();

  int bottom = height() - 1;
  p.setPen(node_color.toQColor());
  // Draw double thickness top border
  p.drawLine(0, 0, width(), 0);
  p.drawLine(0, 1, width(), 1);
  
  // if active draw the sides
  bool flag = static_cast<NodeParamViewItem*>(static_cast<NodeItemDock*>(parent())->widget())->GetActive();
  if (flag) {
    p.drawLine(0, 0, 0, height());
    p.drawLine(width()-1, 0, width()-1, height());

    // If collapsed draw the bottom of the header
    if (!title_bar_collapse_btn_->isChecked()) {
      p.drawLine(0, bottom, width(), bottom);
    }
  }
  
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
