#ifndef TIMELINELABEL_H
#define TIMELINELABEL_H

#include <QWidget>
#include <memory>

#include "ui/clickablelabel.h"
#include "timeline/track.h"

class TimelineLabel : public QWidget
{
  Q_OBJECT
public:
  TimelineLabel();

  void SetTrack(Track* track);
  void UpdateState();
protected:
  virtual void paintEvent(QPaintEvent *event) override;
private:
  QPushButton* mute_button_;
  QPushButton* solo_button_;
  QPushButton* lock_button_;

  ClickableLabel* label_;

  Track* track_;
private slots:
  void RenameTrack();
  void UpdateHeight(int h);
};

using TimelineLabelPtr = std::shared_ptr<TimelineLabel>;

#endif // TIMELINELABEL_H
