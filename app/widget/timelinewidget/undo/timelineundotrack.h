/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef TIMELINEUNDOTRACK_H
#define TIMELINEUNDOTRACK_H

#include "node/output/track/track.h"

namespace olive {

class TrackRippleRemoveBlockCommand : public UndoCommand
{
public:
  TrackRippleRemoveBlockCommand(Track* track, Block* block) :
    track_(track),
    block_(block)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

protected:
  virtual void redo() override
  {
    before_ = block_->previous();
    track_->RippleRemoveBlock(block_);
  }

  virtual void undo() override
  {
    track_->InsertBlockAfter(block_, before_);
  }

private:
  Track* track_;

  Block* block_;

  Block* before_;

};

class TrackPrependBlockCommand : public UndoCommand
{
public:
  TrackPrependBlockCommand(Track* track, Block* block) :
    track_(track),
    block_(block)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

protected:
  virtual void redo() override
  {
    track_->PrependBlock(block_);
  }

  virtual void undo() override
  {
    track_->RippleRemoveBlock(block_);
  }

private:
  Track* track_;
  Block* block_;
};

class TrackInsertBlockAfterCommand : public UndoCommand
{
public:
  TrackInsertBlockAfterCommand(Track* track, Block* block, Block* before) :
    track_(track),
    block_(block),
    before_(before)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return block_->project();
  }

protected:
  virtual void redo() override
  {
    track_->InsertBlockAfter(block_, before_);
  }

  virtual void undo() override
  {
    track_->RippleRemoveBlock(block_);
  }

private:
  Track* track_;

  Block* block_;

  Block* before_;
};

/**
 * @brief Replaces Block `old` with Block `replace`
 *
 * Both blocks must have equal lengths.
 */
class TrackReplaceBlockCommand : public UndoCommand
{
public:
  TrackReplaceBlockCommand(Track* track, Block* old, Block* replace) :
    track_(track),
    old_(old),
    replace_(replace)
  {
  }

  virtual Project* GetRelevantProject() const override
  {
    return track_->project();
  }

protected:
  virtual void redo() override
  {
    track_->ReplaceBlock(old_, replace_);
  }

  virtual void undo() override
  {
    track_->ReplaceBlock(replace_, old_);
  }

private:
  Track* track_;
  Block* old_;
  Block* replace_;

};

}

#endif // TIMELINEUNDOTRACK_H
