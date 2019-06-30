#ifndef _OLIVE_SEQUENCE_SCROLL_VIEW_H_
#define _OLIVE_SEQUENCE_SCROLL_VIEW_H_

#include <QWidget>

namespace olive {

class Sequence;

class SequenceScrollView : public QWidget {
  Q_OBJECT

private:
  Sequence* sequence_;
  SequenceScrollView* scrollView_;

  int startTime_;
  int endTime_;
  float pxPerFrame_;
  int width_;
  float unitFrameTime_;
  float unitWidth_;

  float scrollStart_;
  float scrollEnd_;

  void update();

public:
  SequenceScrollView(
    QWidget* const parent,
    Sequence* const sequence);

  int getTimeRelativeToView(int pixel) const;
  int getTimeAmountRelativeToView(int pixel) const;
  int getPositionRelativeToView(int time) const;
  int getPixelAmountRelativeToView(int time) const;

  int startTime() const;
  int endTime() const;
  int width() const;
  float pxPerFrame() const;
  float unitFrameTime() const;
  float unitWidth() const;

signals:
  void onDidUpdate();

};

}

#endif