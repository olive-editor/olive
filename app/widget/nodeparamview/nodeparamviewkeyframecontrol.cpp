#include "nodeparamviewkeyframecontrol.h"

#include <QHBoxLayout>

#include "ui/icons/icons.h"

NodeParamViewKeyframeControl::NodeParamViewKeyframeControl(NodeInput* input, QWidget *parent) :
  QWidget(parent),
  input_(input)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  // Automatically right aligns all buttons
  layout->addStretch();

  prev_key_btn_ = CreateNewToolButton(olive::icon::TriLeft);
  prev_key_btn_->setIconSize(prev_key_btn_->iconSize() / 2);
  layout->addWidget(prev_key_btn_);

  toggle_key_btn_ = CreateNewToolButton(olive::icon::Diamond);
  toggle_key_btn_->setCheckable(true);
  toggle_key_btn_->setIconSize(toggle_key_btn_->iconSize() / 2);
  layout->addWidget(toggle_key_btn_);

  next_key_btn_ = CreateNewToolButton(olive::icon::TriRight);
  next_key_btn_->setIconSize(next_key_btn_->iconSize() / 2);
  layout->addWidget(next_key_btn_);

  enable_key_btn_ = CreateNewToolButton(olive::icon::Clock);
  enable_key_btn_->setCheckable(true);
  enable_key_btn_->setIconSize(enable_key_btn_->iconSize() / 4 * 3);
  layout->addWidget(enable_key_btn_);

  connect(enable_key_btn_, SIGNAL(toggled(bool)), this, SLOT(ShowButtonsFromKeyframeEnable(bool)));
  connect(enable_key_btn_, SIGNAL(toggled(bool)), this, SIGNAL(KeyframeEnableChanged(bool)));

  // Pick up keyframing value
  ShowButtonsFromKeyframeEnable(input_->is_keyframing());
}

NodeInput *NodeParamViewKeyframeControl::GetConnectedInput() const
{
  return input_;
}

void NodeParamViewKeyframeControl::SetKeyframeEnabled(bool e)
{
  // Suppress KeyframeEnableChanged() signal from this object
  blockSignals(true);

  enable_key_btn_->setChecked(e);

  blockSignals(false);
}

QPushButton *NodeParamViewKeyframeControl::CreateNewToolButton(const QIcon& icon) const
{
  QPushButton* btn = new QPushButton();
  btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  btn->setIcon(icon);

  return btn;
}

void NodeParamViewKeyframeControl::ShowButtonsFromKeyframeEnable(bool e)
{
  prev_key_btn_->setVisible(e);
  toggle_key_btn_->setVisible(e);
  next_key_btn_->setVisible(e);
}
