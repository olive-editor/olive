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

#ifndef TRACKLIST_H
#define TRACKLIST_H

#include <QObject>

#include "common/timelinecommon.h"
#include "node/graph.h"
#include "node/output/track/track.h"

class ViewerOutput;

class TrackList : public QObject {
  Q_OBJECT
public:
  TrackList(ViewerOutput *parent, const Timeline::TrackType& type, NodeInputArray* track_input);

  const Timeline::TrackType& type() const;

  const QVector<TrackOutput*>& Tracks() const;

  TrackOutput* TrackAt(int index) const;

  TrackOutput *AddTrack();

  void RemoveTrack();

  const rational& TrackLength() const;

  int TrackCount() const;

signals:
  void BlockAdded(Block* block, int index);

  void BlockRemoved(Block* block);

  void TrackAdded(TrackOutput* track);

  void TrackRemoved(TrackOutput* track);

  void TrackListChanged();

  void LengthChanged(const rational &length);

  void TrackHeightChanged(int index, int height);

private:
  NodeGraph* GetParentGraph() const;

  /**
   * @brief A cache of connected Tracks
   */
  QVector<TrackOutput*> track_cache_;

  NodeInputArray* track_input_;

  rational total_length_;

  enum Timeline::TrackType type_;

private slots:
  /**
   * @brief Slot for when the track connection is added
   */
  void TrackConnected(NodeEdgePtr edge);

  /**
   * @brief Slot for when the track connection is removed
   */
  void TrackDisconnected(NodeEdgePtr edge);

  /**
   * @brief Slot for when a connected Track has added a Block so we can update the UI
   */
  void TrackAddedBlock(Block* block);

  /**
   * @brief Slot for when a connected Track has added a Block so we can update the UI
   */
  void TrackRemovedBlock(Block* block);

  /**
   * @brief Slot for when the count of tracks in the track input changes
   */
  void TrackListSizeChanged(int size);

  /**
   * @brief Slot for when any of the track's length changes so we can update the length of the tracklist
   */
  void UpdateTotalLength();

  /**
   * @brief Slot when a track height changes, transforms to the TrackHeightChanged signal which includes a track index
   */
  void TrackHeightChangedSlot(int height);

};

#endif // TRACKLIST_H
