#ifndef TIMELINELABEL_H
#define TIMELINELABEL_H

#include <QWidget>

#include "timeline/track.h"

class TimelineLabel : public QWidget
{
  Q_OBJECT
public:
  TimelineLabel();

  void SetTrack(Track* track);
private:
  QPushButton* mute_button_;
  QPushButton* solo_button_;
  QPushButton* lock_button_;

  Track* track_;
};

#endif // TIMELINELABEL_H
