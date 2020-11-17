/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "nodeparamviewkeyframecontrol.h"

#include <QHBoxLayout>
#include <QMessageBox>

#include "core.h"
#include "nodeparamviewundo.h"
#include "ui/icons/icons.h"

namespace olive {

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

rational NodeParamViewKeyframeControl::GetCurrentTimeAsNodeTime() const
{
  return GetAdjustedTime(GetTimeTarget(), input_->parentNode(), time_, NodeParam::kInput);
}

rational NodeParamViewKeyframeControl::ConvertToViewerTime(const rational &r) const
{
  return GetAdjustedTime(input_->parentNode(), GetTimeTarget(), r, NodeParam::kOutput);
}

void NodeParamViewKeyframeControl::ShowButtonsFromKeyframeEnable(bool e)
{
  prev_key_btn_->setVisible(e);
  toggle_key_btn_->setVisible(e);
  next_key_btn_->setVisible(e);
}

void NodeParamViewKeyframeControl::ToggleKeyframe(bool e)
{
  rational node_time = GetCurrentTimeAsNodeTime();

  QList<NodeKeyframePtr> keys = input_->get_keyframe_at_time(node_time);

  QUndoCommand* command = new QUndoCommand();

  if (e && keys.isEmpty()) {
    // Add a keyframe here (one for each track)
    for (int i=0;i<input_->get_number_of_keyframe_tracks();i++) {
      NodeKeyframePtr key = NodeKeyframe::Create(node_time,
                                                 input_->get_value_at_time_for_track(node_time, i),
                                                 input_->get_best_keyframe_type_for_time(node_time, i),
                                                 i);

      new NodeParamInsertKeyframeCommand(input_, key, command);
    }
  } else if (!e && !keys.isEmpty()) {
    // Remove all keyframes at this time
    foreach (NodeKeyframePtr key, keys) {
      new NodeParamRemoveKeyframeCommand(input_, key, command);

      if (input_->keyframe_tracks().at(key->track()).size() == 1) {
        // If this was the last keyframe on this track, set the standard value to the value at this time too
        new NodeParamSetStandardValueCommand(input_,
                                             key->track(),
                                             input_->get_value_at_time_for_track(node_time, key->track()),
                                             command);
      }
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewKeyframeControl::UpdateState()
{
  if (!input_) {
    return;
  }

  NodeKeyframePtr earliest_key = input_->get_earliest_keyframe();
  NodeKeyframePtr latest_key = input_->get_latest_keyframe();

  rational node_time = GetCurrentTimeAsNodeTime();

  prev_key_btn_->setEnabled(earliest_key && node_time > earliest_key->time());
  next_key_btn_->setEnabled(latest_key && node_time < latest_key->time());
  toggle_key_btn_->setChecked(input_->has_keyframe_at_time(node_time));
}

void NodeParamViewKeyframeControl::GoToPreviousKey()
{
  rational node_time = GetCurrentTimeAsNodeTime();

  NodeKeyframePtr previous_key = input_->get_closest_keyframe_before_time(node_time);

  if (previous_key) {
    rational key_time = ConvertToViewerTime(previous_key->time());

    emit RequestSetTime(key_time);
  }
}

void NodeParamViewKeyframeControl::GoToNextKey()
{
  rational node_time = GetCurrentTimeAsNodeTime();

  NodeKeyframePtr next_key = input_->get_closest_keyframe_after_time(node_time);

  if (next_key) {
    rational key_time = ConvertToViewerTime(next_key->time());

    emit RequestSetTime(key_time);
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

    // Create one keyframe across all tracks here
    QVector<QVariant> key_vals = input_->get_split_standard_value();

    for (int i=0;i<key_vals.size();i++) {
      NodeKeyframePtr key = NodeKeyframe::Create(GetCurrentTimeAsNodeTime(),
                                                 key_vals.at(i),
                                                 NodeKeyframe::kDefaultType,
                                                 i);

      new NodeParamInsertKeyframeCommand(input_, key, command);
    }
  } else {
    // Confirm the user wants to clear all keyframes
    if (QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Are you sure you want to disable keyframing on this value? This will clear all existing keyframes."),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

      // Store value at this time, we'll set this as the persistent value later
      QVector<QVariant> stored_vals = input_->get_split_values_at_time(GetCurrentTimeAsNodeTime());

      // Delete all keyframes
      foreach (const NodeInput::KeyframeTrack& track, input_->keyframe_tracks()) {
        for (int i=track.size()-1;i>=0;i--) {
          new NodeParamRemoveKeyframeCommand(input_, track.at(i), command);
        }
      }

      // Update standard value
      for (int i=0;i<stored_vals.size();i++) {
        new NodeParamSetStandardValueCommand(input_, i, stored_vals.at(i), command);
      }

      // Disable keyframing
      new NodeParamSetKeyframingCommand(input_, false, command);

    } else {
      // Disable action has effectively been ignored
      enable_key_btn_->setChecked(true);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

}
