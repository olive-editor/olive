#ifndef RENDERCACHE_H
#define RENDERCACHE_H

#include <QMap>

template<class K, class V>
class RenderCache
{
public:
  RenderCache() = default;

  void Clear(){values_.clear();}

  void Add(K stream, V shader){values_.insert(stream, shader);}

  V Get(K stream){return values_.value(stream);}

private:
  QMap<K, V> values_;
};

#endif // RENDERCACHE_H
