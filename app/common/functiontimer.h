#ifndef FUNCTIONTIMER_H
#define FUNCTIONTIMER_H

#include <QDateTime>
#include <QDebug>

#define TIME_THIS_FUNCTION FunctionTimer __f(__FUNCTION__)

class FunctionTimer {
public:
  FunctionTimer(const char* s)
  {
    name_ = s;
    time_ = QDateTime::currentMSecsSinceEpoch();
  }

  ~FunctionTimer()
  {
    qint64 elapsed = (QDateTime::currentMSecsSinceEpoch() - time_);

    if (elapsed > 1) {
      qDebug() << name_ << "took" << elapsed;
    }
  }

private:
  const char* name_;

  qint64 time_;

};

#endif // FUNCTIONTIMER_H
