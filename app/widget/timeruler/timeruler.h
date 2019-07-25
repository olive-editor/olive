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

public slots:
  void SetScroll(int s);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

private:
  void DrawPlayhead(QPainter* p, int x, int y);

  double UnitToScreen(const int& u);

  int ScreenToUnit(const int& p);

  int text_height_;

  int minimum_gap_between_lines_;

  int scroll_;

  bool text_visible_;

  bool centered_text_;

  double scale_;

  rational time_base_;

// FIXME: Test code only
  QTimer test_timer_;

private slots:
  void TimeOut();
// End test code
};

#endif // TIMERULER_H
