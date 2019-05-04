#ifndef TRACKLIST_H
#define TRACKLIST_H

#include "track.h"

class TrackList : public QObject
{
  Q_OBJECT
public:
  TrackList(Sequence* parent, olive::TrackType type);
  TrackList* copy(Sequence* parent);

  void Save(QXmlStreamWriter& stream);

  void AddTrack();
  void RemoveTrack(int i);
  Track* First();
  Track* Last();
  int TrackCount();
  int IndexOfTrack(Track* track);
  Track* TrackAt(int i);
  QVector<Track*> tracks();

  olive::TrackType type();

  Sequence* GetParent();

signals:
  void TrackCountChanged();

private:
  void ResizeTrackArray(int i);

  olive::TrackType type_;
  QVector<Track*> tracks_;
};

#endif // TRACKLIST_H
