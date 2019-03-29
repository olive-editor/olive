#ifndef TRACK_H
#define TRACK_H

#include <QVector>
#include <memory>

#include "effects/effect.h"

namespace olive {
  namespace timeline {
    const int kGhostThickness = 2;
    const int kClipTextPadding = 3;

    /**
     * @brief Set default track sizes
     *
     * Olive has a few default constants used for adjusting track heights in the Timeline. For HiDPI, it makes
     * sense to multiply these by the current DPI scale. It uses a variable from QApplication to do this multiplication,
     * which means the QApplication instance needs to be instantiated before these are calculated. Therefore, call this
     * function ONCE after QApplication is created to multiply the track heights correctly.
     */
    void MultiplyTrackSizesByDPI();

    extern int kTrackDefaultHeight;
    extern int kTrackMinHeight;
    extern int kTrackHeightIncrement;
  }
}

class TrackList;

class Track : public QObject
{
  Q_OBJECT
public:
  enum Type {
    kTypeVideo,
    kTypeAudio,
    kTypeSubtitle,
    kTypeCount
  };

  Track(TrackList* parent, Type type);
  Track* copy(TrackList* parent);

  void Save(QXmlStreamWriter& stream);

  Type type();

  int height();
  void set_height(int h);

  void AddClip(ClipPtr clip);
  int ClipCount();
  ClipPtr GetClip(int i);
  void RemoveClip(int i);
  void RemoveClip(Clip* c);
  QVector<ClipPtr> GetAllClips();
  QVector<Clip*> GetSelectedClips();

  bool IsClipSelected(int clip_index, bool containing = true);
  bool IsClipSelected(Clip* clip, bool containing = true);
  bool IsTransitionSelected(Transition* t);

  void ClearSelections();

  long GetEndFrame();
private:
  void ResizeClipArray(int new_size);

  TrackList* parent_;
  Type type_;
  int height_;
  QVector<ClipPtr> clips_;
  QVector<EffectPtr> effects_;
  QVector<Selection> selections_;
};

#endif // TRACK_H
