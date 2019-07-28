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

#ifndef BLOCK_H
#define BLOCK_H

#include "node/node.h"

/**
 * @brief A Node that represents a block of time, also displayable on a Timeline
 *
 * This is an abstract function. Since different types of Block will provide their lengths in different ways, it's
 * necessary to subclass and override the length() function for a Block to be usable.
 */
class Block : public Node
{
  Q_OBJECT
public:
  Block();

  virtual QString Category() override;

  const rational& in();
  const rational& out();

  virtual rational length() = 0;
  virtual void set_length(const rational& length);

  virtual Block* previous();
  virtual Block* next();

  NodeInput* previous_input();
  NodeInput* next_input();

  NodeOutput* texture_output();
  NodeOutput* block_output();

public slots:
  virtual void Process(const rational &time) override;

  /**
   * @brief Refreshes internal cache of in/out points up to date
   *
   * A block can only know truly know its in point by adding all the lengths of the clips before it. Since this can
   * become timeconsuming, blocks cache their in and out points for easy access, however this does mean their caches
   * need to stay up to date to provide accurate results. Whenever this or any surrounding Block is changed, it's
   * recommended to call Refresh().
   *
   * This function specifically sets the in point to the out point of the previous clip and sets its out point to the
   * in point + this block's length. Therefore, before calling Refresh() on a Block, it's necessary that all the
   * Blocks before it are accurate and up to date. You may need to traverse through the Block list (using previous())
   * and run Refresh() on all Blocks sequentially.
   */
  void Refresh();

protected:
  void RefreshSurrounds();

private:
  NodeInput* previous_input_;
  NodeInput* next_input_;
  NodeOutput* block_output_;

  NodeOutput* texture_output_;

  rational in_point_;
  rational out_point_;
};

#endif // BLOCK_H
