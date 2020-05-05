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

OLIVE_NAMESPACE_ENTER

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
    kTransition
  };

  virtual Type type() const = 0;

  virtual QList<CategoryID> Category() const override;

  const rational& in() const;
  const rational& out() const;
  void set_in(const rational& in);
  void set_out(const rational& out);

  rational length() const;
  void set_length_and_media_out(const rational &length);
  void set_length_and_media_in(const rational &length);

  Block* previous();
  Block* next();
  void set_previous(Block* previous);
  void set_next(Block* next);

  rational media_in() const;
  void set_media_in(const rational& media_in);

  rational media_out() const;

  rational speed() const;
  void set_speed(const rational& speed);
  bool is_still() const;
  bool is_reversed() const;

  bool is_enabled() const;
  void set_enabled(bool e);

  QString block_name() const;
  void set_block_name(const QString& name);

  static bool Link(Block* a, Block* b);
  static void Link(const QList<Block*>& blocks);
  static bool Unlink(Block* a, Block* b);
  static void Unlink(const QList<Block*>& blocks);
  static bool AreLinked(Block* a, Block* b);
  const QVector<Block*>& linked_clips();
  bool HasLinks();

  virtual bool IsBlock() const override;

  virtual void Retranslate() override;

  NodeInput* length_input() const;
  NodeInput* media_in_input() const;
  NodeInput* speed_input() const;

  virtual void InvalidateCache(const TimeRange& range, NodeInput* from, NodeInput* source) override;

public slots:

signals:
  /**
   * @brief Signal emitted when this Block is refreshed
   *
   * Can be used as essentially a "changed" signal for UI widgets to know when to update their views
   */
  void Refreshed();

  void LengthChanged(const rational& length);

  void LinksChanged();

  void NameChanged();

  void EnabledChanged();

protected:
  rational SequenceToMediaTime(const rational& sequence_time) const;

  rational MediaToSequenceTime(const rational& media_time) const;

  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data) override;

  virtual void SaveInternal(QXmlStreamWriter* writer) const override;

  virtual QList<NodeInput*> GetInputsToHash() const override;

  Block* previous_;
  Block* next_;

private:
  NodeInput* name_input_;
  NodeInput* length_input_;
  NodeInput* media_in_input_;
  NodeInput* speed_input_;
  NodeInput* enabled_input_;

  rational in_point_;
  rational out_point_;

  QVector<Block*> linked_clips_;

private slots:
  void LengthInputChanged();

};

OLIVE_NAMESPACE_EXIT

#endif // BLOCK_H
