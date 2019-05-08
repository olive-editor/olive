#ifndef TIMELINEAREA_H
#define TIMELINEAREA_H

#include "timeline/sequence.h"
#include "timeline/track.h"
#include "timeline/timelinefunctions.h"
#include "ui/timelineview.h"
#include "ui/timelinelabel.h"

class TimelineArea : public QWidget
{
  Q_OBJECT
public:
  TimelineArea(Timeline *timeline, olive::timeline::Alignment alignment = olive::timeline::kAlignmentTop);

  void SetTrackType(Sequence* sequence, olive::TrackType track_type);
  void SetAlignment(olive::timeline::Alignment alignment);

  olive::TrackType track_type();
  TimelineView* view();

  virtual void wheelEvent(QWheelEvent *event) override;
public slots:
  void RefreshLabels();
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
private:
  Timeline* timeline_;
  Sequence* track_list_;
  olive::TrackType type_;
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
