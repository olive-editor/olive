#ifndef STILLIMAGECACHE_H
#define STILLIMAGECACHE_H

#include <QHash>
#include <QWaitCondition>

#include "common/rational.h"
#include "project/item/footage/stream.h"
#include "render/texture.h"

namespace olive {

class StillImageCache
{
public:
  struct Entry {
    Entry(TexturePtr t, StreamPtr s, const QString& cs, bool a, int d, const rational& i, bool w)
    {
      texture = t;
      stream = s;
      colorspace = cs;
      alpha_is_associated = a;
      divider = d;
      time = i;
      working = w;
    }

    TexturePtr texture;
    StreamPtr stream;
    QString colorspace;
    bool alpha_is_associated;
    int divider;
    rational time;
    bool working;
  };

  using EntryPtr = std::shared_ptr<Entry>;

  QMutex* mutex()
  {
    return &mutex_;
  }

  QWaitCondition* wait_cond()
  {
    return &wait_cond_;
  }

  const QVector<EntryPtr>& entries() const
  {
    return entries_;
  }

  static bool CompareEntryMetadata(EntryPtr a, EntryPtr b)
  {
    return (a->stream == b->stream
            && a->colorspace == b->colorspace
            && a->alpha_is_associated == b->alpha_is_associated
            && a->divider == b->divider
            && a->time == b->time);
  }

  void PushEntry(EntryPtr e)
  {
    entries_.prepend(e);

    if (entries_.size() > 8) {
      entries_.removeLast();
    }
  }

private:
  QMutex mutex_;

  QWaitCondition wait_cond_;

  QVector<EntryPtr> entries_;

};

}

#endif // STILLIMAGECACHE_H
