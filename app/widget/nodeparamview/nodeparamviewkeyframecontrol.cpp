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
  QWidget(parent)
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
  connect(enable_key_btn_, &QPushButton::clicked, this, &NodeParamViewKeyframeControl::KeyframeEnableBtnClicked);

  // Set defaults
  SetInput(NodeInput());
  ShowButtonsFromKeyframeEnable(false);
}

void NodeParamViewKeyframeControl::SetInput(const NodeInput& input)
{
  if (input_.IsValid()) {
    disconnect(input_.node(), &Node::KeyframeEnableChanged, this, &NodeParamViewKeyframeControl::KeyframeEnableChanged);
    disconnect(input_.node(), &Node::KeyframeAdded, this, &NodeParamViewKeyframeControl::UpdateState);
    disconnect(input_.node(), &Node::KeyframeRemoved, this, &NodeParamViewKeyframeControl::UpdateState);
    disconnect(input_.node(), &Node::KeyframeTimeChanged, this, &NodeParamViewKeyframeControl::UpdateState);
  }

  input_ = input;
  SetButtonsEnabled(input_.IsValid());

  // Pick up keyframing value
  enable_key_btn_->setChecked(input_.IsValid() && input_.IsKeyframing());

  // Update buttons
  UpdateState();

  if (input_.IsValid()) {
    connect(input_.node(), &Node::KeyframeEnableChanged, this, &NodeParamViewKeyframeControl::KeyframeEnableChanged);
    connect(input_.node(), &Node::KeyframeAdded, this, &NodeParamViewKeyframeControl::UpdateState);
    connect(input_.node(), &Node::KeyframeRemoved, this, &NodeParamViewKeyframeControl::UpdateState);
    connect(input_.node(), &Node::KeyframeTimeChanged, this, &NodeParamViewKeyframeControl::UpdateState);
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
  return GetAdjustedTime(GetTimeTarget(), input_.node(), time_, true);
}

rational NodeParamViewKeyframeControl::ConvertToViewerTime(const rational &r) const
{
  return GetAdjustedTime(input_.node(), GetTimeTarget(), r, false);
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

  QVector<NodeKeyframe*> keys = input_.node()->GetKeyframesAtTime(input_, node_time);

  MultiUndoCommand* command = new MultiUndoCommand();

  int nb_tracks = input_.node()->GetNumberOfKeyframeTracks(input_);

  if (e && keys.isEmpty()) {
    // Add a keyframe here (one for each track)
    for (int i=0;i<nb_tracks;i++) {
      NodeKeyframe* key = new NodeKeyframe(node_time,
                                           input_.node()->GetSplitValueAtTimeOnTrack(input_, node_time, i),
                                           input_.node()->GetBestKeyframeTypeForTimeOnTrack(input_, node_time, i),
                                           i,
                                           input_.element(),
                                           input_.input());

      command->add_child(new NodeParamInsertKeyframeCommand(input_.node(), key));
    }
  } else if (!e && !keys.isEmpty()) {
    // Remove all keyframes at this time
    foreach (NodeKeyframe* key, keys) {
      command->add_child(new NodeParamRemoveKeyframeCommand(key));

      if (input_.node()->GetKeyframeTracks(input_).size() == 1) {
        // If this was the last keyframe on this track, set the standard value to the value at this time too
        command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(input_, key->track()),
                                                                input_.node()->GetSplitValueAtTimeOnTrack(input_, node_time, key->track())));
      }
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewKeyframeControl::UpdateState()
{
  if (!input_.IsValid()) {
    return;
  }

  NodeKeyframe* earliest_key = input_.node()->GetEarliestKeyframe(input_);
  NodeKeyframe* latest_key = input_.node()->GetLatestKeyframe(input_);

  rational node_time = GetCurrentTimeAsNodeTime();

  prev_key_btn_->setEnabled(earliest_key && node_time > earliest_key->time());
  next_key_btn_->setEnabled(latest_key && node_time < latest_key->time());
  toggle_key_btn_->setChecked(input_.node()->HasKeyframeAtTime(input_, node_time));
}

void NodeParamViewKeyframeControl::GoToPreviousKey()
{
  rational node_time = GetCurrentTimeAsNodeTime();

  NodeKeyframe* previous_key = input_.node()->GetClosestKeyframeBeforeTime(input_, node_time);

  if (previous_key) {
    rational key_time = ConvertToViewerTime(previous_key->time());

    emit RequestSetTime(key_time);
  }
}

void NodeParamViewKeyframeControl::GoToNextKey()
{
  rational node_time = GetCurrentTimeAsNodeTime();

  NodeKeyframe* next_key = input_.node()->GetClosestKeyframeAfterTime(input_, node_time);

  if (next_key) {
    rational key_time = ConvertToViewerTime(next_key->time());

    emit RequestSetTime(key_time);
  }
}

void NodeParamViewKeyframeControl::KeyframeEnableBtnClicked(bool e)
{
  if (e == input_.IsKeyframing()) {
    // No-op
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  if (e) {
    // Enable keyframing
    command->add_child(new NodeParamSetKeyframingCommand(input_, true));

    // Create one keyframe across all tracks here
    const QVector<QVariant>& key_vals = input_.node()->GetSplitStandardValue(input_);

    for (int i=0;i<key_vals.size();i++) {
      NodeKeyframe* key = new NodeKeyframe(GetCurrentTimeAsNodeTime(),
                                           key_vals.at(i),
                                           NodeKeyframe::kDefaultType,
                                           i,
                                           input_.element(),
                                           input_.input());

      command->add_child(new NodeParamInsertKeyframeCommand(input_.node(), key));
    }
  } else {
    // Confirm the user wants to clear all keyframes
    if (QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Are you sure you want to disable keyframing on this value? This will clear all existing keyframes."),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

      // Store value at this time, we'll set this as the persistent value later
      const QVector<QVariant>& stored_vals = input_.node()->GetSplitValueAtTime(input_, GetCurrentTimeAsNodeTime());

      // Delete all keyframes
      foreach (const NodeKeyframeTrack& track, input_.node()->GetKeyframeTracks(input_)) {
        for (int i=track.size()-1;i>=0;i--) {
          command->add_child(new NodeParamRemoveKeyframeCommand(track.at(i)));
        }
      }

      // Update standard value
      for (int i=0;i<stored_vals.size();i++) {
        command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(input_, i), stored_vals.at(i)));
      }

      // Disable keyframing
      command->add_child(new NodeParamSetKeyframingCommand(input_, false));

    } else {
      // Disable action has effectively been ignored
      enable_key_btn_->setChecked(true);
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

void NodeParamViewKeyframeControl::KeyframeEnableChanged(const NodeInput &input, bool e)
{
  if (input_ == input) {
    enable_key_btn_->setChecked(e);
  }
}

}
