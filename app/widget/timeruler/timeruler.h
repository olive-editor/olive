#ifndef TIMERULER_H
#define TIMERULER_H

#include <QTimer>
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

  void SetCenteredText(bool c);

  void SetTime(const int64_t &r);

public slots:
  void SetScroll(int s);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;

signals:
  void TimeChanged(int64_t);

private:
  void DrawPlayhead(QPainter* p, int x, int y);

  double ScreenToUnitFloat(int screen);

  int64_t ScreenToUnit(int screen);

  void SeekToScreenPoint(int screen);

  int text_height_;

  int minimum_gap_between_lines_;

  int scroll_;

  bool text_visible_;

  bool centered_text_;

  double scale_;

  rational time_base_;

  int64_t time_;

};

#endif // TIMERULER_H
