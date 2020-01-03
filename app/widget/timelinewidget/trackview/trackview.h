#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QScrollArea>
#include <QSplitter>

#include "node/output/track/tracklist.h"
#include "trackviewitem.h"
#include "trackviewsplitter.h"

class TrackView : public QScrollArea
{
  Q_OBJECT
public:
  TrackView(Qt::Alignment vertical_alignment = Qt::AlignTop,
            QWidget* parent = nullptr);

  void ConnectTrackList(TrackList* list);
  void DisconnectTrackList();

protected:
  virtual void resizeEvent(QResizeEvent *e) override;

private:
  QList<TrackViewItem*> items_;

  TrackList* list_;

  TrackViewSplitter* splitter_;

  Qt::Alignment alignment_;

  int last_scrollbar_max_;

private slots:
  void ScrollbarRangeChanged(int min, int max);

  void TrackHeightChanged(int index, int height);

  void InsertTrack(TrackOutput* track);

  void RemoveTrack(TrackOutput* track);

};

#endif // TRACKVIEW_H
