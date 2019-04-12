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
#include "ghost.h"

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
  TrackList* GetTrackList(olive::TrackType type);

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

  void AddClipsFromGhosts(ComboAction *ca, const QVector<Ghost> &ghosts);

  void MoveClip(Clip* c,
                ComboAction* ca,
                long iin,
                long iout,
                long iclip_in,
                Track *itrack,
                bool verify_transitions = true,
                bool relative = false);

  void EditToPoint(bool in, bool ripple);

  bool SnapPoint(long* l, double zoom, bool use_playhead, bool use_markers, bool use_workarea);

  void DeleteAreas(ComboAction* ca, QVector<Selection> areas, bool deselect_areas = false, bool ripple = false);
  void DeleteInToOut(bool ripple);
  void DeleteClipsUsingMedia(const QVector<Media *> &media);

  void Ripple(ComboAction *ca, long point, long length, const QVector<Clip *> &ignore = QVector<Clip*>());

  void ChangeTrackHeightsRelatively(int diff);

  void ToggleLinksOnSelected();

  void Split();
  bool SplitAllClipsAtPoint(ComboAction *ca, long point);
  bool SplitClipAtPositions(ComboAction* ca, Clip *clip, QVector<long> positions, bool also_split_links = true);

  void RippleDeleteEmptySpace(ComboAction *ca, Track *track, long point);
  void RippleDeleteArea(ComboAction* ca, long ripple_point, long ripple_length);

  Node* GetSelectedGizmo();

  bool IsClipSelected(Clip* clip, bool containing = true);
  bool IsTransitionSelected(Transition* t);

  void SelectAll();
  void SelectAtPlayhead();
  void ClearSelections();
  void AddSelectionsToClipboard(bool delete_originals);
  QVector<Selection> Selections();
  void SetSelections(const QVector<Selection>& selections);
  void TidySelections();

  long playhead;

  bool using_workarea;
  long workarea_in;
  long workarea_out;

  bool wrapper_sequence;

  int save_id;

  QVector<Marker> markers;
signals:
  void Changed();
private:
  QVector<TrackList*> track_lists_;

  ClipPtr SplitClip(ComboAction* ca, bool transitions, Clip *clip, long frame);
  ClipPtr SplitClip(ComboAction* ca, bool transitions, Clip *clip, long frame, long post_in);
  bool SplitSelection(ComboAction* ca, QVector<Selection> selections);
};

using SequencePtr = std::shared_ptr<Sequence>;

#endif // SEQUENCE_H
