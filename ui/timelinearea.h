#ifndef TIMELINEAREA_H
#define TIMELINEAREA_H

#include "timeline/sequence.h"
#include "timeline/track.h"
#include "timeline/tracklist.h"
#include "timeline/timelinefunctions.h"
#include "ui/timelineview.h"
#include "ui/timelinelabel.h"

class TimelineArea : public QWidget
{
  Q_OBJECT
public:
  TimelineArea(Timeline *timeline, olive::timeline::Alignment alignment = olive::timeline::kAlignmentTop);

  void SetTrackList(Sequence* sequence, Track::Type track_list);
public slots:
  void RefreshLabels();
private:
  Timeline* timeline_;
  TrackList* track_list_;
  TimelineView* view_;
  QVector<TimelineLabelPtr> labels_;
  olive::timeline::Alignment alignment_;
  QVBoxLayout* label_container_layout_;
};

#endif // TIMELINEAREA_H
