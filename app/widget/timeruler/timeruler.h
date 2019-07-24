#ifndef TIMERULER_H
#define TIMERULER_H

#include <QWidget>

#include "common/rational.h"

class TimeRuler : public QWidget
{
  Q_OBJECT
public:
  TimeRuler(bool text_visible = true, QWidget* parent = nullptr);

  void SetTextVisible(bool e);

  void SetScale(double d);

  void SetTimebase(const rational& r);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private:
  int text_height_;

  int width_of_second_;

  int minimum_gap_between_lines_;

  int scroll_;

  bool text_visible_;

  double scale_;

  rational time_base_;
};

#endif // TIMERULER_H
