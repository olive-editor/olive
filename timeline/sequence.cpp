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
#include "global/debug.h"

Sequence::Sequence() :
  playhead(0),
  using_workarea(false),
  workarea_in(0),
  workarea_out(0),
  wrapper_sequence(false)
{
  // Set up tracks
  track_lists.resize(Track::kTypeCount);

  for (int i=0;i<track_lists.size();i++) {
    track_lists[i] = std::make_shared<TrackList>(this, i);
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
  for (int i=0;i<track_lists.size();i++) {
    s->track_lists[i] = track_lists.at(i).copy(s.get());
  }

  // copy all of the sequence's markers
  s->markers = markers;

  return s;
}

void Sequence::Save(QXmlStreamWriter &stream)
{
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

  for (int j=0;j<tracks.size();j++) {
    tracks.at(j)->Save(stream);
  }


  for (int j=0;j<markers.size();j++) {
    markers.at(j).Save(stream);
  }
  stream.writeEndElement();
}

long Sequence::GetEndFrame() {
  long end_frame = 0;

  for (int i=0;i<tracks.size();i++) {
    end_frame = qMax(tracks.at(i)->GetEndFrame(), end_frame);
  }

  return end_frame;
}

QVector<Clip *> Sequence::GetAllClips()
{
  QVector<Clip*> all_clips;

  for (int i=0;i<tracks.size();i++) {
    all_clips.append(tracks.at(i)->GetAllClips());
  }

  return all_clips;
}

TrackList *Sequence::GetTrackList(Track::Type type)
{
  return track_lists.at(type).get();
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
  QVector<Clip*> all_clips = GetAllClips();

  QVector<Clip*> selected_clips;

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i).get();
    if (c != nullptr && IsClipSelected(c, containing)) {
      selected_clips.append(c);
    }
  }

  return selected_clips;
}

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

Effect *Sequence::GetSelectedGizmo()
{
  Effect* gizmo_ptr = nullptr;

  for (int i=0;i<clips.size();i++) {
    Clip* c = clips.at(i).get();
    if (c != nullptr
        && c->IsActiveAt(playhead)
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

void Sequence::ClearSelections()
{
  for (int i=0;i<tracks.size();i++) {
    tracks.at(i)->ClearSelections();
  }
}

// static variable for the currently active sequence
SequencePtr olive::ActiveSequence = nullptr;
