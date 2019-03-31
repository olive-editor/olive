/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <memory>
#include <QVector>

#include "clip.h"
#include "marker.h"
#include "selection.h"
#include "tracklist.h"

class Sequence : public QObject {
  Q_OBJECT
public:
  Sequence();
  SequencePtr copy();

  void Save(QXmlStreamWriter& stream);

  QString name;
  int width;
  int height;
  double frame_rate;
  int audio_frequency;
  int audio_layout;

  long GetEndFrame();
  QVector<Clip*> GetAllClips();
  TrackList* GetTrackList(Track::Type type);

  /**
   * @brief Close all open clips in a Sequence
   *
   * Closes any currently open clips on a Sequence and waits for them to close before returning. This may be slow as a
   * result on large Sequence objects. If a Clip is a nested Sequence, this function calls itself recursively on that
   * Sequence too.
   *
   * @param s
   *
   * The Sequence to close all clips on.
   */
  void Close();

  void RefreshClipsUsingMedia(Media* m = nullptr);
  QVector<Clip*> SelectedClips(bool containing = true);
  //QVector<int> SelectedClipIndexes();

  void DeleteAreas(ComboAction* ca, QVector<Selection> areas, bool deselect_areas);
  void DeleteInToOut(bool ripple);

  void Ripple(ComboAction *ca, long point, long length, const QVector<int>& ignore = QVector<int>());

  void ChangeTrackHeightsRelatively(int diff);

  bool SplitAllClipsAtPoint(ComboAction *ca, long point);
  void SplitClipAtPositions(ComboAction* ca, Clip *clip, QVector<long> positions, bool relink = true);

  Effect* GetSelectedGizmo();

  bool IsClipSelected(Clip* clip, bool containing = true);
  bool IsTransitionSelected(Transition* t);

  void SelectAll();
  void SelectAtPlayhead();
  void ClearSelections();
  void AddSelectionsToClipboard(bool delete_originals);
  QVector<Selection> Selections();

  long playhead;

  bool using_workarea;
  long workarea_in;
  long workarea_out;

  bool wrapper_sequence;

  int save_id;

  QVector<Marker> markers;
private:
  QVector<TrackList*> track_lists_;

  ClipPtr SplitClip(ComboAction* ca, bool transitions, Clip *clip, long frame);
  ClipPtr SplitClip(ComboAction* ca, bool transitions, Clip *clip, long frame, long post_in);
};

using SequencePtr = std::shared_ptr<Sequence>;

// static variable for the currently active sequence
namespace olive {
  extern SequencePtr ActiveSequence;
}

#endif // SEQUENCE_H
