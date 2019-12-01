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
 *
 * When overriding Node::copy(), the derivative class should also call Block::CopyParameters() on the new Block instance
 * which will copy the block's name, length, and media in point. It does not copy any node-specific parameters like any
 * input values or connections as per standard with Node::copy().
 */
class Block : public Node
{
  Q_OBJECT
public:
  Block();

  enum Type {
    kClip,
    kGap,
    kTrack
  };

  virtual Type type() = 0;

  virtual QString Category() override;

  const rational& in();
  const rational& out();
  void set_in(const rational& in);
  void set_out(const rational& out);

  const rational &length();
  void set_length(const rational &length);
  void set_length_and_media_in(const rational &length);

  Block* previous();
  Block* next();
  void set_previous(Block* previous);
  void set_next(Block* next);

  NodeOutput* buffer_output();
  NodeOutput* block_output();

  const rational& media_in();
  void set_media_in(const rational& media_in);

  const QString& block_name();
  void set_block_name(const QString& name);

  static void Link(Block* a, Block* b);
  static void Link(QList<Block*> blocks);
  static void Unlink(Block* a, Block* b);
  static bool AreLinked(Block* a, Block* b);
  const QVector<Block*>& linked_clips();
  bool HasLinks();

  virtual bool IsBlock() override;

  virtual QVariant Value(NodeOutput* output) override;

public slots:

signals:
  /**
   * @brief Signal emitted when this Block is refreshed
   *
   * Can be used as essentially a "changed" signal for UI widgets to know when to update their views
   */
  void Refreshed();

  void LengthChanged(const rational& length);

protected:
  rational SequenceToMediaTime(const rational& sequence_time);

  rational MediaToSequenceTime(const rational& media_time);

  static void CopyParameters(Block* source, Block* dest);

  Block* previous_;
  Block* next_;

private:
  NodeOutput* block_output_;
  NodeOutput* buffer_output_;

  rational length_;
  rational media_in_;

  rational in_point_;
  rational out_point_;

  QString block_name_;

  QVector<Block*> linked_clips_;

};

#endif // BLOCK_H
