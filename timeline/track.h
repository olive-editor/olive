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

    extern int kTimelineLabelFixedWidth;
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

  Sequence* sequence();
  TrackList* track_list();

  void Save(QXmlStreamWriter& stream);

  Type type();

  int height();
  void set_height(int h);

  QString name();
  void SetName(const QString& s);

  void AddClip(ClipPtr clip);
  int ClipCount();
  ClipPtr GetClip(int i);
  void RemoveClip(int i);
  void RemoveClip(Clip* c);
  QVector<Clip*> GetAllClips();
  QVector<Clip*> GetSelectedClips(bool containing);
  ClipPtr GetClipObjectFromRawPtr(Clip* c);
  Clip* GetClipFromPoint(long point);

  int Index();

  bool IsClipSelected(int clip_index, bool containing = true);
  bool IsClipSelected(Clip* clip, bool containing = true);
  bool IsTransitionSelected(Transition* t);

  void DeleteArea(ComboAction *ca, const Selection& s);
  void DeleteArea(ComboAction *ca, long in, long out);

  void SelectArea(long in, long out);
  void SelectClip(Clip *c);
  void SelectAll();
  void SelectAtPoint(long point);
  void TidySelections();
  void ClearSelections();
  void DeselectArea(long in, long out);
  QVector<Selection> Selections();

  long GetEndFrame();

  bool IsEffectivelyMuted();
  bool IsMuted();
  bool IsSoloed();
  bool IsLocked();

public slots:
  void SetMuted(bool muted);
  void SetSoloed(bool soloed);
  void SetLocked(bool locked);

signals:
  void HeightChanged(int height);

private:
  void ResizeClipArray(int new_size);

  TrackList* parent_;
  Type type_;
  int height_;
  QVector<ClipPtr> clips_;
  QVector<EffectPtr> effects_;
  QVector<Selection> selections_;

  bool muted_;
  bool soloed_;
  bool locked_;

  QString name_;
};

#endif // TRACK_H
