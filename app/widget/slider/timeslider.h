#ifndef TIMESLIDER_H
#define TIMESLIDER_H

#include "common/rational.h"
#include "integerslider.h"

class TimeSlider : public IntegerSlider
{
  Q_OBJECT
public:
  TimeSlider(QWidget* parent = nullptr);

  void SetTimebase(const rational& timebase);

protected:
  virtual QString ValueToString(const QVariant& v) override;

  virtual QVariant StringToValue(const QString& s, bool* ok) override;

private:
  rational timebase_;

private slots:
  void TimecodeDisplayChanged();

};

#endif // TIMESLIDER_H
