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

#ifndef TRACKOUTPUT_H
#define TRACKOUTPUT_H

#include "audio/audiovisualwaveform.h"
#include "node/block/block.h"
#include "timeline/timelinecommon.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A time traversal Node for sorting through one channel/track of Blocks
 */
class TrackOutput : public Node
{
  Q_OBJECT
public:
  TrackOutput();

  virtual ~TrackOutput() override;

  const Timeline::TrackType& track_type() const;
  void set_track_type(const Timeline::TrackType& track_type);

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  const double& GetTrackHeight() const;
  void SetTrackHeight(const double& height);

  int GetTrackHeightInPixels() const
  {
    return InternalHeightToPixelHeight(GetTrackHeight());
  }

  void SetTrackHeightInPixels(int h)
  {
    SetTrackHeight(PixelHeightToInternalHeight(h));
  }

  static int InternalHeightToPixelHeight(double h)
  {
    return qRound(h * QFontMetrics(QFont()).height());
  }

  static double PixelHeightToInternalHeight(int h)
  {
    return double(h) / double(QFontMetrics(QFont()).height());
  }

  static int GetDefaultTrackHeightInPixels()
  {
    return InternalHeightToPixelHeight(kTrackHeightDefault);
  }

  static int GetMinimumTrackHeightInPixels()
  {
    return InternalHeightToPixelHeight(kTrackHeightMinimum);
  }

  virtual void Retranslate() override;

  const int& Index();
  void SetIndex(const int& index);

  /**
   * @brief Returns the block that starts BEFORE (not AT) and ends AFTER (not AT) a time
   *
   * Catches the first block that matches `block.in < time && block.out > time` or nullptr if any
   * block starts/ends precisely at that time or the time exceeds the track length.
   */
  Block* BlockContainingTime(const rational& time) const;

  /**
   * @brief Returns the block that starts BEFORE a given time and ends either AFTER or AT that time
   *
   * @return Catches the first block that matches `block.out >= time` or nullptr if this time
   * exceeds the track length.
   */
  Block* NearestBlockBefore(const rational& time) const;

  /**
   * @brief Returns the block that starts BEFORE or AT a given time.
   *
   * @return Catches the first block that matches `block.out > time` or nullptr if this time
   * exceeds the track length.
   */
  Block* NearestBlockBeforeOrAt(const rational& time) const;

  /**
   * @brief Returns the block that starts either AT a given time or the soonest block AFTER
   *
   * @return Catches the first block that matches `block.in >= time` or nullptr if this time
   * exceeds the track length.
   */
  Block* NearestBlockAfterOrAt(const rational& time) const;

  /**
   * @brief Returns the block that starts AFTER the given time (but never AT the given time)
   *
   * @return Catches the first block that matches `block.in > time` or nullptr if this time
   * exceeds the track length.
   */
  Block* NearestBlockAfter(const rational& time) const;

  /**
   * @brief Returns the block that should be rendered/visible at a given time
   *
   * Use this for any video rendering or determining which block will actually be active at any
   * time.
   *
   * @return Catches the first block that matches `block.in <= time && block.out > time`. Returns
   * nullptr if the time exceeds the track length, the block active at this time is disabled, or
   * if IsMuted() is true.
   */
  Block* BlockAtTime(const rational& time) const;

  /**
   * @brief Returns a list of blocks that should be rendered/visible during a given time range
   *
   * Use this for audio rendering to determine all blocks that will be active throughout a range
   * of time.
   *
   * @return Similar to BlockAtTime() but will match several blocks where
   * `block.in < range.out && block.out > range.in`. Returns an empty list if IsMuted() or if
   * `range.in >= track.length`. Blocks that are not enabled will be omitted from the returned list.
   */
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

  static TrackOutput* TrackFromBlock(const Block *block);

  const rational& track_length() const;

  virtual bool IsTrack() const override;

  static QString GetDefaultTrackName(Timeline::TrackType type, int index);

  bool IsMuted() const;

  bool IsLocked() const;

  NodeInputArray* block_input() const;

  virtual void Hash(QCryptographicHash& hash, const rational &time) const override;

  AudioVisualWaveform& waveform()
  {
    return waveform_;
  }

  static const double kTrackHeightDefault;
  static const double kTrackHeightMinimum;
  static const double kTrackHeightInterval;

public slots:
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
  void TrackHeightChangedInPixels(int pixel_height);

  /**
   * @brief Signal emitted when the muted setting changes
   */
  void MutedChanged(bool e);

  /**
   * @brief Signal emitted when the index has changed
   */
  void IndexChanged(int i);

  /**
   * @brief Signal emitted when preview (waveform) has changed and UI should be updated
   */
  void PreviewChanged();

protected:
  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data) override;

  virtual void SaveInternal(QXmlStreamWriter* writer) const override;

private:
  void UpdateInOutFrom(int index);

  int GetInputIndexFromCacheIndex(int cache_index);
  int GetInputIndexFromCacheIndex(Block* block);

  void SetLengthInternal(const rational& r, bool invalidate = true);

  QList<Block*> block_cache_;

  NodeInputArray* block_input_;

  NodeInput* muted_input_;

  Timeline::TrackType track_type_;

  rational track_length_;

  double track_height_;

  int index_;

  bool locked_;

  AudioVisualWaveform waveform_;

private slots:
  void BlockConnected(NodeEdgePtr edge);

  void BlockDisconnected(NodeEdgePtr edge);

  void BlockLengthChanged();

  void MutedInputValueChanged();

};

OLIVE_NAMESPACE_EXIT

#endif // TRACKOUTPUT_H
