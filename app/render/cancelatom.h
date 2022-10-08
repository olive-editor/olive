#ifndef CANCELATOM_H
#define CANCELATOM_H

#include <QMutex>

namespace olive {

class CancelAtom
{
public:
  CancelAtom() :
    cancelled_(false),
    heard_(false)
  {}

  bool IsCancelled()
  {
    QMutexLocker locker(&mutex_);
    if (cancelled_) {
      heard_ = true;
    }
    return cancelled_;
  }

  void Cancel()
  {
    QMutexLocker locker(&mutex_);
    cancelled_ = true;
  }

  bool HeardCancel()
  {
    QMutexLocker locker(&mutex_);
    return heard_;
  }

private:
  QMutex mutex_;

  bool cancelled_;

  bool heard_;

};

}

#endif // CANCELATOM_H
