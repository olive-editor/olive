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

#endif // RENDERCACHE_H
