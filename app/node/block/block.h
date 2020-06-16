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
#include "timeline/timelinecommon.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A Node that represents a block of time, also displayable on a Timeline
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

  virtual void Hash(QCryptographicHash &hash, const rational &time) const override;

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

  void EnabledChanged();

protected:
  rational SequenceToMediaTime(const rational& sequence_time) const;

  rational MediaToSequenceTime(const rational& media_time) const;

  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data) override;

  virtual void SaveInternal(QXmlStreamWriter* writer) const override;

  virtual QList<NodeInput*> GetInputsToHash() const override;

  virtual void LengthChangedEvent(const rational& old_length,
                                  const rational& new_length,
                                  const Timeline::MovementMode& mode);

  Block* previous_;
  Block* next_;

private:
  void set_length_internal(const rational &length);

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
