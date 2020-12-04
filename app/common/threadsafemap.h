#ifndef THREADSAFEMAP_H
#define THREADSAFEMAP_H

#include <QMap>
#include <QMutex>

template <typename K, typename V>
class ThreadSafeMap
{
public:
  ThreadSafeMap() = default;

  void insert(K key, V value)
  {
    mutex_.lock();
    map_.insert(key, value);
    mutex_.unlock();
  }

private:
  QMutex mutex_;

  QMap<K, V> map_;

};

#endif // THREADSAFEMAP_H
