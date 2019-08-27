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

  enum Type {
    kClip,
    kGap,
    kEnd
  };

  virtual Block* copy() = 0;

  virtual Type type() = 0;

  virtual QString Category() override;

  const rational& in();
  const rational& out();

  virtual const rational &length();
  virtual void set_length(const rational &length);

  Block* previous();
  Block* next();

  NodeInput* previous_input();

  NodeOutput* texture_output();
  NodeOutput* block_output();

  static void ConnectBlocks(Block* previous, Block* next);
  static void DisconnectBlocks(Block* previous, Block* next);

  const rational& media_in();
  void set_media_in(const rational& media_in);

  /**
   * @brief Override removes previous input as that is not a direct dependency
   */
  virtual QList<NodeDependency> RunDependencies(NodeOutput* output, const rational &time) override;

public slots:
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
  virtual void Refresh();

  /**
   * @brief Calls Refresh() on this block and all blocks connected after it (but not before it)
   */
  void RefreshFollowing();

signals:
  /**
   * @brief Signal emitted when this Block is refreshed
   *
   * Can be used as essentially a "changed" signal for UI widgets to know when to update their views
   */
  void Refreshed();

protected:
  virtual QVariant Value(NodeOutput* output, const rational& time) override;

private:  
  NodeInput* previous_input_;
  NodeOutput* block_output_;

  NodeOutput* texture_output_;

  rational in_point_;
  rational out_point_;

  rational length_;

  rational media_in_;

  Block* next_;

private slots:
  void EdgeAddedSlot(NodeEdgePtr edge);

  void EdgeRemovedSlot(NodeEdgePtr edge);

};

#endif // BLOCK_H
