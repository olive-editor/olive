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

#ifndef TRACKLIST_H
#define TRACKLIST_H

#include <QObject>

#include "node/graph.h"
#include "node/output/track/track.h"
#include "timeline/timelinecommon.h"

namespace olive {

class ViewerOutput;

class TrackList : public QObject
{
  Q_OBJECT
public:
  TrackList(ViewerOutput *parent, const Track::Type& type, NodeInput* track_input);

  const Track::Type& type() const
  {
    return type_;
  }

  const QVector<Track*>& GetTracks() const
  {
    return track_cache_;
  }

  Track* GetTrackAt(int index) const;

  const rational& GetTotalLength() const
  {
    return total_length_;
  }

  int GetTrackCount() const
  {
    return track_cache_.size();
  }

  NodeGraph* GetParentGraph() const;

  NodeInput* track_input() const
  {
    return track_input_;
  }

signals:
  void TrackListChanged();

  void LengthChanged(const rational &length);

  void TrackAdded(Track* track);

  void TrackRemoved(Track* track);

private:
  void UpdateTrackIndexesFrom(int index);

  /**
   * @brief A cache of connected Tracks
   */
  QVector<Track*> track_cache_;

  NodeInput* track_input_;

  rational total_length_;

  enum Track::Type type_;

private slots:
  /**
   * @brief Slot for when the track connection is added
   */
  void TrackConnected(Node* node, int element);

  /**
   * @brief Slot for when the track connection is removed
   */
  void TrackDisconnected(Node* node, int element);

  /**
   * @brief Slot for when any of the track's length changes so we can update the length of the tracklist
   */
  void UpdateTotalLength();

};

}

#endif // TRACKLIST_H
