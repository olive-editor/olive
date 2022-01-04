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

#ifndef TRACK_H
#define TRACK_H

#include "node/block/block.h"
#include "timeline/timelinecommon.h"

namespace olive {

class Sequence;

/**
 * @brief A time traversal Node for sorting through one channel/track of Blocks
 */
class Track : public Node
{
  Q_OBJECT
public:
  enum Type {
    kNone = -1,
    kVideo,
    kAudio,
    kSubtitle,
    kCount
  };

  Track();

  NODE_DEFAULT_DESTRUCTOR(Track)

  const Track::Type& type() const;
  void set_type(const Track::Type& track_type);

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  static rational TransformTimeForBlock(const Block* block, const rational& time)
  {
    if (time == RATIONAL_MAX || time == RATIONAL_MIN) {
      return time;
    }

    return time - block->in();
  }

  static TimeRange TransformRangeForBlock(const Block* block, const TimeRange& range)
  {
    return TimeRange(TransformTimeForBlock(block, range.in()), TransformTimeForBlock(block, range.out()));
  }

  static rational TransformTimeFromBlock(const Block* block, const rational& time)
  {
    if (time == RATIONAL_MAX || time == RATIONAL_MIN) {
      return time;
    }

    return time + block->in();
  }

  static TimeRange TransformRangeFromBlock(const Block* block, const TimeRange& range)
  {
    return TimeRange(TransformTimeFromBlock(block, range.in()), TransformTimeFromBlock(block, range.out()));
  }

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

  class Reference
  {
  public:
    Reference() :
      type_(kNone),
      index_(-1)
    {
    }

    Reference(const Track::Type& type, const int& index) :
      type_(type),
      index_(index)
    {
    }

    const Track::Type& type() const
    {
      return type_;
    }

    const int& index() const
    {
      return index_;
    }

    bool operator==(const Reference& ref) const
    {
      return type_ == ref.type_ && index_ == ref.index_;
    }

    bool operator!=(const Reference& ref) const
    {
      return !(*this == ref);
    }

    bool operator<(const Track::Reference& rhs) const
    {
      if (type_ != rhs.type_) {
        return type_ < rhs.type_;
      }

      return index_ < rhs.index_;
    }

    QString ToString() const
    {
      QString type_string = TypeToString(type_);
      if (type_string.isEmpty()) {
        return QString();
      } else {
        return QStringLiteral("%1:%2").arg(type_string, QString::number(index_));
      }
    }

    /// For IDs that shouldn't change between localizations
    static QString TypeToString(Type type)
    {
      switch (type) {
      case kVideo:
        return QStringLiteral("v");
      case kAudio:
        return QStringLiteral("a");
      case kSubtitle:
        return QStringLiteral("s");
      case kCount:
      case kNone:
        break;
      }

      return QString();
    }

    /// For human-facing strings
    static QString TypeToTranslatedString(Type type)
    {
      switch (type) {
      case kVideo:
        return tr("V");
      case kAudio:
        return tr("A");
      case kSubtitle:
        return tr("S");
      case kCount:
      case kNone:
        break;
      }

      return QString();
    }

    static Type TypeFromString(const QString& s)
    {
      if (s.size() >= 3) {
        if (s.at(1) == ':') {
          if (s.at(0) == 'v') {
            // Video stream
            return Track::kVideo;
          } else if (s.at(0) == 'a') {
            // Audio stream
            return Track::kAudio;
          }
        }
      }

      return Track::kNone;
    }

    static Reference FromString(const QString& s)
    {
      Reference ref;
      Type parse_type = TypeFromString(s);

      if (parse_type != Track::kNone) {
        bool ok;
        int parse_index = s.mid(2).toInt(&ok);

        if (ok) {
          ref.type_ = parse_type;
          ref.index_ = parse_index;
        }
      }

      return ref;
    }

    bool IsValid() const
    {
      return type_ > kNone && type_ < kCount && index_ >= 0;
    }

  private:
    Track::Type type_;

    int index_;

  };

  Reference ToReference() const
  {
    return Reference(type(), Index());
  }

  const int& Index() const
  {
    return index_;
  }

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
  QVector<Block*> BlocksAtTimeRange(const TimeRange& range) const;

  const QVector<Block *> &Blocks() const
  {
    return blocks_;
  }

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options) override;

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

  rational track_length() const;

  bool IsMuted() const;

  bool IsLocked() const;

  int GetArrayIndexFromBlock(Block* block) const;

  Sequence *sequence() const
  {
    return sequence_;
  }

  void set_sequence(Sequence *sequence)
  {
    sequence_ = sequence;
  }

  static const double kTrackHeightDefault;
  static const double kTrackHeightMinimum;
  static const double kTrackHeightInterval;

  static const QString kBlockInput;
  static const QString kMutedInput;

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
  void IndexChanged(int old, int now);

  /**
   * @brief Emitted when a block changes length and all the subsequent blocks had to update
   */
  void BlocksRefreshed();

protected:
  virtual void Hash(QCryptographicHash& hash, const NodeGlobals &globals, const VideoParams& video_params) const override;

  virtual bool LoadCustom(QXmlStreamReader* reader, XMLNodeData& xml_node_data, uint version, const QAtomicInt* cancelled) override;

  virtual void SaveCustom(QXmlStreamWriter* writer) const override;

  virtual void InputConnectedEvent(const QString& input, int element, Node *output) override;

  virtual void InputDisconnectedEvent(const QString& input, int element, Node *output) override;

  virtual void InputValueChangedEvent(const QString& input, int element) override;

private:
  void UpdateInOutFrom(int index);

  int GetArrayIndexFromCacheIndex(int index) const;

  int GetCacheIndexFromArrayIndex(int index) const;

  TimeRangeList block_length_pending_invalidations_;

  QVector<Block*> blocks_;
  QVector<int> block_array_indexes_;

  Track::Type track_type_;

  double track_height_;

  int index_;

  bool locked_;

  Sequence *sequence_;

private slots:
  void BlockLengthChanged();

};

uint qHash(const Track::Reference& r, uint seed = 0);

QDataStream &operator<<(QDataStream &out, const Track::Reference &ref);

QDataStream &operator>>(QDataStream &in, Track::Reference &ref);

}

#endif // TRACK_H
