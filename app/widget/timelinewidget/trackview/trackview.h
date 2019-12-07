#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QScrollArea>
#include <QSplitter>

#include "trackviewsplitter.h"

class TrackView : public QScrollArea
{
  Q_OBJECT
public:
  TrackView(Qt::Alignment vertical_alignment = Qt::AlignTop,
            QWidget* parent = nullptr);

protected:
  virtual void resizeEvent(QResizeEvent* event) override;

private:
  TrackViewSplitter* splitter_;

  Qt::Alignment alignment_;

  int last_scrollbar_max_;

  QWidget* top_spacer_;

private slots:
  void ScrollbarRangeChanged(int min, int max);

};

#endif // TRACKVIEW_H
