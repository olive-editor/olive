#ifndef CANCELABLEOBJECT_H
#define CANCELABLEOBJECT_H

#include <QAtomicInt>

class CancelableObject {
public:
  CancelableObject() :
    cancelled_(false)
  {
  }

  void Cancel() {
    cancelled_ = true;
  }

  const QAtomicInt& IsCancelled() const {
    return cancelled_;
  }

private:
  QAtomicInt cancelled_;

};

#endif // CANCELABLEOBJECT_H
