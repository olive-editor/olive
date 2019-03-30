#ifndef TRACKLIST_H
#define TRACKLIST_H

#include "track.h"

class TrackList : public QObject
{
  Q_OBJECT
public:
  TrackList(Sequence* parent, Track::Type type);
  TrackList* copy(Sequence* parent);

  void AddTrack();
  void RemoveTrack(int i);
  Track* First();
  int TrackCount();
  QVector<Track*> tracks();

  Sequence* GetParent();

private:
  void ResizeTrackArray(int i);

  Track::Type type_;
  QVector<Track*> tracks_;
};

#endif // TRACKLIST_H
