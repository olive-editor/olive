#include "nodeparamviewkeyframecontrol.h"

#include <QHBoxLayout>
#include <QMessageBox>

#include "core.h"
#include "nodeparamviewundo.h"
#include "ui/icons/icons.h"

NodeParamViewKeyframeControl::NodeParamViewKeyframeControl(bool right_align, QWidget *parent) :
  QWidget(parent),
  input_(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  if (right_align) {
    // Automatically right aligns all buttons
    layout->addStretch();
  }

  prev_key_btn_ = CreateNewToolButton(icon::TriLeft);
  prev_key_btn_->setIconSize(prev_key_btn_->iconSize() / 2);
  layout->addWidget(prev_key_btn_);

  toggle_key_btn_ = CreateNewToolButton(icon::Diamond);
  toggle_key_btn_->setCheckable(true);
  toggle_key_btn_->setIconSize(toggle_key_btn_->iconSize() / 2);
  layout->addWidget(toggle_key_btn_);

  next_key_btn_ = CreateNewToolButton(icon::TriRight);
  next_key_btn_->setIconSize(next_key_btn_->iconSize() / 2);
  layout->addWidget(next_key_btn_);

  enable_key_btn_ = CreateNewToolButton(icon::Clock);
  enable_key_btn_->setCheckable(true);
  enable_key_btn_->setIconSize(enable_key_btn_->iconSize() / 4 * 3);
  layout->addWidget(enable_key_btn_);

  connect(prev_key_btn_, &QPushButton::clicked, this, &NodeParamViewKeyframeControl::GoToPreviousKey);
  connect(next_key_btn_, &QPushButton::clicked, this, &NodeParamViewKeyframeControl::GoToNextKey);
  connect(toggle_key_btn_, &QPushButton::clicked, this, &NodeParamViewKeyframeControl::ToggleKeyframe);
  connect(enable_key_btn_, &QPushButton::toggled, this, &NodeParamViewKeyframeControl::ShowButtonsFromKeyframeEnable);
  connect(enable_key_btn_, &QPushButton::clicked, this, &NodeParamViewKeyframeControl::KeyframeEnableChanged);

  // Set defaults
  SetInput(nullptr);
  ShowButtonsFromKeyframeEnable(false);
}

NodeInput *NodeParamViewKeyframeControl::GetConnectedInput() const
{
  return input_;
}

void NodeParamViewKeyframeControl::SetInput(NodeInput *input)
{
  if (input_ != nullptr) {
    disconnect(input_, &NodeInput::KeyframeEnableChanged, enable_key_btn_, &QPushButton::setChecked);
    disconnect(input_, &NodeInput::KeyframeAdded, this, &NodeParamViewKeyframeControl::UpdateState);
    disconnect(input_, &NodeInput::KeyframeRemoved, this, &NodeParamViewKeyframeControl::UpdateState);
  }

  input_ = input;
  SetButtonsEnabled(input_);

  // Pick up keyframing value
  enable_key_btn_->setChecked(input_ && input_->is_keyframing());

  // Update buttons
  UpdateState();

  if (input_ != nullptr) {
    connect(input_, &NodeInput::KeyframeEnableChanged, enable_key_btn_, &QPushButton::setChecked);
    connect(input_, &NodeInput::KeyframeAdded, this, &NodeParamViewKeyframeControl::UpdateState);
    connect(input_, &NodeInput::KeyframeRemoved, this, &NodeParamViewKeyframeControl::UpdateState);
  }
}

void NodeParamViewKeyframeControl::SetTime(const rational &time)
{
  time_ = time;

  UpdateState();
}

QPushButton *NodeParamViewKeyframeControl::CreateNewToolButton(const QIcon& icon) const
{
  QPushButton* btn = new QPushButton();
  btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  btn->setIcon(icon);

  return btn;
}

void NodeParamViewKeyframeControl::SetButtonsEnabled(bool e)
{
  prev_key_btn_->setEnabled(e);
  toggle_key_btn_->setEnabled(e);
  next_key_btn_->setEnabled(e);
  enable_key_btn_->setEnabled(e);
}

void NodeParamViewKeyframeControl::ShowButtonsFromKeyframeEnable(bool e)
{
  prev_key_btn_->setVisible(e);
  toggle_key_btn_->setVisible(e);
  next_key_btn_->setVisible(e);
}

void NodeParamViewKeyframeControl::ToggleKeyframe(bool e)
{
  NodeKeyframePtr key = input_->get_keyframe_at_time(time_);

  QUndoCommand* command = new QUndoCommand();

  if (e && !key) {
    // Add a keyframe here
    key = NodeKeyframe::Create(time_,
                               input_->get_value_at_time(time_),
                               input_->get_best_keyframe_type_for_time(time_));

    new NodeParamInsertKeyframeCommand(input_, key, command);
  } else if (!e && key) {
    // Remove a keyframe here
    new NodeParamRemoveKeyframeCommand(input_, key, command);

    // If this was the last keyframe, we'll set the standard value to the value at this time too
    if (input_->keyframes().size() == 1) {
      new NodeParamSetStandardValueCommand(input_, key->value(), command);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewKeyframeControl::UpdateState()
{
  if (!input_) {
    return;
  }

  prev_key_btn_->setEnabled(!input_->keyframes().isEmpty() && time_ > input_->keyframes().first()->time());
  next_key_btn_->setEnabled(!input_->keyframes().isEmpty() && time_ < input_->keyframes().last()->time());
  toggle_key_btn_->setChecked(input_->has_keyframe_at_time(time_));
}

void NodeParamViewKeyframeControl::GoToPreviousKey()
{
  for (int i=input_->keyframes().size()-1;i>=0;i--) {
    // Find closest keyframe that is before this time
    const rational& this_key_time = input_->keyframes().at(i)->time();

    if (this_key_time < time_) {
      emit RequestSetTime(this_key_time);
      break;
    }
  }
}

void NodeParamViewKeyframeControl::GoToNextKey()
{
  for (int i=0;i<input_->keyframes().size();i++) {
    // Find closest keyframe that is before this time
    const rational& this_key_time = input_->keyframes().at(i)->time();

    if (this_key_time > time_) {
      emit RequestSetTime(this_key_time);
      break;
    }
  }
}

void NodeParamViewKeyframeControl::KeyframeEnableChanged(bool e)
{
  if (e == input_->is_keyframing()) {
    // No-op
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  if (e) {
    // Enable keyframing
    new NodeParamSetKeyframingCommand(input_, true, command);

    // NodeInputs already have one keyframe by default, we move it to the current time here
    NodeKeyframePtr key = NodeKeyframe::Create(time_, input_->get_standard_value(), NodeKeyframe::kDefaultType);
    new NodeParamInsertKeyframeCommand(input_, key, command);
  } else {
    // Confirm the user wants to clear all keyframes
    if (QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Are you sure you want to disable keyframing on this value? This will clear all existing keyframes."),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

      // Store value at this time, we'll set this as the persistent value later
      QVariant stored_val = input_->get_value_at_time(time_);

      // Delete all keyframes
      for (int i=input_->keyframes().size()-1;i>=0;i--) {
        new NodeParamRemoveKeyframeCommand(input_, input_->keyframes().at(i), command);
      }

      // Update standard value
      new NodeParamSetStandardValueCommand(input_, stored_val, command);

      // Disable keyframing
      new NodeParamSetKeyframingCommand(input_, false, command);

    } else {
      // Disable action has effectively been ignored
      enable_key_btn_->setChecked(true);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}
