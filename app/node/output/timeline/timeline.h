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

#ifndef TIMELINEOUTPUT_H
#define TIMELINEOUTPUT_H

#include "node/block/block.h"
#include "node/output/track/track.h"
#include "panel/timeline/timeline.h"

/**
 * @brief Node that represents the end of the Timeline as well as a time traversal Node
 */
class TimelineOutput : public Node
{
  Q_OBJECT
public:
  TimelineOutput();

  virtual QString Name() override;
  virtual QString id() override;
  virtual QString Category() override;
  virtual QString Description() override;

  void AttachTimeline(TimelinePanel* timeline);

  NodeInput* track_input();

public slots:
  virtual void Process(const rational &time) override;

private:
  int GetTrackIndex(TrackOutput* track);

  rational GetSequenceLength();

  TrackOutput* attached_track();

  void AttachTrack(TrackOutput *track);

  void DetachTrack(TrackOutput* track);

  void AddTrack();

  TimelinePanel* attached_timeline_;

  NodeInput* track_input_;

  /**
   * @brief A cache of connected Tracks
   */
  QVector<TrackOutput*> track_cache_;

private slots:
  /**
   * @brief Slot for when the track connection is added
   */
  void TrackConnectionAdded(NodeEdgePtr edge);

  /**
   * @brief Slot for when the track connection is removed
   */
  void TrackConnectionRemoved(NodeEdgePtr edge);

  /**
   * @brief Slot for when a connected Track has added a Block so we can update the UI
   */
  void TrackAddedBlock(Block* block);

  /**
   * @brief Slot for when a connected Track has added a Block so we can update the UI
   */
  void TrackRemovedBlock(Block* block);

  /**
   * @brief Slot for when an attached Track has an edge added
   */
  void TrackEdgeAdded(NodeEdgePtr edge);

  /**
   * @brief Slot for when an attached Track has an edge added
   */
  void TrackEdgeRemoved(NodeEdgePtr edge);

  /**
   * @brief Forwards a PlaceBlock signal to the requested track
   *
   * If the track index doesn't exist, tracks are automatically created until a track at that index does exist
   * (provided the track index is positive). A negative track index fails immediately.
   */
  void PlaceBlock(Block* block, rational start, int track);

  /**
   * @brief Forwards a ReplaceBlock signal to the appropriate track
   */
  void ReplaceBlock(Block* old, Block* replace, int track);

  /**
   * @brief Forwards a SplitAtTime signal to the appropriate track
   */
  void SplitAtTime(rational time, int track);

  /**
   * @brief Resizes a Block
   */
  void ResizeBlock(Block* block, rational new_length);

};

#endif // TIMELINEOUTPUT_H
