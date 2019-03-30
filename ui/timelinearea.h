#ifndef TIMELINEAREA_H
#define TIMELINEAREA_H

#include "timeline/sequence.h"
#include "timeline/track.h"
#include "timeline/tracklist.h"
#include "ui/timelineview.h"
#include "ui/timelinelabel.h"

class TimelineArea : public QWidget
{
  Q_OBJECT
public:
  TimelineArea();

  void SetTrackList(Sequence* sequence, Track::Type track_list);
public slots:
  void RefreshLabels();
private:
  TrackList* track_list_;
  TimelineView* view_;
  QVector<TimelineLabel> labels_;
};

#endif // TIMELINEAREA_H
