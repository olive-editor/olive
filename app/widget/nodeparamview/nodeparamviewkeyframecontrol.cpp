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
  SetInput(nullptr, -1);
  ShowButtonsFromKeyframeEnable(false);
}

void NodeParamViewKeyframeControl::SetInput(NodeInput *input, int element)
{
  if (input_ != nullptr) {
    disconnect(input_, &NodeInput::KeyframeEnableChanged, enable_key_btn_, &QPushButton::setChecked);
    disconnect(input_, &NodeInput::KeyframeAdded, this, &NodeParamViewKeyframeControl::UpdateState);
    disconnect(input_, &NodeInput::KeyframeRemoved, this, &NodeParamViewKeyframeControl::UpdateState);
  }

  input_ = input;
  element_ = element;
  SetButtonsEnabled(input_);

  // Pick up keyframing value
  enable_key_btn_->setChecked(input_ && input_->IsKeyframing(element_));

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
  return GetAdjustedTime(GetTimeTarget(), input_->parent(), time_, true);
}

rational NodeParamViewKeyframeControl::ConvertToViewerTime(const rational &r) const
{
  return GetAdjustedTime(input_->parent(), GetTimeTarget(), r, false);
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

  QVector<NodeKeyframe*> keys = input_->GetKeyframesAtTime(node_time, element_);

  QUndoCommand* command = new QUndoCommand();

  int nb_tracks = input_->GetNumberOfKeyframeTracks();

  if (e && keys.isEmpty()) {
    // Add a keyframe here (one for each track)
    for (int i=0;i<nb_tracks;i++) {
      NodeKeyframe* key = new NodeKeyframe(node_time,
                                           input_->GetValueAtTimeForTrack(node_time, i, element_),
                                           input_->GetBestKeyframeTypeForTime(node_time, i, element_),
                                           i,
                                           element_);

      new NodeParamInsertKeyframeCommand(input_, key, command);
    }
  } else if (!e && !keys.isEmpty()) {
    // Remove all keyframes at this time
    foreach (NodeKeyframe* key, keys) {
      new NodeParamRemoveKeyframeCommand(key, command);

      if (input_->GetKeyframeTracks(key->track()).size() == 1) {
        // If this was the last keyframe on this track, set the standard value to the value at this time too
        new NodeParamSetStandardValueCommand(input_,
                                             key->track(),
                                             element_,
                                             input_->GetValueAtTimeForTrack(node_time, key->track(), element_),
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

  NodeKeyframe* earliest_key = input_->GetEarliestKeyframe(element_);
  NodeKeyframe* latest_key = input_->GetLatestKeyframe(element_);

  rational node_time = GetCurrentTimeAsNodeTime();

  prev_key_btn_->setEnabled(earliest_key && node_time > earliest_key->time());
  next_key_btn_->setEnabled(latest_key && node_time < latest_key->time());
  toggle_key_btn_->setChecked(input_->HasKeyframeAtTime(node_time, element_));
}

void NodeParamViewKeyframeControl::GoToPreviousKey()
{
  rational node_time = GetCurrentTimeAsNodeTime();

  NodeKeyframe* previous_key = input_->GetClosestKeyframeBeforeTime(node_time, element_);

  if (previous_key) {
    rational key_time = ConvertToViewerTime(previous_key->time());

    emit RequestSetTime(key_time);
  }
}

void NodeParamViewKeyframeControl::GoToNextKey()
{
  rational node_time = GetCurrentTimeAsNodeTime();

  NodeKeyframe* next_key = input_->GetClosestKeyframeAfterTime(node_time, element_);

  if (next_key) {
    rational key_time = ConvertToViewerTime(next_key->time());

    emit RequestSetTime(key_time);
  }
}

void NodeParamViewKeyframeControl::KeyframeEnableChanged(bool e)
{
  if (e == input_->IsKeyframing(element_)) {
    // No-op
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  if (e) {
    // Enable keyframing
    new NodeParamSetKeyframingCommand(input_, true, command);

    // Create one keyframe across all tracks here
    const QVector<QVariant>& key_vals = input_->GetSplitStandardValue(element_);

    for (int i=0;i<key_vals.size();i++) {
      NodeKeyframe* key = new NodeKeyframe(GetCurrentTimeAsNodeTime(),
                                           key_vals.at(i),
                                           NodeKeyframe::kDefaultType,
                                           i,
                                           element_);

      new NodeParamInsertKeyframeCommand(input_, key, command);
    }
  } else {
    // Confirm the user wants to clear all keyframes
    if (QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Are you sure you want to disable keyframing on this value? This will clear all existing keyframes."),
                             QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

      // Store value at this time, we'll set this as the persistent value later
      const QVector<QVariant>& stored_vals = input_->GetSplitValuesAtTime(GetCurrentTimeAsNodeTime(), element_);

      // Delete all keyframes
      foreach (const NodeKeyframeTrack& track, input_->GetKeyframeTracks(element_)) {
        for (int i=track.size()-1;i>=0;i--) {
          new NodeParamRemoveKeyframeCommand(track.at(i), command);
        }
      }

      // Update standard value
      for (int i=0;i<stored_vals.size();i++) {
        new NodeParamSetStandardValueCommand(input_, i, element_, stored_vals.at(i), command);
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
