#include "nodeitemdock.h"
#include "config/config.h"
#include "core.h"
#include "window/mainwindow/mainwindow.h"

#include <QPainter>
#include <QEvent>
#include <QInputDialog>
#include <QMenu>
#include <QPalette>
#include <QLinearGradient>


OLIVE_NAMESPACE_ENTER

NodeItemDock::NodeItemDock(Node* node, QWidget *parent) :
  QDockWidget(parent),
  node_(node)
{
  titlebar_ = new NodeItemDockTitle(node_, this);
  this->setTitleBarWidget(titlebar_);

  connect(titlebar_->ReturnCloseButton(), &QPushButton::clicked, this, &NodeItemDock::Close);
  connect(titlebar_->GetCenterButton(), &QPushButton::clicked, this, &NodeItemDock::SendNodeToCenter);
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

void NodeItemDock::SendNodeToCenter()
{
  emit CenterNode(node_);
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

  QHBoxLayout* title_bar_layout = new QHBoxLayout(title_bar_);
  title_bar_layout->setMargin(0);

  // *** Left Buttons ***

  title_bar_collapse_btn_ = new CollapseButton(title_bar_);
  title_bar_collapse_btn_->setContentsMargins(0, 0, 0, 0);
  title_bar_layout->addWidget(title_bar_collapse_btn_);

  center_button_ = new QPushButton("C", title_bar_);
  center_button_->setContentsMargins(0, 0, 0, 0);
  center_button_->setFixedSize(20, 20);
  title_bar_layout->addWidget(center_button_);

  center_parent_button_ = new QPushButton("P", title_bar_);
  center_parent_button_->setContentsMargins(0, 0, 0, 0);
  center_parent_button_->setFixedSize(20, 20);
  title_bar_layout->addWidget(center_parent_button_);
  connect(center_parent_button_, &QPushButton::clicked, this, &NodeItemDockTitle::ParentPopUpMenu);

  // *** Label ***

  title_bar_lbl_ = new ClickableLabel(title_bar_);
  title_bar_lbl_->setMargin(0);
  title_bar_layout->addWidget(title_bar_lbl_);
  connect(title_bar_lbl_, &ClickableLabel::MouseDoubleClicked, this, &NodeItemDockTitle::EditLabel);

  // *** Right Buttons ***
  title_bar_layout->addStretch();

  close_button_ = new QPushButton("X", title_bar_);
  close_button_->setContentsMargins(0, 0, 0, 0);
  close_button_->setFixedSize(20, 20);
  title_bar_layout->addWidget(close_button_);

  // Add title bar to widget
  main_layout->addWidget(title_bar_);

  QPalette app_pal = Core::instance()->main_window()->palette();
  QPalette pal = palette();
  /*
  QLinearGradient titleBarGradient(0, 0, 0, this->height());
  titleBarGradient.setColorAt(0, app_pal.color(QPalette::Base));
  titleBarGradient.setColorAt(0.5, app_pal.color(QPalette::AlternateBase));
  titleBarGradient.setColorAt(1, app_pal.color(QPalette::Base));
  //pal.setBrush(QPalette::Window, titleBarGradient);
  */
  pal.setColor(QPalette::Window, app_pal.color(QPalette::Base));
  this->setAutoFillBackground(true);
  this->setPalette(pal);

  connect(node_, &Node::LabelChanged, this, &NodeItemDockTitle::Retranslate);

  Retranslate();
}

void NodeItemDockTitle::paintEvent(QPaintEvent* event) {
  //QWidget::paintEvent(event);
  QPainter p(this);
  Color node_color = Config::Current()[QStringLiteral("NodeCatColor%1")
      .arg(node_->Category().first())].value<Color>();

  int bottom = height() - 1;
  p.setPen(node_color.toQColor());
  // Draw double thickness top border
  p.drawLine(0, 0, width() - 1, 0);
  p.drawLine(0, 1, width() - 1, 1);
  
  // if active draw the sides
  bool flag = static_cast<NodeParamViewItem*>(static_cast<NodeItemDock*>(parent())->widget())->GetActive();
  if (flag) {
    p.drawLine(0, 0, 0, height());
    p.drawLine(width()-1, 0, width()-1,  height());

    // If collapsed draw the bottom of the header
    if (!title_bar_collapse_btn_->isChecked()) {
      p.drawLine(0, bottom, width()-1, bottom);
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

QPushButton* NodeItemDockTitle::GetCenterButton()
{
  return center_button_;
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

void NodeItemDockTitle::EditLabel()
{
  QString start_label = node_->GetLabel();
  bool ok;

  QString s = QInputDialog::getText(this,
                                    tr("Label Node"),
                                    tr("Set node label"),
                                    QLineEdit::Normal,
                                    start_label,
                                    &ok);

  if (ok) {
    node_->SetLabel(s);
  }
}

void NodeItemDockTitle::ParentPopUpMenu()
{
  if (node_->GetImmediateDependencies().isEmpty()) {
    return;
  }
  QMenu* nodeParents = new QMenu(this);
  foreach(Node* node, node_->GetImmediateDependencies()) {
    QAction* action = new QAction(this);
    if (node->GetLabel().isEmpty()) {
      action->setText(node->Name());
    } else {
      action->setText(node->GetLabel());
    }

    QVariant v = QVariant::fromValue((void*)node);
    action->setData(v);

    nodeParents->addAction(action);

    connect(action, &QAction::triggered, this, &NodeItemDockTitle::CenterParentNode);
    connect(action, &QAction::triggered, nodeParents, &QMenu::deleteLater);
  }

  nodeParents->popup(QCursor::pos());
}

void NodeItemDockTitle::CenterParentNode()
{
  QAction *act = qobject_cast<QAction *>(sender());
  QVariant v = act->data();
  Node* node = (Node*)v.value<void*>();

  emit static_cast<NodeItemDock*>(parent())->CenterNode(node);
}

bool NodeItemDockTitle::eventFilter(QObject* pObject, QEvent* pEvent)
{
  pEvent->ignore();
  return false;
}

OLIVE_NAMESPACE_EXIT
