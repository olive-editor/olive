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
#include "ghost.h"
#include "nodes/nodegraph.h"

class Sequence : public NodeGraph {
  Q_OBJECT
public:
  Sequence();
  SequencePtr copy();

  void Save(QXmlStreamWriter& stream);

  const QString& name();
  void set_name(const QString& s);

  const double& frame_rate();
  void set_frame_rate(const double& d);

  const int& audio_frequency();
  void set_audio_frequency(const int& f);

  const int& audio_layout();
  void set_audio_layout(const int& l);

  long GetEndFrame();
  QVector<Clip*> GetAllClips();

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

  OldEffectNode* GetSelectedGizmo();

  bool IsClipSelected(Clip* clip, bool containing = true);
  bool IsTransitionSelected(Transition* t);

  void SelectAll();
  void SelectAtPlayhead();
  void ClearSelections();
  void AddSelectionsToClipboard(bool delete_originals);
  QVector<Selection> Selections();
  void SetSelections(const QVector<Selection>& selections);
  void TidySelections();

  Track* PreviousTrack(Track* t);
  Track* NextTrack(Track* t);
  Track* SiblingTrack(Track* t, int diff);
  int IndexOfTrack(Track* t);
  Track* FirstTrack(olive::TrackType type);
  Track* LastTrack(olive::TrackType type);
  Track* TrackAt(olive::TrackType type, int index);
  int TrackCount(olive::TrackType type);
  QVector<Track*> GetTrackList(olive::TrackType type);

  // FIXME: TEST CODE
  GLuint texture();
  NodeIO* texture_io;
  // END TEST CODE

  rational playhead;

  bool using_workarea;
  long workarea_in;
  long workarea_out;

  bool wrapper_sequence;

  int save_id;

  QVector<Marker> markers;

private:
  Track *AddTrack(olive::TrackType type);

  //QVector<Track*> tracks_;

  ClipPtr SplitClip(ComboAction* ca, bool transitions, Clip *clip, long frame);
  ClipPtr SplitClip(ComboAction* ca, bool transitions, Clip *clip, long frame, long post_in);
  bool SplitSelection(ComboAction* ca, QVector<Selection> selections);

  QString name_;
  double frame_rate_;
  int audio_frequency_;
  int audio_layout_;
};

using SequencePtr = std::shared_ptr<Sequence>;

#endif // SEQUENCE_H
