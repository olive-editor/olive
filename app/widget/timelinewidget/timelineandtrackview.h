#ifndef TIMELINEANDTRACKVIEW_H
#define TIMELINEANDTRACKVIEW_H

#include <QSplitter>
#include <QWidget>

#include "view/timelineview.h"
#include "trackview/trackview.h"

class TimelineAndTrackView : public QWidget
{
public:
  TimelineAndTrackView(Qt::Alignment vertical_alignment = Qt::AlignTop,
                       QWidget* parent = nullptr);

  QSplitter* splitter() const;

  TimelineView* view() const;

  TrackView* track_view() const;

private:
  QSplitter* splitter_;

  TimelineView* view_;

  TrackView* track_view_;

private slots:
  void ViewValueChanged(int v);

  void TracksValueChanged(int v);

};

#endif // TIMELINEANDTRACKVIEW_H
