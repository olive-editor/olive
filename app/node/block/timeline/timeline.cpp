#include "timeline.h"

TimelineBlock::TimelineBlock() :
  current_block_(nullptr)
{
}

rational TimelineBlock::length()
{
  return 0;
}

void TimelineBlock::Process(const rational &time)
{
  // Run default process function
  Block::Process(time);

    // This node represents the end of the timeline, so if the time is beyond its start, there's no image to display
  if (time >= in()) {
    texture_output_->set_value(0);
    current_block_ = nullptr;
    return;
  }

  // If we're here, we need to find the current clip to display
  if (current_block_ == nullptr) {
    current_block_ = this;
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
