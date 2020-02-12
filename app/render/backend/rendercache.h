#ifndef RENDERCACHE_H
#define RENDERCACHE_H

#include <QMap>
#include <QMutex>

template<class K, class V>
class RenderCache
{
public:
  RenderCache() = default;

  void Clear(){values_.clear();}

  void Add(K key, V val){values_.insert(key, val);}

  V Get(K key) const {return values_.value(key);}

  bool Has(K key) const {return values_.contains(key);}

private:
  QHash<K, V> values_;
};

template<class K, class V>
class ThreadSafeRenderCache
{
public:
  ThreadSafeRenderCache() = default;

  void Clear() {
    lock_.lock();
    values_.clear();
    lock_.unlock();
  }

  void Add(K key, V val) {
    lock_.lock();
    values_.insert(key, val);
    lock_.unlock();
  }

  V Get(K key) {
    lock_.lock();
    V val = values_.value(key);
    lock_.unlock();
    return val;
  }

  bool Has(K key) {
    lock_.lock();
    bool has = values_.contains(key);
    lock_.unlock();
    return has;
  }

private:
  QHash<K, V> values_;

  QMutex lock_;

};

#endif // RENDERCACHE_H
