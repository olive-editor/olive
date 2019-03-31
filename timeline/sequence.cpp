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

#include "panels/panels.h"
#include "project/clipboard.h"
#include "global/debug.h"

Sequence::Sequence() :
  playhead(0),
  using_workarea(false),
  workarea_in(0),
  workarea_out(0),
  wrapper_sequence(false)
{
  // Set up tracks
  track_lists_.resize(Track::kTypeCount);

  for (int i=0;i<track_lists_.size();i++) {
    track_lists_[i] = new TrackList(this, static_cast<Track::Type>(i));
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
  if (this == olive::ActiveSequence.get()) {
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

TrackList *Sequence::GetTrackList(Track::Type type)
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

      selected_clips.append(t->GetAllClips());
    }
  }

  return selected_clips;
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
    ca->append(new SetTimelineInOutCommand(olive::ActiveSequence.get(), false, 0, 0));
    olive::UndoStack.push(ca);
    update_ui(true);
  }
}

void Sequence::Ripple(ComboAction *ca, long point, long length, const QVector<int> &ignore)
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

void Sequence::DeleteAreas(ComboAction* ca, QVector<Selection> areas, bool deselect_areas)
{
  Selection::Tidy(areas);

  panel_graph_editor->set_row(nullptr);
  panel_effect_controls->Clear(true);

  QVector<int> pre_clips;
  QVector<ClipPtr> post_clips;

  QVector<Clip*> all_clips = GetAllClips();

  for (int i=0;i<areas.size();i++) {
    const Selection& s = areas.at(i);
    for (int j=0;j<all_clips.size();j++) {
      Clip* c = all_clips.at(j);
      if (c->track() == s.track() && !c->undeletable) {
        if (selection_contains_transition(s, c, kTransitionOpening)) {
          // delete opening transition
          ca->append(new DeleteTransitionCommand(c->opening_transition));
        } else if (selection_contains_transition(s, c, kTransitionClosing)) {
          // delete closing transition
          ca->append(new DeleteTransitionCommand(c->closing_transition));
        } else if (c->timeline_in() >= s.in() && c->timeline_out() <= s.out()) {
          // clips falls entirely within deletion area
          ca->append(new DeleteClipAction(c));
        } else if (c->timeline_in() < s.in() && c->timeline_out() > s.out()) {
          // middle of clip is within deletion area

          // duplicate clip
          ClipPtr post = SplitClip(ca, true, c, s.in(), s.out());

          pre_clips.append(j);
          post_clips.append(post);
        } else if (c->timeline_in() < s.in() && c->timeline_out() > s.in()) {
          // only out point is in deletion area
          c->move(ca, c->timeline_in(), s.in(), c->clip_in(), c->track());

          if (c->closing_transition != nullptr) {
            if (s.in() < c->timeline_out() - c->closing_transition->get_true_length()) {
              ca->append(new DeleteTransitionCommand(c->closing_transition));
            } else {
              ca->append(new ModifyTransitionCommand(c->closing_transition, c->closing_transition->get_true_length() - (c->timeline_out() - s.in)));
            }
          }
        } else if (c->timeline_in() < s.out() && c->timeline_out() > s.out()) {
          // only in point is in deletion area
          c->move(ca, s.out(), c->timeline_out(), c->clip_in() + (s.out() - c->timeline_in()), c->track());

          if (c->opening_transition != nullptr) {
            if (s.out() > c->timeline_in() + c->opening_transition->get_true_length()) {
              ca->append(new DeleteTransitionCommand(c->opening_transition));
            } else {
              ca->append(new ModifyTransitionCommand(c->opening_transition, c->opening_transition->get_true_length() - (s.out - c->timeline_in())));
            }
          }
        }
      }
    }
  }

  // deselect selected clip areas
  if (deselect_areas) {
    QVector<Selection> area_copy = areas;
    for (int i=0;i<area_copy.size();i++) {
      const Selection& s = area_copy.at(i);
      deselect_area(s.in, s.out, s.track);
    }
  }

  relink_clips_using_ids(pre_clips, post_clips);
  ca->append(new AddClipCommand(olive::ActiveSequence.get(), post_clips));
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

void Sequence::SplitClipAtPositions(ComboAction *ca, Clip* clip, QVector<long> positions, bool relink)
{
  // Add the clip and each of its links to the pre_splits array

  QVector<Clip*> pre_splits;
  pre_splits.append(clip);

  if (relink) {
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

      if (post_splits[i][j] != nullptr && i + 1 < positions.size()) {
        post_splits[i][j]->set_timeline_out(positions.at(i+1));
      }
    }
  }

  for (int i=0;i<post_splits.size();i++) {
    olive::timeline::RelinkClips(pre_splits, post_splits[i]);
    ca->append(new AddClipCommand(olive::ActiveSequence.get(), post_splits[i]));
  }
}

/*
QVector<int> Sequence::SelectedClipIndexes()
{
  QVector<int> selected_clips;

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i).get();
    if (c != nullptr && IsClipSelected(c, true)) {
      selected_clips.append(i);
    }
  }

  return selected_clips;
}
*/

Effect *Sequence::GetSelectedGizmo()
{
  Effect* gizmo_ptr = nullptr;

  QVector<Clip*> clips = GetAllClips();

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i);
    if (c->IsActiveAt(playhead)
        && IsClipSelected(c, true)) {
      // This clip is selected and currently active - we'll use this for gizmos

      if (!c->effects.isEmpty()) {

        // find which effect has gizmos selected, or default to the first gizmo effect we find if there is
        // none selected

        for (int j=0;j<c->effects.size();j++) {
          Effect* e = c->effects.at(j).get();

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
      olive::UndoStack.push(ca);
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

    pre->move(ca, pre->timeline_in(), frame, pre->clip_in(), pre->track(), false);

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
                                        nullptr,
                                        0)
               );

  }

  return nullptr;
}

// static variable for the currently active sequence
SequencePtr olive::ActiveSequence = nullptr;
