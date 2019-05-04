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

  void SetTrackList(Sequence* sequence, olive::TrackType track_list);
  void SetAlignment(olive::timeline::Alignment alignment);

  TrackList* track_list();
  TimelineView* view();

  virtual void wheelEvent(QWheelEvent *event) override;
public slots:
  void RefreshLabels();
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
private:
  Timeline* timeline_;
  TrackList* track_list_;
  TimelineView* view_;
  QVector<TimelineLabelPtr> labels_;
  olive::timeline::Alignment alignment_;
  QVBoxLayout* label_container_layout_;
  QScrollBar* scrollbar_;
  QWidget* label_container_;
private slots:
  void setScrollMaximum(int);
};

#endif // TIMELINEAREA_H
