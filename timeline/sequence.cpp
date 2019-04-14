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

#include "sequence.h"

#include <QCoreApplication>

#include "timelinefunctions.h"
#include "panels/panels.h"
#include "global/clipboard.h"
#include "global/config.h"
#include "global/debug.h"

Sequence::Sequence() :
  playhead(0),
  using_workarea(false),
  workarea_in(0),
  workarea_out(0),
  wrapper_sequence(false)
{
  // Set up tracks
  track_lists_.resize(olive::kTypeCount);

  for (int i=0;i<track_lists_.size();i++) {
    track_lists_[i] = new TrackList(this, static_cast<olive::TrackType>(i));
  }
}

SequencePtr Sequence::copy() {
  SequencePtr s = std::make_shared<Sequence>();
  s->name = QCoreApplication::translate("Sequence", "%1 (copy)").arg(name);
  s->width = width;
  s->height = height;
  s->frame_rate = frame_rate;
  s->audio_frequency = audio_frequency;
  s->audio_layout = audio_layout;

  // deep copy all of the sequence's clips
  for (int i=0;i<track_lists_.size();i++) {
    s->track_lists_[i] = track_lists_.at(i)->copy(s.get());
  }

  // copy all of the sequence's markers
  s->markers = markers;

  return s;
}

void Sequence::Save(QXmlStreamWriter &stream)
{
  // Provide unique IDs for each Clip
  QVector<Clip*> all_clips = GetAllClips();
  for (int i=0;i<all_clips.size();i++) {
    all_clips.at(i)->load_id = i;
  }

  stream.writeStartElement("sequence");
  stream.writeAttribute("id", QString::number(save_id));
  stream.writeAttribute("name", name);
  stream.writeAttribute("width", QString::number(width));
  stream.writeAttribute("height", QString::number(height));
  stream.writeAttribute("framerate", QString::number(frame_rate, 'f', 10));
  stream.writeAttribute("afreq", QString::number(audio_frequency));
  stream.writeAttribute("alayout", QString::number(audio_layout));
  if (this == Timeline::GetTopSequence().get()) {
    stream.writeAttribute("open", "1");
  }
  stream.writeAttribute("workarea", QString::number(using_workarea));
  stream.writeAttribute("workareaIn", QString::number(workarea_in));
  stream.writeAttribute("workareaOut", QString::number(workarea_out));

  QVector<TransitionPtr> transition_save_cache;
  QVector<int> transition_clip_save_cache;

  for (int j=0;j<track_lists_.size();j++) {
    track_lists_.at(j)->Save(stream);
  }

  for (int j=0;j<markers.size();j++) {
    markers.at(j).Save(stream);
  }

  stream.writeEndElement();
}

int Sequence::SequenceWidth()
{
  return width;
}

int Sequence::SequenceHeight()
{
  return height;
}

double Sequence::SequenceFrameRate()
{
  return frame_rate;
}

long Sequence::SequencePlayhead()
{
  return playhead;
}

long Sequence::GetEndFrame() {
  long end_frame = 0;

  for (int j=0;j<track_lists_.size();j++) {

    TrackList* track_list = track_lists_.at(j);

    for (int i=0;i<track_list->TrackCount();i++) {
      end_frame = qMax(track_list->TrackAt(i)->GetEndFrame(), end_frame);
    }

  }

  return end_frame;
}

QVector<Clip *> Sequence::GetAllClips()
{
  QVector<Clip*> all_clips;

  for (int j=0;j<track_lists_.size();j++) {

    TrackList* track_list = track_lists_.at(j);

    for (int i=0;i<track_list->TrackCount();i++) {
      all_clips.append(track_list->TrackAt(i)->GetAllClips());
    }

  }

  return all_clips;
}

TrackList *Sequence::GetTrackList(olive::TrackType type)
{
  return track_lists_.at(type);
}

void Sequence::Close()
{
  QVector<Clip*> all_clips = GetAllClips();

  for (int i=0;i<all_clips.size();i++) {
    all_clips.at(i)->Close(true);
  }
}

void Sequence::RefreshClipsUsingMedia(Media *m) {

  QVector<Clip*> all_clips = GetAllClips();

  for (int i=0;i<all_clips.size();i++) {
    Clip* c = all_clips.at(i);

    if (m == nullptr || c->media() == m) {
      c->Close(true);
      c->refresh();
    }
  }

}

QVector<Clip *> Sequence::SelectedClips(bool containing)
{
  QVector<Clip*> selected_clips;

  for (int i=0;i<track_lists_.size();i++) {
    TrackList* tl = track_lists_.at(i);

    for (int j=0;j<tl->TrackCount();j++) {
      Track* t = tl->TrackAt(j);

      selected_clips.append(t->GetSelectedClips(containing));
    }
  }

  return selected_clips;
}

void Sequence::AddClipsFromGhosts(ComboAction* ca, const QVector<Ghost>& ghosts)
{
  // add clips
  long earliest_point = LONG_MAX;
  QVector<ClipPtr> added_clips;
  for (int i=0;i<ghosts.size();i++) {
    const Ghost& g = ghosts.at(i);

    earliest_point = qMin(earliest_point, g.in);

    ClipPtr c = std::make_shared<Clip>(g.track->Sibling(g.track_movement));
    c->set_media(g.media, g.media_stream);
    c->set_timeline_in(g.in);
    c->set_timeline_out(g.out);
    c->set_clip_in(g.clip_in);
    if (c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
      Footage* m = c->media()->to_footage();
      if (m->video_tracks.size() == 0) {
        // audio only (greenish)
        c->set_color(128, 192, 128);
      } else if (m->audio_tracks.size() == 0) {
        // video only (orangeish)
        c->set_color(192, 160, 128);
      } else {
        // video and audio (blueish)
        c->set_color(128, 128, 192);
      }
      c->set_name(m->name);
    } else if (c->media()->get_type() == MEDIA_TYPE_SEQUENCE) {
      // sequence (red?ish?)
      c->set_color(192, 128, 128);

      c->set_name(c->media()->to_sequence()->name);
    }
    c->refresh();
    added_clips.append(c);

  }
  ca->append(new AddClipCommand(added_clips));

  // link clips from the same media
  for (int i=0;i<added_clips.size();i++) {
    ClipPtr c = added_clips.at(i);
    for (int j=0;j<added_clips.size();j++) {
      ClipPtr cc = added_clips.at(j);
      if (c != cc && c->media() == cc->media()) {
        c->linked.append(cc.get());
      }
    }

    if (olive::config.add_default_effects_to_clips) {
      if (c->type() == olive::kTypeVideo) {
        // add default video effects
        c->effects.append(olive::node_library[kTransformEffect]->Create(c.get()));
      } else if (c->type() == olive::kTypeAudio) {
        // add default audio effects
        c->effects.append(olive::node_library[kVolumeEffect]->Create(c.get()));
        c->effects.append(olive::node_library[kPanEffect]->Create(c.get()));
      }
    }
  }

  if (olive::config.enable_seek_to_import) {
    panel_sequence_viewer->seek(earliest_point);
  }

  olive::timeline::snapped = false;
}

void Sequence::MoveClip(Clip *c, ComboAction *ca, long iin, long iout, long iclip_in, Track *itrack, bool verify_transitions, bool relative)
{
  ClipPtr clip_ptr = c->track()->GetClipObjectFromRawPtr(c);

  ca->append(new MoveClipAction(clip_ptr, iin, iout, iclip_in, itrack, relative));

  if (verify_transitions) {

    // if this is a shared transition, and the corresponding clip will be moved away somehow
    if (c->opening_transition != nullptr
        && c->opening_transition->secondary_clip != nullptr
        && c->opening_transition->secondary_clip->timeline_out() != iin) {
      // separate transition
      ca->append(new SetPointer(reinterpret_cast<void**>(&c->opening_transition->secondary_clip), nullptr));
      ca->append(new AddTransitionCommand(nullptr,
                                          c->opening_transition->secondary_clip,
                                          c->opening_transition,
                                          kInvalidNode,
                                          0));
    }

    if (c->closing_transition != nullptr
        && c->closing_transition->secondary_clip != nullptr
        && c->closing_transition->GetParentClip()->timeline_in() != iout) {
      // separate transition
      ca->append(new SetPointer(reinterpret_cast<void**>(&c->closing_transition->secondary_clip), nullptr));
      ca->append(new AddTransitionCommand(nullptr,
                                          c,
                                          c->closing_transition,
                                          kInvalidNode,
                                          0));
    }
  }
}

void Sequence::EditToPoint(bool in, bool ripple)
{
  QVector<Clip*> all_clips = GetAllClips();

  if (all_clips.size() > 0) {
    long sequence_end = 0;

    bool playhead_falls_on_in = false;
    bool playhead_falls_on_out = false;
    long next_cut = LONG_MAX;
    long prev_cut = 0;

    // find closest in point to playhead
    for (int i=0;i<all_clips.size();i++) {
      Clip* c = all_clips.at(i);

      sequence_end = qMax(c->timeline_out(), sequence_end);

      if (c->timeline_in() == playhead)
        playhead_falls_on_in = true;

      if (c->timeline_out() == playhead)
        playhead_falls_on_out = true;

      if (c->timeline_in() > playhead)
        next_cut = qMin(c->timeline_in(), next_cut);

      if (c->timeline_out() > playhead)
        next_cut = qMin(c->timeline_out(), next_cut);

      if (c->timeline_in() < playhead)
        prev_cut = qMax(c->timeline_in(), prev_cut);

      if (c->timeline_out() < playhead)
        prev_cut = qMax(c->timeline_out(), prev_cut);

    }

    next_cut = qMin(sequence_end, next_cut);

    QVector<Selection> areas;
    ComboAction* ca = new ComboAction();
    bool push_undo = true;
    long seek = playhead;

    if ((in && (playhead_falls_on_out || (playhead_falls_on_in && playhead == 0)))
        || (!in && (playhead_falls_on_in || (playhead_falls_on_out && playhead == sequence_end)))) { // one frame mode
      if (ripple) {
        // set up deletion areas based on track count
        long in_point = playhead;
        if (!in) {
          in_point--;
          seek--;
        }

        if (in_point >= 0) {

          for (int i=0;i<track_lists_.size();i++) {

            TrackList* tl = track_lists_.at(i);

            for (int j=0;j<tl->TrackCount();j++) {
              areas.append(Selection(in_point, in_point+1, tl->TrackAt(j)));
            }

          }

          // trim and move clips around the in point
          DeleteAreas(ca, areas, true);

          if (ripple) {
            Ripple(ca, in_point, -1);
          }
        } else {
          push_undo = false;
        }
      } else {
        push_undo = false;
      }
    } else {
      // set up deletion areas based on track count

      long area_in, area_out;

      if (in) {
        seek = prev_cut;
        area_in = prev_cut;
        area_out = playhead;
      } else {
        area_in = playhead;
        area_out = next_cut;
      }

      if (area_in == area_out) {

        push_undo = false;

      } else {

        for (int i=0;i<track_lists_.size();i++) {

          TrackList* tl = track_lists_.at(i);

          for (int j=0;j<tl->TrackCount();j++) {
            areas.append(Selection(area_in, area_out, tl->TrackAt(j)));
          }

        }

        // trim and move clips around the in point
        DeleteAreas(ca, areas, true);
        if (ripple) {
          Ripple(ca, area_in, area_in - area_out);
        }
      }
    }

    if (push_undo) {
      olive::undo_stack.push(ca);

      update_ui(true);

      if (seek != playhead && ripple) {
        panel_sequence_viewer->seek(seek);
      }
    } else {
      delete ca;
    }
  } else {
    panel_sequence_viewer->seek(0);
  }
}

bool Sequence::SnapPoint(long *l, double zoom, bool use_playhead, bool use_markers, bool use_workarea)
{
  olive::timeline::snapped = false;
  if (olive::timeline::snapping) {
    if (use_playhead && !panel_sequence_viewer->playing) {
      // snap to playhead
      if (olive::timeline::SnapToPoint(playhead, l, zoom)) return true;
    }

    // snap to marker
    if (use_markers) {
      for (int i=0;i<markers.size();i++) {
        if (olive::timeline::SnapToPoint(markers.at(i).frame, l, zoom)) return true;
      }
    }

    // snap to in/out
    if (use_workarea && using_workarea) {
      if (olive::timeline::SnapToPoint(workarea_in, l, zoom)) return true;
      if (olive::timeline::SnapToPoint(workarea_out, l, zoom)) return true;
    }

    // snap to clip/transition
    QVector<Clip*> all_clips = GetAllClips();
    for (int i=0;i<all_clips.size();i++) {

      Clip* c = all_clips.at(i);
      if (olive::timeline::SnapToPoint(c->timeline_in(), l, zoom)) {
        return true;
      } else if (olive::timeline::SnapToPoint(c->timeline_out(), l, zoom)) {
        return true;
      } else if (c->opening_transition != nullptr
                 && olive::timeline::SnapToPoint(c->timeline_in() + c->opening_transition->get_true_length(), l, zoom)) {
        return true;
      } else if (c->closing_transition != nullptr
                 && olive::timeline::SnapToPoint(c->timeline_out() - c->closing_transition->get_true_length(), l, zoom)) {
        return true;
      } else {
        // try to snap to clip markers
        for (int j=0;j<c->get_markers().size();j++) {
          if (olive::timeline::SnapToPoint(c->get_markers().at(j).frame + c->timeline_in() - c->clip_in(), l, zoom)) {
            return true;
          }
        }
      }

    }
  }

  return false;
}

void Sequence::DeleteInToOut(bool ripple)
{
  if (using_workarea) {

    QVector<Selection> areas_to_delete;

    for (int i=0;i<track_lists_.size();i++) {

      TrackList* tl = track_lists_.at(i);

      for (int j=0;j<tl->TrackCount();j++) {
        areas_to_delete.append(Selection(workarea_in, workarea_out, tl->TrackAt(j)));
      }

    }

    ComboAction* ca = new ComboAction();
    DeleteAreas(ca, areas_to_delete, true);
    if (ripple) Ripple(ca,
                       workarea_in,
                       workarea_in - workarea_out);
    ca->append(new SetTimelineInOutCommand(this, false, 0, 0));
    olive::undo_stack.push(ca);
    update_ui(true);
  }
}

void Sequence::DeleteClipsUsingMedia(const QVector<Media *>& media)
{
  QVector<Clip*> all_clips = GetAllClips();

  ComboAction* ca = new ComboAction();
  bool deleted = false;

  for (int j=0;j<media.size();j++) {
    Media* m = media.at(j);

    if (olive::clipboard.DeleteClipsWithMedia(ca, m)) {
      deleted = true;
    }

    for (int i=0;i<all_clips.size();i++) {
      Clip* c = all_clips.at(i);

      if (c->media() == media.at(j)) {
        ca->append(new DeleteClipAction(c));
        deleted = true;
      }
    }
  }

  if (deleted) {
    olive::undo_stack.push(ca);
    update_ui(true);
  } else {
    delete ca;
  }
}

void Sequence::Ripple(ComboAction *ca, long point, long length, const QVector<Clip*> &ignore)
{
  ca->append(new RippleAction(this, point, length, ignore));
}

void Sequence::ChangeTrackHeightsRelatively(int diff)
{
  for (int i=0;i<track_lists_.size();i++) {
    TrackList* tl = track_lists_.at(i);

    for (int j=0;j<tl->TrackCount();j++) {
      Track* t = tl->TrackAt(j);

      t->set_height(t->height() + diff);
    }
  }
}

void Sequence::ToggleLinksOnSelected()
{
  QVector<Clip*> selected_clips = SelectedClips();

  bool link = true;
  QVector<Clip*> link_clips;

  for (int i=0;i<selected_clips.size();i++) {
    Clip* c = selected_clips.at(i);

    if (!link_clips.contains(c)) {
      link_clips.append(c);
    }

    if (c->linked.size() > 0) {
      link = false; // prioritize unlinking

      for (int j=0;j<c->linked.size();j++) { // add links to the command
        if (!link_clips.contains(c->linked.at(j))) {
          link_clips.append(c->linked.at(j));
        }
      }
    }
  }

  if (!link_clips.isEmpty()) {
    olive::undo_stack.push(new LinkCommand(link_clips, link));
  }
}

void Sequence::Split()
{
  ComboAction* ca = new ComboAction();
  bool split_occurred = false;

  // See if there are any selected clips at the current playhead to split
  QVector<Clip*> selected_clips = SelectedClips(true);
  if (selected_clips.size() > 0) {
    // see if whole clips are selected
    QVector<Clip*> pre_clips;
    QVector<ClipPtr> post_clips;

    for (int i=0;i<selected_clips.size();i++) {
      Clip* c = selected_clips.at(i);

      ClipPtr s = SplitClip(ca, true, c, playhead);

      if (s != nullptr) {
        pre_clips.append(c);
        post_clips.append(s);
        split_occurred = true;
      }
    }

    if (split_occurred) {

      // relink clips if we split
      olive::timeline::RelinkClips(pre_clips, post_clips);
      ca->append(new AddClipCommand(post_clips));

    }
  }

  // If we weren't able to split any selected clips above, see if there are arbitrary selections to split
  if (!split_occurred) {

    TrackList* track_list;
    Track* track;

    foreach (track_list, track_lists_) {

      QVector<Track*> tracks = track_list->tracks();
      foreach (track, tracks) {

        QVector<Selection> track_selections = track->Selections();
        QVector<long> split_positions;

        for (int j=0;j<track_selections.size();j++) {
          const Selection& s = track_selections.at(j);

          if (!split_positions.contains(s.in())) {
            split_positions.append(s.in());
          }

          if (!split_positions.contains(s.out())) {
            split_positions.append(s.out());
          }
        }

        for (int i=0;i<track->ClipCount();i++) {
          Clip* c = track->GetClip(i).get();

          if (SplitClipAtPositions(ca, c, split_positions, false)) {
            split_occurred = true;
          }
        }
      }
    }
  }

  // if nothing was selected or no selections fell within playhead, simply split at playhead
  if (!split_occurred) {
    split_occurred = SplitAllClipsAtPoint(ca, playhead);
  }

  if (split_occurred) {
    olive::undo_stack.push(ca);
    update_ui(true);
  } else {
    delete ca;
  }
}

void Sequence::DeleteAreas(ComboAction* ca, QVector<Selection> areas, bool deselect_areas, bool ripple)
{
  if (areas.isEmpty()) {
    return;
  }

  Selection::Tidy(areas);

  panel_graph_editor->set_row(nullptr);
  panel_effect_controls->Clear(true);

  QVector<Clip*> pre_clips;
  QVector<ClipPtr> post_clips;

  QVector<Clip*> all_clips = GetAllClips();

  for (int i=0;i<areas.size();i++) {
    const Selection& s = areas.at(i);
    for (int j=0;j<all_clips.size();j++) {
      Clip* c = all_clips.at(j);
      if (c->track() == s.track() && !c->undeletable) {
        if (s.ContainsTransition(c, kTransitionOpening)) {
          // delete opening transition
          ca->append(new DeleteTransitionCommand(c->opening_transition));
        } else if (s.ContainsTransition(c, kTransitionClosing)) {
          // delete closing transition
          ca->append(new DeleteTransitionCommand(c->closing_transition));
        } else if (c->timeline_in() >= s.in() && c->timeline_out() <= s.out()) {
          // clips falls entirely within deletion area
          ca->append(new DeleteClipAction(c));
        } else if (c->timeline_in() < s.in() && c->timeline_out() > s.out()) {
          // middle of clip is within deletion area

          // duplicate clip
          ClipPtr post = SplitClip(ca, true, c, s.in(), s.out());

          pre_clips.append(c);
          post_clips.append(post);
        } else if (c->timeline_in() < s.in() && c->timeline_out() > s.in()) {
          // only out point is in deletion area
          MoveClip(c, ca, c->timeline_in(), s.in(), c->clip_in(), c->track());

          if (c->closing_transition != nullptr) {
            if (s.in() < c->timeline_out() - c->closing_transition->get_true_length()) {
              ca->append(new DeleteTransitionCommand(c->closing_transition));
            } else {
              ca->append(new ModifyTransitionCommand(c->closing_transition,
                                                     c->closing_transition->get_true_length() - (c->timeline_out() - s.in())));
            }
          }
        } else if (c->timeline_in() < s.out() && c->timeline_out() > s.out()) {
          // only in point is in deletion area
          MoveClip(c, ca, s.out(), c->timeline_out(), c->clip_in() + (s.out() - c->timeline_in()), c->track());

          if (c->opening_transition != nullptr) {
            if (s.out() > c->timeline_in() + c->opening_transition->get_true_length()) {
              ca->append(new DeleteTransitionCommand(c->opening_transition));
            } else {
              ca->append(new ModifyTransitionCommand(c->opening_transition,
                                                     c->opening_transition->get_true_length() - (s.out() - c->timeline_in())));
            }
          }
        }
      }
    }
  }

  // deselect selected clip areas
  long minimum_in = LONG_MAX;
  long minimum_length = LONG_MAX;
  if (deselect_areas) {
    QVector<Selection> area_copy = areas;
    for (int i=0;i<area_copy.size();i++) {
      const Selection& s = area_copy.at(i);
      s.track()->DeselectArea(s.in(), s.out());

      // Get ripple point and ripple length
      minimum_in = qMin(minimum_in, s.in());
      minimum_length = qMin(minimum_length, s.in() - s.out());
    }
  }

  if (ripple) {
    RippleDeleteArea(ca, minimum_in, minimum_length);
  }

  olive::timeline::RelinkClips(pre_clips, post_clips);
  ca->append(new AddClipCommand(post_clips));
}

bool Sequence::SplitAllClipsAtPoint(ComboAction *ca, long point)
{
  bool split = false;

  QVector<Clip*> all_clips = GetAllClips();

  for (int j=0;j<all_clips.size();j++) {
    Clip* c = all_clips.at(j);

    if (c->IsActiveAt(point)) {
      SplitClipAtPositions(ca, c, {point}, true);
      split = true;
    }
  }

  return split;
}

bool Sequence::SplitClipAtPositions(ComboAction *ca, Clip* clip, QVector<long> positions, bool also_split_links)
{
  // Add the clip and each of its links to the pre_splits array

  bool split_occurred = false;

  QVector<Clip*> pre_splits;
  pre_splits.append(clip);

  if (also_split_links) {
    for (int i=0;i<clip->linked.size();i++)   {
      pre_splits.append(clip->linked.at(i));
    }
  }

  std::sort(positions.begin(), positions.end());

  // Remove any duplicate positions
  for (int i=1;i<positions.size();i++) {
    if (positions.at(i-1) == positions.at(i)) {
      positions.removeAt(i);
      i--;
    }
  }

  QVector< QVector<ClipPtr> > post_splits(positions.size());

  for (int i=positions.size()-1;i>=0;i--) {

    post_splits[i].resize(pre_splits.size());

    for (int j=0;j<pre_splits.size();j++) {
      post_splits[i][j] = SplitClip(ca, true, pre_splits.at(j), positions.at(i));

      if (post_splits[i][j] != nullptr) {
        split_occurred = true;

        if (i + 1 < positions.size()) {
          post_splits[i][j]->set_timeline_out(qMin(post_splits[i][j]->timeline_out(),
                                                   positions.at(i+1)));
        }
      }
    }
  }

  for (int i=0;i<post_splits.size();i++) {
    olive::timeline::RelinkClips(pre_splits, post_splits[i]);
    ca->append(new AddClipCommand(post_splits[i]));
  }

  return split_occurred;
}

void Sequence::RippleDeleteEmptySpace(ComboAction* ca, Track* track, long point)
{
  QVector<Clip*> track_clips = track->GetAllClips();

  long ripple_start = LONG_MAX;
  long ripple_end = LONG_MAX;

  for (int i=0;i<track_clips.size();i++) {
    Clip* c = track_clips.at(i);

    if (c->timeline_in() <= point && c->timeline_out() >= point) {
      // This point is not actually empty, so there's nothing to do here
      return;
    }

    if (c->timeline_out() <= point) {

      ripple_start = qMin(c->timeline_out(), ripple_start);

    } else if (c->timeline_in() >= point) {

      ripple_end = qMin(c->timeline_in(), ripple_end);

    }
  }

  // We now know the maximum ripple we could do to clear this empty space, but we need to ensure it won't cause
  // overlaps of clips in other tracks

  if (ripple_start == ripple_end) {
    return;
  }

  RippleDeleteArea(ca, point, ripple_start - ripple_end);
}

void Sequence::RippleDeleteArea(ComboAction* ca, long ripple_point, long ripple_length) {

  for (int i=0;i<track_lists_.size();i++) {
    TrackList* tl = track_lists_.at(i);

    for (int j=0;j<tl->TrackCount();j++) {
      Track* t = tl->TrackAt(j);

      // We've already tested `track`, so we don't need to test it again
      long first_in_point_after_point = LONG_MAX;
      long out_point_just_before_first_in_point = LONG_MIN;

      QVector<Clip*> track_clips = t->GetAllClips();

      // Find the in point of the clip directly after the point
      for (int k=0;k<track_clips.size();k++) {
        Clip* c = track_clips.at(k);

        if (c->timeline_in() >= ripple_point) {
          first_in_point_after_point = qMin(first_in_point_after_point, c->timeline_in());
        }
      }

      // Ensure we found a valid in point before proceeding
      if (first_in_point_after_point != LONG_MAX) {

        // Find the out point of the clip directly before the clip found above
        for (int k=0;k<track_clips.size();k++) {
          Clip* c = track_clips.at(k);

          if (c->timeline_out() <= first_in_point_after_point) {
            out_point_just_before_first_in_point = qMax(out_point_just_before_first_in_point, c->timeline_out());
          }
        }

        long ripple_test = first_in_point_after_point - out_point_just_before_first_in_point + ripple_length;

        if (ripple_test < 0) {
          ripple_length -= ripple_test;
        }
      }
    }
  }

  if (ripple_length != 0) {
    Ripple(ca, ripple_point, ripple_length);
  }

}

Node *Sequence::GetSelectedGizmo()
{
  Node* gizmo_ptr = nullptr;

  QVector<Clip*> clips = GetAllClips();

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i);
    if (c->IsActiveAt(playhead)
        && c->IsSelected()) {
      // This clip is selected and currently active - we'll use this for gizmos

      if (!c->effects.isEmpty()) {

        // find which effect has gizmos selected, or default to the first gizmo effect we find if there is
        // none selected

        for (int j=0;j<c->effects.size();j++) {
          Node* e = c->effects.at(j).get();

          // retrieve gizmo data from effect
          if (e->are_gizmos_enabled()) {
            if (gizmo_ptr == nullptr) {
              gizmo_ptr = e;
            }
            if (panel_effect_controls->IsEffectSelected(e)) {
              gizmo_ptr = e;
              break;
            }
          }
        }
      }

      if (gizmo_ptr != nullptr) {
        break;
      }
    }
  }

  return gizmo_ptr;
}

void Sequence::SelectAll()
{
  for (int j=0;j<track_lists_.size();j++) {
    TrackList* tl = track_lists_.at(j);

    for (int i=0;i<tl->TrackCount();i++) {
      tl->TrackAt(i)->SelectAll();
    }
  }
}

void Sequence::SelectAtPlayhead()
{
  for (int j=0;j<track_lists_.size();j++) {
    TrackList* tl = track_lists_.at(j);

    for (int i=0;i<tl->TrackCount();i++) {
      tl->TrackAt(i)->SelectAtPoint(playhead);
    }
  }
}

void Sequence::ClearSelections()
{
  for (int j=0;j<track_lists_.size();j++) {
    TrackList* tl = track_lists_.at(j);

    for (int i=0;i<tl->TrackCount();i++) {
      tl->TrackAt(i)->ClearSelections();
    }
  }
}

void Sequence::AddSelectionsToClipboard(bool delete_originals)
{
  olive::clipboard.Clear();
  olive::clipboard.SetType(Clipboard::CLIPBOARD_TYPE_CLIP);

  QVector<Clip*> original_clips;
  QVector<ClipPtr> copied_clips;

  long min_in = LONG_MAX;

  QVector<Selection> selections = Selections();
  for (int i=0;i<selections.size();i++) {
    const Selection& s = selections.at(i);

    // Get the clips contained in the track this selection pertains to
    QVector<Clip*> track_clips = s.track()->GetAllClips();

    for (int j=0;j<track_clips.size();j++) {

      Clip* c = track_clips.at(j);

      // Check if this selection contains this clip
      if (!(c->timeline_out() < s.in() || c->timeline_in() > s.out())) {

        // If so, we'll be copying this clip
        original_clips.append(c);

        ClipPtr copy = c->copy(nullptr);

        // If we only copied part of this clip, adjust the copy so it's only that part of the clip
        if (copy->timeline_in() < s.in()) {
          copy->set_clip_in(copy->clip_in() + (s.in() - copy->timeline_in()));
          copy->set_timeline_in(s.in());
        }

        if (copy->timeline_out() > s.out()) {
          copy->set_timeline_out(s.out());
        }

        // Store the minimum in point as all copies will be stored offset from 0
        min_in = qMin(min_in, s.in());

        copied_clips.append(copy);
        olive::clipboard.Append(copy);

      }
    }
  }

  // Determine whether we actually copied anything
  if (min_in < LONG_MAX) {

    // Offset all copied clips to 0
    for (int i=0;i<copied_clips.size();i++) {
      Clip* copy = copied_clips.at(i).get();

      copy->set_timeline_in(copy->timeline_in() - min_in);
      copy->set_timeline_out(copy->timeline_out() - min_in);
    }

    // Relink the copied clips with each other
    olive::timeline::RelinkClips(original_clips, copied_clips);

    // If we're deleting the originals (i.e. cutting), delete them now
    if (delete_originals) {
      ComboAction* ca = new ComboAction();
      DeleteAreas(ca, selections, true);
      olive::undo_stack.push(ca);
    }

  }
}

QVector<Selection> Sequence::Selections()
{
  QVector<Selection> selections;

  for (int j=0;j<track_lists_.size();j++) {
    TrackList* tl = track_lists_.at(j);

    for (int i=0;i<tl->TrackCount();i++) {
      selections.append(tl->TrackAt(i)->Selections());
    }
  }

  return selections;
}

void Sequence::SetSelections(const QVector<Selection> &selections)
{
  ClearSelections();

  for (int i=0;i<selections.size();i++) {
    const Selection& s = selections.at(i);

    s.track()->SelectArea(s.in(), s.out());
  }
}

void Sequence::TidySelections()
{
  QVector<Selection> selections = Selections();
  Selection::Tidy(selections);
  SetSelections(selections);
}

ClipPtr Sequence::SplitClip(ComboAction *ca, bool transitions, Clip* pre, long frame)
{
  return SplitClip(ca, transitions, pre, frame, frame);
}

ClipPtr Sequence::SplitClip(ComboAction *ca, bool transitions, Clip* pre, long frame, long post_in)
{
  if (pre == nullptr) {
    return nullptr;
  }

  if (pre->timeline_in() < frame && pre->timeline_out() > frame) {
    // duplicate clip without duplicating its transitions, we'll restore them later

    ClipPtr post = pre->copy(pre->track());

    long new_clip_length = frame - pre->timeline_in();

    post->set_timeline_in(post_in);
    post->set_clip_in(pre->clip_in() + (post->timeline_in() - pre->timeline_in()));

    MoveClip(pre, ca, pre->timeline_in(), frame, pre->clip_in(), pre->track(), false);

    if (transitions) {

      // check if this clip has a closing transition
      if (pre->closing_transition != nullptr) {

        // if so, move closing transition to the post clip
        post->closing_transition = pre->closing_transition;

        // and set the original clip's closing transition to nothing
        ca->append(new SetPointer(reinterpret_cast<void**>(&pre->closing_transition), nullptr));

        // and set the transition's reference to the post clip
        if (post->closing_transition->parent_clip == pre) {
          ca->append(new SetPointer(reinterpret_cast<void**>(&post->closing_transition->parent_clip), post.get()));
        }
        if (post->closing_transition->secondary_clip == pre) {
          ca->append(new SetPointer(reinterpret_cast<void**>(&post->closing_transition->secondary_clip), post.get()));
        }

        // and make sure it's at the correct size to the closing clip
        if (post->closing_transition != nullptr && post->closing_transition->get_true_length() > post->length()) {
          ca->append(new ModifyTransitionCommand(post->closing_transition, post->length()));
          post->closing_transition->set_length(post->length());
        }

      }

      // we're keeping the opening clip, so ensure that's a correct size too
      if (pre->opening_transition != nullptr && pre->opening_transition->get_true_length() > new_clip_length) {
        ca->append(new ModifyTransitionCommand(pre->opening_transition, new_clip_length));
      }
    }

    return post;

  } else if (frame == pre->timeline_in()
             && pre->opening_transition != nullptr
             && pre->opening_transition->secondary_clip != nullptr) {
    // special case for shared transitions to split it into two

    // set transition to single-clip mode
    ca->append(new SetPointer(reinterpret_cast<void**>(&pre->opening_transition->secondary_clip), nullptr));

    // clone transition for other clip
    ca->append(new AddTransitionCommand(nullptr,
                                        pre->opening_transition->secondary_clip,
                                        pre->opening_transition,
                                        kInvalidNode,
                                        0)
               );

  }

  return nullptr;
}

bool Sequence::SplitSelection(ComboAction *ca, QVector<Selection> selections)
{
  bool ret = false;
  QVector<Clip*> all_clips = GetAllClips();

  for (int i=0;i<all_clips.size();i++) {
    Clip* c = all_clips.at(i);

    QVector<long> points;

    for (int j=0;j<selections.size();j++) {
      const Selection& s = selections.at(j);

      if (s.track() == c->track()) {
        if (c->timeline_in() < s.in() && c->timeline_out() > s.in()) {
          points.append(s.in());
        }
        if (c->timeline_in() < s.out() && c->timeline_out() > s.out()) {
          points.append(s.out());
        }
      }
    }

    if (SplitClipAtPositions(ca, c, points, false)) {
      ret = true;
    }
  }

  return ret;
}
