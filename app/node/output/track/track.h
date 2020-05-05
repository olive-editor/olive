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

#ifndef TRACKOUTPUT_H
#define TRACKOUTPUT_H

#include "common/timelinecommon.h"
#include "node/block/block.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A time traversal Node for sorting through one channel/track of Blocks
 */
class TrackOutput : public Block
{
  Q_OBJECT
public:
  TrackOutput();

  const Timeline::TrackType& track_type();
  void set_track_type(const Timeline::TrackType& track_type);

  virtual Type type() const override;

  virtual Block* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QList<CategoryID> Category() const override;
  virtual QString Description() const override;

  QString GetTrackName();

  const int& GetTrackHeight() const;
  void SetTrackHeight(const int& height);

  virtual void Retranslate() override;

  const int& Index();
  void SetIndex(const int& index);

  Block* BlockContainingTime(const rational& time) const;

  /**
   * @brief Returns the block that starts BEFORE a given time and ends some time AFTER or precisely AT that time
   */
  Block* NearestBlockBefore(const rational& time) const;

  /**
   * @brief Returns the block that starts BEFORE a given time OR the block that starts precisely at that time
   */
  Block* NearestBlockBeforeOrAt(const rational& time) const;

  /**
   * @brief Returns the block that starts either precisely AT a given time or the soonest block AFTER
   */
  Block* NearestBlockAfterOrAt(const rational& time) const;

  /**
   * @brief Returns the block that starts AFTER the given time (but never AT the given time)
   */
  Block* NearestBlockAfter(const rational& time) const;

  Block* BlockAtTime(const rational& time) const;
  QList<Block*> BlocksAtTimeRange(const TimeRange& range) const;

  const QList<Block *> &Blocks() const;

  virtual void InvalidateCache(const TimeRange& range, NodeInput* from, NodeInput *source) override;

  /**
   * @brief Adds Block `block` at the very beginning of the Sequence before all other clips
   */
  void PrependBlock(Block* block);

  /**
   * @brief Inserts Block `block` at a specific index (0 is the start of the timeline)
   *
   * If the index == 0, this function does the same as PrependBlock(). If the index >= the current number of blocks,
   * this function is the same as AppendBlock().
   */
  void InsertBlockAtIndex(Block* block, int index);

  /**
   * @brief Inserts Block after another Block
   *
   * Equivalent to calling InsertBlockBetweenBlocks(block, before, before->next())
   */
  void InsertBlockAfter(Block* block, Block* before);

  /**
   * @brief Inserts Block before another Block
   */
  void InsertBlockBefore(Block* block, Block* after);

  /**
   * @brief Adds Block `block` at the very end of the Sequence after all other clips
   */
  void AppendBlock(Block* block);

  /**
   * @brief Removes a Block pushing all subsequent Blocks earlier to take up the space
   */
  void RippleRemoveBlock(Block* block);

  /**
   * @brief Replaces Block `old` with Block `replace`
   *
   * Both blocks must have equal lengths.
   */
  void ReplaceBlock(Block* old, Block* replace);

  void BlockInvalidateCache();

  void UnblockInvalidateCache();

  static TrackOutput* TrackFromBlock(const Block *block);

  const rational& track_length() const;

  virtual bool IsTrack() const override;

  static int GetTrackHeightIncrement();

  static int GetDefaultTrackHeight();

  static int GetTrackHeightMinimum();

  static QString GetDefaultTrackName(Timeline::TrackType type, int index);

  bool IsMuted() const;

  bool IsLocked() const;

  NodeInputArray* block_input() const;

  virtual void Hash(QCryptographicHash& hash, const rational &time) const override;

public slots:
  void SetTrackName(const QString& name);

  void SetMuted(bool e);

  void SetLocked(bool e);

signals:
  /**
   * @brief Signal emitted when a Block is added to this Track
   */
  void BlockAdded(Block* block);

  /**
   * @brief Signal emitted when a Block is removed from this Track
   */
  void BlockRemoved(Block* block);

  /**
   * @brief Signal emitted when the length of the track has changed
   */
  void TrackLengthChanged();

  /**
   * @brief Signal emitted when the height of the track has changed
   */
  void TrackHeightChanged(int height);

  /**
   * @brief Signal emitted when the muted setting changes
   */
  void MutedChanged(bool e);

  /**
   * @brief Signal emitted when the index has changed
   */
  void IndexChanged(int i);

protected:

private:
  void UpdateInOutFrom(int index);

  int GetInputIndexFromCacheIndex(int cache_index);
  int GetInputIndexFromCacheIndex(Block* block);

  QList<Block*> block_cache_;

  NodeInputArray* block_input_;

  NodeInput* muted_input_;

  Timeline::TrackType track_type_;

  rational track_length_;

  int track_height_;

  QString track_name_;

  int block_invalidate_cache_stack_;

  int index_;

  bool locked_;

private slots:
  void BlockConnected(NodeEdgePtr edge);

  void BlockDisconnected(NodeEdgePtr edge);

  void BlockLengthChanged();

  void MutedInputValueChanged();

};

OLIVE_NAMESPACE_EXIT

#endif // TRACKOUTPUT_H
