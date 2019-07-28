/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "timeline.h"

TimelineOutput::TimelineOutput() :
  current_block_(nullptr),
  attached_timeline_(nullptr)
{
  block_input_ = new NodeInput();
  block_input_->add_data_input(NodeInput::kBlock);
  AddParameter(block_input_);

  texture_output_ = new NodeOutput();
  texture_output_->set_data_type(NodeOutput::kTexture);
  AddParameter(texture_output_);
}

QString TimelineOutput::Name()
{
  return tr("Timeline");
}

QString TimelineOutput::id()
{
  return "org.olivevideoeditor.Olive.timeline";
}

QString TimelineOutput::Category()
{
  return tr("Output");
}

QString TimelineOutput::Description()
{
  return tr("Node for communicating between a Timeline panel and the node graph. Also represents the end of a"
            "Sequence.");
}

#include "project/item/sequence/sequence.h"

void TimelineOutput::AttachTimeline(TimelinePanel *timeline)
{
  if (attached_timeline_ != nullptr) {
    attached_timeline_->Clear();
  }

  attached_timeline_ = timeline;

  if (attached_timeline_ != nullptr) {
    attached_timeline_->Clear();

    Block* previous_block = attached_block();
    while (previous_block != nullptr) {

      // FIXME: Dynamic cast is a dumb way of doing this
      ClipBlock* clip_block = dynamic_cast<ClipBlock*>(previous_block);

      if (clip_block != nullptr) {
        attached_timeline_->AddClip(clip_block);
      }

      previous_block = previous_block->previous();
    }
  }
}

NodeInput *TimelineOutput::block_input()
{
  return block_input_;
}

NodeOutput *TimelineOutput::texture_output()
{
  return texture_output_;
}

void TimelineOutput::Process(const rational &time)
{
  // This node is intended to connect to the end of the timeline, so being beyond its out point is considered the end
  // of the sequence
  if (attached_block() == nullptr || time >= attached_block()->out()) {
    texture_output_->set_value(0);
    current_block_ = nullptr;
    return;
  }

  // If we're here, we need to find the current clip to display
  if (current_block_ == nullptr) {
    current_block_ = attached_block();
  }

  // If the time requested is an earlier Block, traverse earlier until we find it
  while (time < current_block_->in()) {
    current_block_ = current_block_->previous();
  }

  // If the time requested is in a later Block, traverse later
  while (time >= current_block_->out()) {
    current_block_ = current_block_->next();
  }

  // At this point, we must have found the correct block so we use its texture output to produce the image
  texture_output_->set_value(current_block_->texture_output()->get_value(time));
}

Block *TimelineOutput::attached_block()
{
  return ValueToPtr<Block>(block_input_->get_value(0));
}
