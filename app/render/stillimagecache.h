#ifndef STILLIMAGECACHE_H
#define STILLIMAGECACHE_H

#include <QHash>

#include "common/rational.h"
#include "project/item/footage/stream.h"
#include "render/texture.h"

OLIVE_NAMESPACE_ENTER

class StillImageCache
{
public:
  struct Entry {
    TexturePtr texture;
    StreamPtr stream;
    QString colorspace;
    bool alpha_is_associated;
    int divider;
    rational time;
  };

  QMutex* mutex()
  {
    return &mutex_;
  }

  const QVector<Entry>& entries() const
  {
    return entries_;
  }

  const QVector<Entry>& pending() const
  {
    return pending_;
  }

  static bool CompareEntryMetadata(const Entry& a, const Entry& b)
  {
    return (a.stream == b.stream
            && a.colorspace == b.colorspace
            && a.alpha_is_associated == b.alpha_is_associated
            && a.divider == b.divider
            && a.time == b.time);
  }

  void PushPending(const Entry& e)
  {
    pending_.prepend(e);
  }

  void PushEntry(const Entry& e)
  {
    entries_.prepend(e);

    if (entries_.size() > 8) {
      entries_.removeLast();
    }
  }

  void RemovePending(const Entry& e)
  {
    for (int i=0; i<pending_.size(); i++) {
      if (CompareEntryMetadata(pending_.at(i), e)) {
        pending_.removeAt(i);
        break;
      }
    }
  }

private:
  QMutex mutex_;

  QVector<Entry> entries_;

  QVector<Entry> pending_;

};

OLIVE_NAMESPACE_EXIT

#endif // STILLIMAGECACHE_H
