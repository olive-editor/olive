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

#include "node/graph.h"
#include "node/output/track/track.h"
#include "timeline/tracktypes.h"

class TimelineOutput;

class TrackList : public QObject {
  Q_OBJECT
public:
  TrackList(TimelineOutput *parent, const enum TrackType& type, NodeInput* track_input);

  TrackOutput* attached_track();

  void AttachTrack(TrackOutput *track);

  void DetachTrack(TrackOutput* track);

  const QVector<TrackOutput*>& Tracks();

  TrackOutput* TrackAt(int index);

  void AddTrack();

  void RemoveTrack();

  const rational& TrackLength();

  const enum TrackType& TrackType();

public slots:/**
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

signals:
  void BlockAdded(Block* block, int index);

  void BlockRemoved(Block* block);

  void TrackAdded(TrackOutput* track);

  void TrackRemoved(TrackOutput* track);

  void TrackListChanged();

  void LengthChanged(const rational &length);

private:
  NodeGraph* GetParentGraph();

  /**
   * @brief A cache of connected Tracks
   */
  QVector<TrackOutput*> track_cache_;

  NodeInput* track_input_;

  rational total_length_;

  enum TrackType type_;

private slots:
  void UpdateTotalLength();
};

#endif // TRACKLIST_H
