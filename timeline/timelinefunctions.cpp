#include "timelinefunctions.h"

#include "global/math.h"
#include "global/config.h"
#include "global/timing.h"
#include "sequence.h"

// snapping
bool olive::timeline::snapping = true;
bool olive::timeline::snapped = false;
long olive::timeline::snap_point = 0;

void olive::timeline::RelinkClips(QVector<Clip *> &pre_clips, QVector<ClipPtr> &post_clips)
{
  // Loop through each "pre" clip
  for (int i=0;i<pre_clips.size();i++) {

    Clip* pre = pre_clips.at(i);

    // Loop through each "pre" clip's linked array
    for (int j=0;j<pre->linked.size();j++) {

      // Loop again through the "pre" clips to determine which are linked to each other
      for (int l=0;l<pre_clips.size();l++) {

        // If this "pre" is linked to this "pre"
        if (pre->linked.at(j) == pre_clips.at(l)) {

          // Check if we have an equivalent in the post_clips, and link it if so
          if (post_clips.at(l) != nullptr) {
            post_clips.at(i)->linked.append(post_clips.at(l).get());
          }

        }
      }
    }
  }
}

bool olive::timeline::SnapToPoint(long point, long* l, double zoom) {
  long limit = getFrameFromScreenPoint(zoom, 10); // FIXME magic number 10 used for on screen pixel threshold to snap to
  if (*l > point-limit-1 && *l < point+limit+1) {
    olive::timeline::snap_point = point;
    *l = point;
    olive::timeline::snapped = true;
    return true;
  }
  return false;
}


QVector<Ghost> olive::timeline::CreateGhostsFromMedia(Sequence *seq,
                                                      long entry_point,
                                                      QVector<olive::timeline::MediaImportData> &media_list)
{
  QVector<Ghost> ghosts;

  for (int i=0;i<media_list.size();i++) {
    bool can_import = true;

    const olive::timeline::MediaImportData import_data = media_list.at(i);
    Media* medium = import_data.media();
    Footage* m = nullptr;
    Sequence* s = nullptr;
    long sequence_length = 0;
    long default_clip_in = 0;
    long default_clip_out = 0;

    switch (medium->get_type()) {
    case MEDIA_TYPE_FOOTAGE:
      m = medium->to_footage();
      can_import = m->ready;
      if (m->using_inout) {
        double source_fr = 30;
        if (m->video_tracks.size() > 0 && !qIsNull(m->video_tracks.at(0).video_frame_rate)) {
          source_fr = m->video_tracks.at(0).video_frame_rate * m->speed;
        }
        default_clip_in = rescale_frame_number(m->in, source_fr, seq->frame_rate);
        default_clip_out = rescale_frame_number(m->out, source_fr, seq->frame_rate);
      }
      break;
    case MEDIA_TYPE_SEQUENCE:
      s = medium->to_sequence().get();
      sequence_length = s->GetEndFrame();
      if (seq != nullptr) sequence_length = rescale_frame_number(sequence_length, s->frame_rate, seq->frame_rate);
      can_import = (s != seq && sequence_length != 0);
      if (s->using_workarea) {
        default_clip_in = rescale_frame_number(s->workarea_in, s->frame_rate, seq->frame_rate);
        default_clip_out = rescale_frame_number(s->workarea_out, s->frame_rate, seq->frame_rate);
      }
      break;
    default:
      can_import = false;
    }

    if (can_import) {
      Ghost g;
      g.clip = nullptr;
      g.trim_type = olive::timeline::TRIM_NONE;
      g.old_clip_in = g.clip_in = default_clip_in;
      g.media = medium;
      g.in = entry_point;
      g.transition = nullptr;

      switch (medium->get_type()) {
      case MEDIA_TYPE_FOOTAGE:
        // is video source a still image?
        if (m->video_tracks.size() > 0 && m->video_tracks.at(0).infinite_length && m->audio_tracks.size() == 0) {
          g.out = g.in + 100;
        } else {
          long length = m->get_length_in_frames(seq->frame_rate);
          g.out = entry_point + length - default_clip_in;
          if (m->using_inout) {
            g.out -= (length - default_clip_out);
          }
        }

        if (import_data.type() == olive::timeline::kImportAudioOnly
            || import_data.type() == olive::timeline::kImportBoth) {
          for (int j=0;j<m->audio_tracks.size();j++) {
            if (m->audio_tracks.at(j).enabled) {
              g.track = seq->GetTrackList(Track::kTypeAudio)->TrackAt(j);
              g.media_stream = m->audio_tracks.at(j).file_index;
              ghosts.append(g);
            }
          }
        }

        if (import_data.type() == olive::timeline::kImportVideoOnly
            || import_data.type() == olive::timeline::kImportBoth) {
          for (int j=0;j<m->video_tracks.size();j++) {
            if (m->video_tracks.at(j).enabled) {
              g.track = seq->GetTrackList(Track::kTypeVideo)->TrackAt(j);
              g.media_stream = m->video_tracks.at(j).file_index;
              ghosts.append(g);
            }
          }
        }
        break;
      case MEDIA_TYPE_SEQUENCE:
        g.out = entry_point + sequence_length - default_clip_in;

        if (s->using_workarea) {
          g.out -= (sequence_length - default_clip_out);
        }

        if (import_data.type() == olive::timeline::kImportVideoOnly
            || import_data.type() == olive::timeline::kImportBoth) {
          g.track = seq->GetTrackList(Track::kTypeVideo)->First();
          ghosts.append(g);
        }

        if (import_data.type() == olive::timeline::kImportAudioOnly
            || import_data.type() == olive::timeline::kImportBoth) {
          g.track = seq->GetTrackList(Track::kTypeAudio)->First();
          ghosts.append(g);
        }

        break;
      }
      entry_point = g.out;
    }
  }
  for (int i=0;i<ghosts.size();i++) {
    Ghost& g = ghosts[i];
    g.old_in = g.in;
    g.old_out = g.out;
    g.track_movement = 0;
  }

  return ghosts;
}
