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

#include "timelineview.h"

#include <QPainter>
#include <QColor>
#include <QMouseEvent>
#include <QObject>
#include <QVariant>
#include <QPoint>
#include <QMessageBox>
#include <QtMath>
#include <QScrollBar>
#include <QMimeData>
#include <QToolTip>
#include <QInputDialog>
#include <QStatusBar>

#include "global/global.h"
#include "panels/panels.h"
#include "project/projectelements.h"
#include "rendering/audio.h"
#include "global/config.h"
#include "global/timing.h"
#include "ui/sourcetable.h"
#include "ui/sourceiconview.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "ui/viewerwidget.h"
#include "ui/resizablescrollbar.h"
#include "dialogs/newsequencedialog.h"
#include "mainwindow.h"
#include "ui/rectangleselect.h"
#include "rendering/renderfunctions.h"
#include "ui/cursors.h"
#include "ui/menuhelper.h"
#include "ui/menu.h"
#include "ui/focusfilter.h"
#include "dialogs/clippropertiesdialog.h"
#include "global/debug.h"
#include "effects/effect.h"
#include "effects/internal/solideffect.h"
#include "timeline/track.h"
#include "global/math.h"
#include "project/projectfunctions.h"
#include "ui/waveform.h"

#define MAX_TEXT_WIDTH 20
#define TRANSITION_BETWEEN_RANGE 40

TimelineView::TimelineView(Timeline *parent) :
  timeline_(parent),
  self_created_sequence(nullptr),
  track_list_(nullptr),
  scroll(0),
  alignment_(olive::timeline::kAlignmentTop),
  track_resizing(false)
{
  setMouseTracking(true);

  setFocusPolicy(Qt::ClickFocus);

  setAcceptDrops(true);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));

  tooltip_timer.setInterval(500);
  connect(&tooltip_timer, SIGNAL(timeout()), this, SLOT(tooltip_timer_timeout()));
}

void TimelineView::SetAlignment(olive::timeline::Alignment alignment)
{
  alignment_ = alignment;
}

void TimelineView::SetTrackList(TrackList *tl)
{
  track_list_ = tl;

  update();
}

void TimelineView::show_context_menu(const QPoint& pos) {
  if (sequence() != nullptr) {
    // hack because sometimes right clicking doesn't trigger mouse release event
    ParentTimeline()->rect_select_init = false;
    ParentTimeline()->rect_select_proc = false;

    Menu menu(this);

    QAction* undoAction = menu.addAction(tr("&Undo"));
    QAction* redoAction = menu.addAction(tr("&Redo"));
    connect(undoAction, SIGNAL(triggered(bool)), olive::Global.get(), SLOT(undo()));
    connect(redoAction, SIGNAL(triggered(bool)), olive::Global.get(), SLOT(redo()));
    undoAction->setEnabled(olive::undo_stack.canUndo());
    redoAction->setEnabled(olive::undo_stack.canRedo());
    menu.addSeparator();

    // collect all the selected clips
    QVector<Clip*> selected_clips = sequence()->SelectedClips();

    olive::MenuHelper.make_edit_functions_menu(&menu, !selected_clips.isEmpty());

    if (selected_clips.isEmpty()) {
      // no clips are selected

      // determine if we can perform a ripple empty space
      ParentTimeline()->cursor_frame = ParentTimeline()->getTimelineFrameFromScreenPoint(pos.x());
      ParentTimeline()->cursor_track = getTrackFromScreenPoint(pos.y());

      // check if the space the cursor is currently at is empty
      if (ParentTimeline()->cursor_track->GetClipFromPoint(ParentTimeline()->cursor_frame) == nullptr) {
        QAction* ripple_delete_action = menu.addAction(tr("R&ipple Delete Empty Space"));
        connect(ripple_delete_action, SIGNAL(triggered(bool)), ParentTimeline(), SLOT(ripple_delete_empty_space()));
      }

      QAction* seq_settings = menu.addAction(tr("Sequence Settings"));
      connect(seq_settings, SIGNAL(triggered(bool)), this, SLOT(open_sequence_properties()));
    }

    if (!selected_clips.isEmpty()) {

      bool video_clips_are_selected = false;
      bool audio_clips_are_selected = false;

      for (int i=0;i<selected_clips.size();i++) {
        if (selected_clips.at(i)->type() == Track::kTypeVideo) {
          video_clips_are_selected = true;
        } else {
          audio_clips_are_selected = true;
        }
      }

      menu.addSeparator();

      menu.addAction(tr("&Speed/Duration"), olive::Global.get(), SLOT(open_speed_dialog()));

      if (audio_clips_are_selected) {
        menu.addAction(tr("Auto-Cut Silence"), olive::Global.get(), SLOT(open_autocut_silence_dialog()));
      }

      QAction* autoscaleAction = menu.addAction(tr("Auto-S&cale"), this, SLOT(toggle_autoscale()));
      autoscaleAction->setCheckable(true);
      // set autoscale to the first selected clip
      autoscaleAction->setChecked(selected_clips.at(0)->autoscaled());

      olive::MenuHelper.make_clip_functions_menu(&menu);

      // check if all selected clips have the same media for a "Reveal In Project"
      bool same_media = true;
      rc_reveal_media = selected_clips.at(0)->media();
      for (int i=1;i<selected_clips.size();i++) {
        if (selected_clips.at(i)->media() != rc_reveal_media) {
          same_media = false;
          break;
        }
      }

      if (same_media) {
        QAction* revealInProjectAction = menu.addAction(tr("&Reveal in Project"));
        connect(revealInProjectAction, SIGNAL(triggered(bool)), this, SLOT(reveal_media()));
      }

      menu.addAction(tr("Properties"), this, SLOT(show_clip_properties()));
    }

    menu.exec(mapToGlobal(pos));
  }
}

void TimelineView::toggle_autoscale() {
  QVector<Clip*> selected_clips = sequence()->SelectedClips();

  if (!selected_clips.isEmpty()) {
    SetClipProperty* action = new SetClipProperty(kSetClipPropertyAutoscale);

    for (int i=0;i<selected_clips.size();i++) {
      Clip* c = selected_clips.at(i);
      action->AddSetting(c, !c->autoscaled());
    }

    olive::undo_stack.push(action);
  }
}

void TimelineView::tooltip_timer_timeout() {
  if (tooltip_clip != nullptr) {
    QToolTip::showText(QCursor::pos(),
                       tr("%1\nStart: %2\nEnd: %3\nDuration: %4").arg(
                         tooltip_clip->name(),
                         frame_to_timecode(tooltip_clip->timeline_in(), olive::config.timecode_view, sequence()->frame_rate),
                         frame_to_timecode(tooltip_clip->timeline_out(), olive::config.timecode_view, sequence()->frame_rate),
                         frame_to_timecode(tooltip_clip->length(), olive::config.timecode_view, sequence()->frame_rate)
                         ));
  }

  tooltip_timer.stop();
}

void TimelineView::open_sequence_properties() {
  QVector<Media*> sequence_items = olive::project_model.GetAllSequences();

  for (int i=0;i<sequence_items.size();i++) {
    if (sequence_items.at(i)->to_sequence().get() == sequence()) {
      NewSequenceDialog nsd(this, sequence_items.at(i));
      nsd.exec();
      return;
    }
  }

  QMessageBox::critical(this, tr("Error"), tr("Couldn't locate media wrapper for sequence."));
}

void TimelineView::show_clip_properties()
{
  // get list of selected clips
  QVector<Clip*> selected_clips = sequence()->SelectedClips();

  // if clips are selected, open the clip properties dialog
  if (!selected_clips.isEmpty()) {
    ClipPropertiesDialog cpd(this, selected_clips);
    cpd.exec();
  }
}

void TimelineView::dragEnterEvent(QDragEnterEvent *event) {
  bool import_init = false;

  QVector<olive::timeline::MediaImportData> media_list;
  ParentTimeline()->importing_files = false;

  for (int i=0;i<panel_project.size();i++) {
    if (panel_project.at(i)->IsProjectWidget(event->source())) {

      QModelIndexList items = panel_project.at(i)->get_current_selected();

      media_list.resize(items.size());
      for (int i=0;i<items.size();i++) {
        media_list[i] = panel_project.at(i)->item_to_media(items.at(i));
      }
      import_init = true;

      break;

    }
  }

  if (event->source() == panel_footage_viewer) {
    if (panel_footage_viewer->seq.get() != sequence()) { // don't allow nesting the same sequence

      media_list.append(olive::timeline::MediaImportData(panel_footage_viewer->media,
                                                         static_cast<olive::timeline::MediaImportType>(event->mimeData()->text().toInt())));
      import_init = true;

    }
  }

  if (olive::config.enable_drag_files_to_timeline && event->mimeData()->hasUrls()) {
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
      QStringList file_list;

      for (int i=0;i<urls.size();i++) {
        file_list.append(urls.at(i).toLocalFile());
      }

      olive::project_model.process_file_list(file_list);

      QVector<Media*> last_imported_media = olive::project_model.GetLastImportedMedia();
      for (int i=0;i<last_imported_media.size();i++) {
        Footage* f = last_imported_media.at(i)->to_footage();

        // waits for media to have a duration
        // TODO would be much nicer if this was multithreaded
        f->ready_lock.lock();
        f->ready_lock.unlock();

        if (f->ready) {
          media_list.append(last_imported_media.at(i));
        }
      }

      if (media_list.isEmpty()) {
        olive::undo_stack.undo();
      } else {
        import_init = true;
        ParentTimeline()->importing_files = true;
      }
    }
  }

  if (import_init) {
    event->acceptProposedAction();

    long entry_point;
    Sequence* seq = sequence();

    if (seq == nullptr) {
      // if no sequence, we're going to create a new one using the clips as a reference
      entry_point = 0;

      self_created_sequence = olive::project::CreateSequenceFromMedia(media_list);
      seq = self_created_sequence.get();
    } else {
      entry_point = ParentTimeline()->getTimelineFrameFromScreenPoint(event->pos().x());
      ParentTimeline()->drag_frame_start = entry_point + getFrameFromScreenPoint(ParentTimeline()->zoom, 50);
      ParentTimeline()->drag_track_start = track_list_->First();
    }

    ParentTimeline()->ghosts = olive::timeline::CreateGhostsFromMedia(seq, entry_point, media_list);

    ParentTimeline()->importing = true;
  }
}

void TimelineView::dragMoveEvent(QDragMoveEvent *event) {
  if (ParentTimeline()->importing) {
    event->acceptProposedAction();

    if (sequence() != nullptr) {
      QPoint pos = event->pos();
      ParentTimeline()->scroll_to_frame(ParentTimeline()->getTimelineFrameFromScreenPoint(event->pos().x()));
      update_ghosts(pos, event->keyboardModifiers() & Qt::ShiftModifier);
      ParentTimeline()->move_insert = ((event->keyboardModifiers() & Qt::ControlModifier) && (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER || ParentTimeline()->importing));
      update_ui(false);
    }
  }
}

void TimelineView::wheelEvent(QWheelEvent *event) {
  static_cast<TimelineArea*>(parent())->wheelEvent(event);
}

void TimelineView::dragLeaveEvent(QDragLeaveEvent* event) {
  event->accept();
  if (ParentTimeline()->importing) {
    if (ParentTimeline()->importing_files) {
      olive::undo_stack.undo();
    }
    ParentTimeline()->importing_files = false;
    ParentTimeline()->ghosts.clear();
    ParentTimeline()->importing = false;
    update_ui(false);
  }
  if (self_created_sequence != nullptr) {
    self_created_sequence.reset();
    self_created_sequence = nullptr;
  }
}

void TimelineView::delete_area_under_ghosts(ComboAction* ca, Sequence* s) {
  // delete areas before adding
  QVector<Selection> delete_areas;
  for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
    delete_areas.append(ParentTimeline()->ghosts.at(i).ToSelection());
  }
  s->DeleteAreas(ca, delete_areas, false);
}

void TimelineView::insert_clips(ComboAction* ca, Sequence* s) {
  bool ripple_old_point = true;

  long earliest_old_point = LONG_MAX;
  long latest_old_point = LONG_MIN;

  long earliest_new_point = LONG_MAX;
  long latest_new_point = LONG_MIN;

  QVector<Clip*> ignore_clips;
  for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
    const Ghost& g = ParentTimeline()->ghosts.at(i);

    earliest_old_point = qMin(earliest_old_point, g.old_in);
    latest_old_point = qMax(latest_old_point, g.old_out);
    earliest_new_point = qMin(earliest_new_point, g.in);
    latest_new_point = qMax(latest_new_point, g.out);

    if (g.clip != nullptr) {
      ignore_clips.append(g.clip);
    } else {
      // don't try to close old gap if importing
      ripple_old_point = false;
    }
  }

  QVector<Clip*> sequence_clips = sequence()->GetAllClips();
  for (int i=0;i<sequence_clips.size();i++) {
    Clip* c = sequence_clips.at(i);
    // don't split any clips that are moving
    bool found = false;
    for (int j=0;j<ParentTimeline()->ghosts.size();j++) {
      if (ParentTimeline()->ghosts.at(j).clip == c) {
        found = true;
        break;
      }
    }
    if (!found) {
      if (c->timeline_in() < earliest_new_point && c->timeline_out() > earliest_new_point) {
        sequence()->SplitClipAtPositions(ca, c, {earliest_new_point}, true);
      }

      // determine if we should close the gap the old clips left behind
      if (ripple_old_point
          && !((c->timeline_in() < earliest_old_point && c->timeline_out() <= earliest_old_point) || (c->timeline_in() >= latest_old_point && c->timeline_out() > latest_old_point))
          && !ignore_clips.contains(c)) {
        ripple_old_point = false;
      }
    }
  }

  long ripple_length = (latest_new_point - earliest_new_point);

  sequence()->Ripple(ca, earliest_new_point, ripple_length, ignore_clips);

  if (ripple_old_point) {
    // works for moving later clips earlier but not earlier to later
    long second_ripple_length = (earliest_old_point - latest_old_point);

    sequence()->Ripple(ca, latest_old_point, second_ripple_length, ignore_clips);

    if (earliest_old_point < earliest_new_point) {
      for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
        Ghost& g = ParentTimeline()->ghosts[i];
        g.in += second_ripple_length;
        g.out += second_ripple_length;
      }

      QVector<Selection> sequence_selections = sequence()->Selections();
      for (int i=0;i<sequence_selections.size();i++) {
        Selection& s = sequence_selections[i];
        s.set_in(s.in() + second_ripple_length);
        s.set_out(s.out() + second_ripple_length);
      }
      sequence()->SetSelections(sequence_selections);
    }
  }
}

void TimelineView::dropEvent(QDropEvent* event) {
  if (ParentTimeline()->importing && ParentTimeline()->ghosts.size() > 0) {
    event->acceptProposedAction();

    ComboAction* ca = new ComboAction();

    Sequence* s = sequence();

    // if we're dropping into nothing, create a new sequences based on the clip being dragged
    if (s == nullptr) {
      s = self_created_sequence.get();
      olive::project_model.CreateSequence(ca, self_created_sequence, true, nullptr);
      self_created_sequence = nullptr;
    } else if (event->keyboardModifiers() & Qt::ControlModifier) {
      insert_clips(ca, s);
    } else {
      delete_area_under_ghosts(ca, s);
    }

    s->AddClipsFromGhosts(ca, ParentTimeline()->ghosts);

    ParentTimeline()->ghosts.clear();

    ParentTimeline()->importing = false;

    olive::undo_stack.push(ca);

    setFocus();

    update_ui(true);
  }
}

void TimelineView::mouseDoubleClickEvent(QMouseEvent *event) {
  if (sequence() != nullptr) {
    if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_EDIT) {
      Clip* clip = GetClipAtCursor();
      if (clip != nullptr) {
        if (!(event->modifiers() & Qt::ShiftModifier)) {
          sequence()->ClearSelections();
        }
        clip->track()->SelectClip(clip);
        update_ui(false);
      }
    } else if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER) {
      Clip* c = GetClipAtCursor();
      if (c != nullptr) {
        if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE) {
          Timeline::OpenSequence(c->media()->to_sequence());
        }
      }
    }
  }
}

bool TimelineView::current_tool_shows_cursor() {
  return (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_EDIT
          || olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_RAZOR
          || ParentTimeline()->creating);
}

Clip *TimelineView::GetClipAtCursor()
{
  if (ParentTimeline()->cursor_track == nullptr) {
    return nullptr;
  }

  return ParentTimeline()->cursor_track->GetClipFromPoint(ParentTimeline()->cursor_frame);
}

QVector<Track *> TimelineView::GetSplitTracksFromMouseCoords(bool also_split_links, long frame, int top, int bottom)
{
  // Convert top and bottom coords to global coordinates used by GetTracksInRectangle()
  int global_top = mapToGlobal(QPoint(0, top)).y();
  int global_bottom = mapToGlobal(QPoint(0, bottom)).y();

  // Get the current tracks in this mouse range
  QVector<Track*> split_tracks = ParentTimeline()->GetTracksInRectangle(global_top, global_bottom);

  // If we're also splitting links, loop through each track and search for clips that will be split at this point
  if (also_split_links) {

    // Cache array size because we'll be adding to it and don't want to cause an infinite loop
    int split_track_size = split_tracks.size();
    for (int i=0;i<split_track_size;i++) {

      Track* track = split_tracks.at(i);

      for (int j=0;j<track->ClipCount();j++) {
        Clip* c = track->GetClip(j).get();

        // Check if this clip is going to be split at this frame
        if (c->timeline_in() < frame && c->timeline_out() > frame) {

          // Loop through clip's links for more tracks to split
          for (int k=0;k<c->linked.size();k++) {
            Track* link_track = c->linked.at(k)->track();
            if (!split_tracks.contains(link_track)) {
              split_tracks.append(link_track);
            }
          }

          // Break because there will only be one clip active at this frame per track
          break;
        }
      }
    }
  }

  return split_tracks;
}

void TimelineView::mousePressEvent(QMouseEvent *event) {
  if (sequence() != nullptr) {

    int effective_tool = olive::timeline::current_tool;

    // some user actions will override which tool we'll be using
    if (event->button() == Qt::MiddleButton) {
      effective_tool = olive::timeline::TIMELINE_TOOL_HAND;
      ParentTimeline()->creating = false;
    } else if (event->button() == Qt::RightButton) {
      effective_tool = olive::timeline::TIMELINE_TOOL_MENU;
      ParentTimeline()->creating = false;
    }

    // ensure cursor_frame and cursor_track are up to date
    mouseMoveEvent(event);

    // store current cursor positions
    ParentTimeline()->drag_x_start = event->pos().x();
    ParentTimeline()->drag_y_start = event->pos().y();

    // store current frame/tracks as the values to start dragging from
    ParentTimeline()->drag_frame_start = ParentTimeline()->cursor_frame;
    ParentTimeline()->drag_track_start = ParentTimeline()->cursor_track;

    // get the clip the user is currently hovering over, priority to trim_target set from mouseMoveEvent
    Clip* hovered_clip = ParentTimeline()->trim_target == nullptr ?
          GetClipAtCursor()
        : ParentTimeline()->trim_target;

    bool shift = (event->modifiers() & Qt::ShiftModifier);
    bool alt = (event->modifiers() & Qt::AltModifier);

    // Normal behavior is to reset selections to zero when clicking, but if Shift is held, we add selections
    // to the existing selections. `selection_offset` is the index to change selections from (and we don't touch
    // any prior to that)
    if (shift) {
      ParentTimeline()->selection_cache = sequence()->Selections();
    } else {
      ParentTimeline()->selection_cache.clear();
    }

    // if the user is creating an object
    if (ParentTimeline()->creating) {
      Track::Type create_type = Track::kTypeVideo;
      switch (ParentTimeline()->creating_object) {
      case olive::timeline::ADD_OBJ_TITLE:
      case olive::timeline::ADD_OBJ_SOLID:
      case olive::timeline::ADD_OBJ_BARS:
        break;
      case olive::timeline::ADD_OBJ_TONE:
      case olive::timeline::ADD_OBJ_NOISE:
      case olive::timeline::ADD_OBJ_AUDIO:
        create_type = Track::kTypeAudio;
        break;
      }

      // if the track the user clicked is correct for the type of object we're adding

      if (track_list_->type() == create_type) {
        Ghost g;
        g.in = g.old_in = g.out = g.old_out = ParentTimeline()->drag_frame_start;

        g.track = ParentTimeline()->drag_track_start;
        if (g.track == nullptr) {
          g.track = track_list_->Last();
          ParentTimeline()->drag_track_start = track_list_->Last();
          g.track_movement = getTrackIndexFromScreenPoint(event->pos().y()) - g.track->Index();
        }

        g.trim_type = olive::timeline::TRIM_OUT;
        ParentTimeline()->ghosts.append(g);

        ParentTimeline()->moving_init = true;
        ParentTimeline()->moving_proc = true;
      }
    } else {

      // pass through tools to determine what action we'll be starting
      switch (effective_tool) {

      // many tools share pointer-esque behavior
      case olive::timeline::TIMELINE_TOOL_POINTER:
      case olive::timeline::TIMELINE_TOOL_RIPPLE:
      case olive::timeline::TIMELINE_TOOL_SLIP:
      case olive::timeline::TIMELINE_TOOL_ROLLING:
      case olive::timeline::TIMELINE_TOOL_SLIDE:
      case olive::timeline::TIMELINE_TOOL_MENU:
      {
        if (track_resizing && effective_tool != olive::timeline::TIMELINE_TOOL_MENU) {

          // if the cursor is currently hovering over a track, init track resizing
          ParentTimeline()->moving_init = true;

        } else {

          // check if we're currently hovering over a clip or not
          if (hovered_clip != nullptr) {

            if (hovered_clip->IsSelected()) {

              if (shift) {

                // if the user clicks a selected clip while holding shift, deselect the clip
                hovered_clip->track()->DeselectArea(hovered_clip->timeline_in(), hovered_clip->timeline_out());

                // if the user isn't holding alt, also deselect all of its links as well
                if (!alt) {
                  for (int i=0;i<hovered_clip->linked.size();i++) {
                    Clip* link = hovered_clip->linked.at(i);
                    link->track()->DeselectArea(link->timeline_in(), link->timeline_out());
                  }
                }

              } else if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER
                         && ParentTimeline()->transition_select != kTransitionNone) {

                // if the clip was selected by then the user clicked a transition, de-select the clip and its links
                // and select the transition only

                hovered_clip->track()->DeselectArea(hovered_clip->timeline_in(), hovered_clip->timeline_out());

                for (int i=0;i<hovered_clip->linked.size();i++) {
                  Clip* link = hovered_clip->linked.at(i);
                  link->track()->DeselectArea(link->timeline_in(), link->timeline_out());
                }

                long s_in, s_out;

                // select the transition only
                if (ParentTimeline()->transition_select == kTransitionOpening
                    && hovered_clip->opening_transition != nullptr) {
                  s_in = hovered_clip->timeline_in();

                  if (hovered_clip->opening_transition->secondary_clip != nullptr) {
                    s_in -= hovered_clip->opening_transition->get_true_length();
                  }

                  s_out = hovered_clip->timeline_in() + hovered_clip->opening_transition->get_true_length();

                } else if (ParentTimeline()->transition_select == kTransitionClosing
                           && hovered_clip->closing_transition != nullptr) {

                  s_in = hovered_clip->timeline_out() - hovered_clip->closing_transition->get_true_length();
                  s_out = hovered_clip->timeline_out();

                  if (hovered_clip->closing_transition->secondary_clip != nullptr) {
                    s_out += hovered_clip->closing_transition->get_true_length();
                  }
                }
                hovered_clip->track()->SelectArea(s_in, s_out);
              }

            } else {

              // if the clip is not already selected

              // if shift is NOT down, we change clear all current selections
              if (!shift) {
                sequence()->ClearSelections();
              }

              long s_in = hovered_clip->timeline_in();
              long s_out = hovered_clip->timeline_out();

              // if user is using the pointer tool, they may be trying to select a transition
              // check if the use is hovering over a transition
              if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER) {
                if (ParentTimeline()->transition_select == kTransitionOpening) {
                  // move the selection to only select the transitoin
                  s_out = hovered_clip->timeline_in() + hovered_clip->opening_transition->get_true_length();

                  // if the transition is a "shared" transition, adjust the selection to select both sides
                  if (hovered_clip->opening_transition->secondary_clip != nullptr) {
                    s_in -= hovered_clip->opening_transition->get_true_length();
                  }
                } else if (ParentTimeline()->transition_select == kTransitionClosing) {
                  // move the selection to only select the transitoin
                  s_in = hovered_clip->timeline_out() - hovered_clip->closing_transition->get_true_length();

                  // if the transition is a "shared" transition, adjust the selection to select both sides
                  if (hovered_clip->closing_transition->secondary_clip != nullptr) {
                    s_out += hovered_clip->closing_transition->get_true_length();
                  }
                }
              }

              // add the selection to the array
              hovered_clip->track()->SelectArea(s_in, s_out);

              // if the config is set to also seek with selections, do so now
              if (olive::config.select_also_seeks) {
                panel_sequence_viewer->seek(hovered_clip->timeline_in());
              }

              // if alt is not down, select links (provided we're not selecting transitions)
              if (!alt && ParentTimeline()->transition_select == kTransitionNone) {

                for (int i=0;i<hovered_clip->linked.size();i++) {

                  Clip* link = hovered_clip->linked.at(i);

                  // check if the clip is already selected
                  if (!link->IsSelected()) {
                    link->track()->SelectClip(link);
                  }

                }

              }
            }

            // authorize the starting of a move action if the mouse moves after this
            if (effective_tool != olive::timeline::TIMELINE_TOOL_MENU) {
              ParentTimeline()->moving_init = true;
            }

          } else {

            // if the user did not click a clip at all, we start a rectangle selection

            if (!shift) {
              sequence()->ClearSelections();
            }

            ParentTimeline()->rect_select_init = true;
          }

          // update everything
          update_ui(false);
        }
      }
        break;
      case olive::timeline::TIMELINE_TOOL_HAND:

        // initiate moving with the hand tool
        ParentTimeline()->hand_moving = true;

        break;
      case olive::timeline::TIMELINE_TOOL_EDIT:

        // if the config is set to seek with the edit tool, do so now
        if (olive::config.edit_tool_also_seeks) {
          panel_sequence_viewer->seek(ParentTimeline()->drag_frame_start);
        }

        // initiate selecting
        ParentTimeline()->selecting = true;

        break;
      case olive::timeline::TIMELINE_TOOL_RAZOR:
      {

        // initiate razor tool
        ParentTimeline()->splitting = true;

        ParentTimeline()->split_tracks = GetSplitTracksFromMouseCoords(!alt,
                                                                       ParentTimeline()->drag_frame_start,
                                                                       event->pos().y(),
                                                                       event->pos().y());

        update_ui(false);
      }
        break;
      case olive::timeline::TIMELINE_TOOL_TRANSITION:
      {

        // if there is a clip to run the transition tool on, initiate the transition tool
        if (ParentTimeline()->transition_tool_open_clip != nullptr
            || ParentTimeline()->transition_tool_close_clip != nullptr) {
          ParentTimeline()->transition_tool_init = true;
        }

      }
        break;
      }
    }
  }
}

void make_room_for_transition(ComboAction* ca,
                              Clip* c,
                              int type,
                              long transition_start,
                              long transition_end,
                              bool delete_old_transitions,
                              long timeline_in = -1,
                              long timeline_out = -1) {
  // it's possible to specify other in/out points for the clip, but default behavior is to use the ones existing
  if (timeline_in < 0) {
    timeline_in = c->timeline_in();
  }
  if (timeline_out < 0) {
    timeline_out = c->timeline_out();
  }

  // make room for transition
  if (type == kTransitionOpening) {
    if (delete_old_transitions && c->opening_transition != nullptr) {
      ca->append(new DeleteTransitionCommand(c->opening_transition));
    }
    if (c->closing_transition != nullptr) {
      if (transition_end >= c->timeline_out()) {
        ca->append(new DeleteTransitionCommand(c->closing_transition));
      } else if (transition_end > c->timeline_out() - c->closing_transition->get_true_length()) {
        ca->append(new ModifyTransitionCommand(c->closing_transition, c->timeline_out() - transition_end));
      }
    }
  } else {
    if (delete_old_transitions && c->closing_transition != nullptr) {
      ca->append(new DeleteTransitionCommand(c->closing_transition));
    }
    if (c->opening_transition != nullptr) {
      if (transition_start <= c->timeline_in()) {
        ca->append(new DeleteTransitionCommand(c->opening_transition));
      } else if (transition_start < c->timeline_in() + c->opening_transition->get_true_length()) {
        ca->append(new ModifyTransitionCommand(c->opening_transition, transition_start - c->timeline_in()));
      }
    }
  }
}

void TimelineView::VerifyTransitionsAfterCreating(ComboAction* ca, Clip* open, Clip* close, long transition_start, long transition_end) {
  // in case the user made the transition larger than the clips, we're going to delete everything under
  // the transition ghost and extend the clips to the transition's coordinates as necessary

  if (open == nullptr && close == nullptr) {
    qWarning() << "VerifyTransitionsAfterCreating() called with two null clips";
    return;
  }

  // determine whether this is a "shared" transition between to clips or not
  bool shared_transition = (open != nullptr && close != nullptr);

  Track* track = nullptr;

  // first we set the clips to "undeletable" so they aren't affected by delete_areas_and_relink()
  if (open != nullptr) {
    open->undeletable = true;
    track = open->track();
  }
  if (close != nullptr) {
    close->undeletable = true;
    track = close->track();
  }

  // set the area to delete to the transition's coordinates and clear it
  QVector<Selection> areas;
  areas.append(Selection(transition_start, transition_end, track));
  sequence()->DeleteAreas(ca, areas, false);

  // set the clips back to undeletable now that we're done
  if (open != nullptr) {
    open->undeletable = false;
  }
  if (close != nullptr) {
    close->undeletable = false;
  }

  // loop through both kinds of transition
  for (int t=kTransitionOpening;t<=kTransitionClosing;t++) {

    Clip* clip_ref = (t == kTransitionOpening) ? open : close;

    // if we have an opening transition:
    if (clip_ref != nullptr) {

      // make_room_for_transition will adjust the opposite transition to make space for this one,
      // for example if the user makes an opening transition that overlaps the closing transition, it'll resize
      // or even delete the closing transition if necessary (and vice versa)

      make_room_for_transition(ca, clip_ref, t, transition_start, transition_end, true);

      // check if the transition coordinates require the clip to be resized
      if (transition_start < clip_ref->timeline_in() || transition_end > clip_ref->timeline_out()) {

        long new_in, new_out;

        if (t == kTransitionOpening) {

          // if the transition is shared, it doesn't matter if the transition extend beyond the in point since
          // that'll be "absorbed" by the other clip
          new_in = (shared_transition) ? open->timeline_in() : qMin(transition_start, open->timeline_in());

          new_out = qMax(transition_end, open->timeline_out());

        } else {

          new_in = qMin(transition_start, close->timeline_in());

          // if the transition is shared, it doesn't matter if the transition extend beyond the out point since
          // that'll be "absorbed" by the other clip
          new_out = (shared_transition) ? close->timeline_out() : qMax(transition_end, close->timeline_out());

        }



        clip_ref->Move(ca,
                       new_in,
                       new_out,
                       clip_ref->clip_in() - (clip_ref->timeline_in() - new_in),
                       clip_ref->track());
      }
    }
  }
}

int TimelineView::GetTotalAreaHeight()
{
  // start by adding a track height worth of padding
  int panel_height = olive::timeline::kTrackDefaultHeight;

  for (int i=0;i<track_list_->TrackCount();i++) {
    panel_height += track_list_->TrackAt(i)->height();
  }

  return panel_height;
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event) {
  QToolTip::hideText();
  if (sequence() != nullptr) {
    bool alt = (event->modifiers() & Qt::AltModifier);
    bool shift = (event->modifiers() & Qt::ShiftModifier);
    bool ctrl = (event->modifiers() & Qt::ControlModifier);

    if (event->button() == Qt::LeftButton) {
      ComboAction* ca = new ComboAction();
      bool push_undo = false;

      if (ParentTimeline()->creating) {
        if (ParentTimeline()->ghosts.size() > 0) {
          const Ghost& g = ParentTimeline()->ghosts.at(0);

          if (ParentTimeline()->creating_object == olive::timeline::ADD_OBJ_AUDIO) {
            olive::MainWindow->statusBar()->clearMessage();
            panel_sequence_viewer->cue_recording(qMin(g.in, g.out), qMax(g.in, g.out), g.track);
            ParentTimeline()->creating = false;
          } else if (g.in != g.out) {
            ClipPtr c = std::make_shared<Clip>(g.track->Sibling(g.track_movement));
            c->set_media(nullptr, 0);
            c->set_timeline_in(qMin(g.in, g.out));
            c->set_timeline_out(qMax(g.in, g.out));
            c->set_clip_in(0);
            c->set_color(192, 192, 64);

            if (ctrl) {
              insert_clips(ca, sequence());
            } else {
              sequence()->DeleteAreas(ca, {c->ToSelection()}, false);
            }

            QVector<ClipPtr> add;
            add.append(c);
            ca->append(new AddClipCommand(add));

            if (c->type() == Track::kTypeVideo && olive::config.add_default_effects_to_clips) {
              // default video effects (before custom effects)
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_TRANSFORM, EFFECT_TYPE_EFFECT)));
            }

            switch (ParentTimeline()->creating_object) {
            case olive::timeline::ADD_OBJ_TITLE:
              c->set_name(tr("Title"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_RICHTEXT, EFFECT_TYPE_EFFECT)));
              break;
            case olive::timeline::ADD_OBJ_SOLID:
              c->set_name(tr("Solid Color"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_SOLID, EFFECT_TYPE_EFFECT)));
              break;
            case olive::timeline::ADD_OBJ_BARS:
            {
              c->set_name(tr("Bars"));
              EffectPtr e = Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_SOLID, EFFECT_TYPE_EFFECT));

              // Auto-select bars
              SolidEffect* solid_effect = static_cast<SolidEffect*>(e.get());
              solid_effect->SetType(SolidEffect::SOLID_TYPE_BARS);

              c->effects.append(e);
            }
              break;
            case olive::timeline::ADD_OBJ_TONE:
              c->set_name(tr("Tone"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_TONE, EFFECT_TYPE_EFFECT)));
              break;
            case olive::timeline::ADD_OBJ_NOISE:
              c->set_name(tr("Noise"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_NOISE, EFFECT_TYPE_EFFECT)));
              break;
            default:
              break;
            }

            if (c->type() == Track::kTypeAudio && olive::config.add_default_effects_to_clips) {
              // default audio effects (after custom effects)
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_VOLUME, EFFECT_TYPE_EFFECT)));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_PAN, EFFECT_TYPE_EFFECT)));
            }

            push_undo = true;

            if (!shift) {
              ParentTimeline()->creating = false;
            }
          }
        }
      } else if (ParentTimeline()->moving_proc) {

        // see if any clips actually moved, otherwise we don't need to do any processing
        // (perhaps this could be moved further up to cover more actions?)

        bool process_moving = false;

        for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
          const Ghost& g = ParentTimeline()->ghosts.at(i);
          if (g.in != g.old_in
              || g.out != g.old_out
              || g.clip_in != g.old_clip_in
              || g.track_movement != 0) {
            process_moving = true;
            break;
          }
        }

        if (process_moving) {

          const Ghost& first_ghost = ParentTimeline()->ghosts.at(0);

          // start a ripple movement
          if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_RIPPLE) {

            // ripple_length becomes the length/number of frames we trimmed
            // ripple_point is the "axis" around which we move all the clips, any clips after it get moved
            long ripple_length;
            long ripple_point = LONG_MAX;

            if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {

              // it's assumed that all the ghosts rippled by the same length, so we just take the difference of the
              // first ghost here
              ripple_length = first_ghost.old_in - first_ghost.in;

              // for in trimming movements we also move the selections forward (unnecessary for out trimming since
              // the selected clips more or less stay in the same place)
              /*
              for (int i=0;i<sequence()->selections.size();i++) {
                sequence()->selections[i].in += ripple_length;
                sequence()->selections[i].out += ripple_length;
              }
              */
            } else {

              // use the out points for length if the user trimmed the out point
              ripple_length = first_ghost.old_out - ParentTimeline()->ghosts.at(0).out;

            }

            // build a list of "ignore clips" that won't get affected by ripple_clips() below
            QVector<Clip*> ignore_clips;
            for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
              const Ghost& g = ParentTimeline()->ghosts.at(i);

              // for the same reason that we pushed selections forward above, for in trimming,
              // we push the ghosts forward here
              if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {
                ignore_clips.append(g.clip);
                ParentTimeline()->ghosts[i].in += ripple_length;
                ParentTimeline()->ghosts[i].out += ripple_length;
              }

              // find the earliest ripple point
              long comp_point = (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) ? g.old_in : g.old_out;
              ripple_point = qMin(ripple_point, comp_point);
            }

            // if this was out trimming, flip the direction of the ripple
            if (ParentTimeline()->trim_type == olive::timeline::TRIM_OUT) ripple_length = -ripple_length;

            // finally, ripple everything
            sequence()->Ripple(ca, ripple_point, ripple_length, ignore_clips);
          }

          if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER
              && (event->modifiers() & Qt::AltModifier)
              && ParentTimeline()->trim_target == nullptr) {

            // if the user was holding alt (and not trimming), we duplicate clips rather than move them
            QVector<Clip*> old_clips;
            QVector<ClipPtr> new_clips;
            QVector<Selection> delete_areas;
            for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
              const Ghost& g = ParentTimeline()->ghosts.at(i);
              if (g.old_in != g.in || g.old_out != g.out || g.track_movement != 0 || g.clip_in != g.old_clip_in) {

                // create copy of clip
                ClipPtr c = g.clip->copy(g.track->Sibling(g.track_movement));

                c->set_timeline_in(g.in);
                c->set_timeline_out(g.out);

                delete_areas.append(g.ToSelection());

                old_clips.append(g.clip);
                new_clips.append(c);

              }
            }

            if (new_clips.size() > 0) {

              // delete anything under the new clips
              sequence()->DeleteAreas(ca, delete_areas, false);

              // relink duplicated clips
              olive::timeline::RelinkClips(old_clips, new_clips);

              // add them
              ca->append(new AddClipCommand(new_clips));

            }

          } else {

            // if we're not holding alt, this will just be a move

            // if the user is holding ctrl, perform an insert rather than an overwrite
            if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER && ctrl) {

              insert_clips(ca, sequence());

            } else if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER || olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_SLIDE) {

              // if the user is not holding ctrl, we start standard clip movement

              // delete everything under the new clips
              QVector<Selection> delete_areas;
              for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
                // step 1 - set clips that are moving to "undeletable" (to avoid step 2 deleting any part of them)
                const Ghost& g = ParentTimeline()->ghosts.at(i);

                // set clip to undeletable so it's unaffected by delete_areas_and_relink() below
                g.clip->undeletable = true;

                // if the user was moving a transition make sure they're undeletable too
                if (g.transition != nullptr) {
                  g.transition->parent_clip->undeletable = true;
                  if (g.transition->secondary_clip != nullptr) {
                    g.transition->secondary_clip->undeletable = true;
                  }
                }

                // set area to delete
                delete_areas.append(g.ToSelection());
              }

              sequence()->DeleteAreas(ca, delete_areas, false);

              // clean up, i.e. make everything not undeletable again
              for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
                const Ghost& g = ParentTimeline()->ghosts.at(i);
                g.clip->undeletable = false;

                if (g.transition != nullptr) {
                  g.transition->parent_clip->undeletable = false;
                  if (g.transition->secondary_clip != nullptr) {
                    g.transition->secondary_clip->undeletable = false;
                  }
                }
              }
            }

            // finally, perform actual movement of clips
            for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
              Ghost& g = ParentTimeline()->ghosts[i];

              Clip* c = g.clip;

              if (g.transition == nullptr) {

                // if this was a clip rather than a transition

                c->Move(ca,
                        (g.in - g.old_in),
                        (g.out - g.old_out),
                        (g.clip_in - g.old_clip_in),
                        g.track->Sibling(g.track_movement),
                        false,
                        true);

              } else {

                // if the user was moving a transition

                bool is_opening_transition = (g.transition == c->opening_transition);
                long new_transition_length = g.out - g.in;
                if (g.transition->secondary_clip != nullptr) new_transition_length >>= 1;
                ca->append(
                      new ModifyTransitionCommand(is_opening_transition ? c->opening_transition : c->closing_transition,
                                                  new_transition_length)
                      );

                long clip_length = c->length();

                if (g.transition->secondary_clip != nullptr) {

                  // if this is a shared transition
                  if (g.in != g.old_in && g.trim_type == olive::timeline::TRIM_NONE) {
                    long movement = g.in - g.old_in;

                    // check if the transition is going to extend the out point (opening clip)
                    long timeline_out_movement = 0;
                    if (g.out > g.transition->parent_clip->timeline_out()) {
                      timeline_out_movement = g.out - g.transition->parent_clip->timeline_out();
                    }

                    // check if the transition is going to extend the in point (closing clip)
                    long timeline_in_movement = 0;
                    if (g.in < g.transition->secondary_clip->timeline_in()) {
                      timeline_in_movement = g.in - g.transition->secondary_clip->timeline_in();
                    }

                    g.transition->parent_clip->Move(ca, movement, timeline_out_movement, movement, g.transition->parent_clip->track(), false, true);
                    g.transition->secondary_clip->Move(ca, timeline_in_movement, movement, timeline_in_movement, g.transition->secondary_clip->track(), false, true);

                    make_room_for_transition(ca, g.transition->parent_clip, kTransitionOpening, g.in, g.out, false);
                    make_room_for_transition(ca, g.transition->secondary_clip, kTransitionClosing, g.in, g.out, false);

                  }

                } else if (is_opening_transition) {

                  if (g.in != g.old_in) {
                    // if transition is going to make the clip bigger, make the clip bigger

                    // check if the transition is going to extend the out point
                    long timeline_out_movement = 0;
                    if (g.out > g.transition->parent_clip->timeline_out()) {
                      timeline_out_movement = g.out - g.transition->parent_clip->timeline_out();
                    }

                    c->Move(ca,
                            (g.in - g.old_in),
                            timeline_out_movement,
                            (g.clip_in - g.old_clip_in),
                            g.track,
                            false,
                            true);
                    clip_length -= (g.in - g.old_in);
                  }

                  make_room_for_transition(ca, c, kTransitionOpening, g.in, g.out, false);

                } else {

                  if (g.out != g.old_out) {

                    // check if the transition is going to extend the in point
                    long timeline_in_movement = 0;
                    if (g.in < g.transition->parent_clip->timeline_in()) {
                      timeline_in_movement = g.in - g.transition->parent_clip->timeline_in();
                    }

                    // if transition is going to make the clip bigger, make the clip bigger
                    c->Move(ca, timeline_in_movement, (g.out - g.old_out), timeline_in_movement, c->track(), false, true);
                    clip_length += (g.out - g.old_out);
                  }

                  make_room_for_transition(ca, c, kTransitionClosing, g.in, g.out, false);

                }
              }
            }

            // time to verify the transitions of moved clips
            for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
              const Ghost& g = ParentTimeline()->ghosts.at(i);

              // only applies to moving clips, transitions are verified above instead
              if (g.transition == nullptr) {
                Clip* c = g.clip;

                long new_clip_length = g.out - g.in;

                // using a for loop between constants to repeat the same steps for the opening and closing transitions
                for (int t=kTransitionOpening;t<=kTransitionClosing;t++) {

                  TransitionPtr transition = (t == kTransitionOpening) ? c->opening_transition : c->closing_transition;

                  // check the whether the clip has a transition here
                  if (transition != nullptr) {

                    // if the new clip size exceeds the opening transition's length, resize the transition
                    if (new_clip_length < transition->get_true_length()) {
                      ca->append(new ModifyTransitionCommand(transition, new_clip_length));
                    }

                    // check if the transition is a shared transition (it'll never have a secondary clip if it isn't)
                    if (transition->secondary_clip != nullptr) {

                      // check if the transition's "edge" is going to move
                      if ((t == kTransitionOpening && g.in != g.old_in)
                          || (t == kTransitionClosing && g.out != g.old_out)
                          || (g.track_movement != 0)) {

                        // if we're here, this clip shares its opening transition as the closing transition of another
                        // clip (or vice versa), and the in point is moving, so we may have to account for this

                        // the other clip sharing this transition may be moving as well, meaning we don't have to do
                        // anything

                        bool split = true;

                        // loop through ghosts to find out

                        // for a shared transition, the secondary_clip will always be the closing transition side and
                        // the parent_clip will always be the opening transition side
                        Clip* search_clip = (t == kTransitionOpening)
                            ? transition->secondary_clip : transition->parent_clip;

                        for (int j=0;j<ParentTimeline()->ghosts.size();j++) {
                          const Ghost& other_clip_ghost = ParentTimeline()->ghosts.at(j);

                          if (other_clip_ghost.clip == search_clip) {

                            // we found the other clip in the current ghosts/selections

                            // see if it's destination edge will be equal to this ghost's edge (in which case the
                            // transition doesn't need to change)
                            //
                            // also only do this if j is less than i, because it only needs to happen once and chances are
                            // the other clip already

                            bool edges_still_touch = (other_clip_ghost.track_movement == g.track_movement);

                            if (edges_still_touch) {
                              if (t == kTransitionOpening) {
                                edges_still_touch = (other_clip_ghost.out == g.in);
                              } else {
                                edges_still_touch = (other_clip_ghost.in == g.out);
                              }
                            }

                            if (edges_still_touch || j < i) {
                              split = false;
                            }

                            break;
                          }
                        }

                        if (split) {
                          // separate shared transition into one transition for each clip

                          if (t == kTransitionOpening) {

                            // set transition to single-clip mode
                            ca->append(new SetPointer(reinterpret_cast<void**>(&transition->secondary_clip),
                                                      nullptr));

                            // create duplicate transition for other clip
                            ca->append(new AddTransitionCommand(nullptr,
                                                                transition->secondary_clip,
                                                                transition,
                                                                nullptr,
                                                                0));

                          } else {

                            // set transition to single-clip mode
                            ca->append(new SetPointer(reinterpret_cast<void**>(&transition->secondary_clip),
                                                      nullptr));

                            // that transition will now attach to the other clip, so we duplicate it for this one

                            // create duplicate transition for this clip
                            ca->append(new AddTransitionCommand(nullptr,
                                                                transition->secondary_clip,
                                                                transition,
                                                                nullptr,
                                                                0));

                          }
                        }

                      }
                    }
                  }
                }
              }
            }
          }

          // move selections to match new ghosts
          QVector<Selection> new_selections;
          for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
            new_selections.append(ParentTimeline()->ghosts.at(i).ToSelection());
          }
          ca->append(new SetSelectionsCommand(sequence(), sequence()->Selections(), new_selections));

          push_undo = true;

        }
      } else if (ParentTimeline()->selecting || ParentTimeline()->rect_select_proc) {
      } else if (ParentTimeline()->transition_tool_proc) {
        const Ghost& g = ParentTimeline()->ghosts.at(0);

        // if the transition is greater than 0 length (if it is 0, we make nothing)
        if (g.in != g.out) {

          // get transition coordinates on the timeline
          long transition_start = qMin(g.in, g.out);
          long transition_end = qMax(g.in, g.out);

          // get clip references from tool's cached data
          Clip* open = ParentTimeline()->transition_tool_open_clip;

          Clip* close = ParentTimeline()->transition_tool_close_clip;



          // if it's shared, the transition length is halved (one half for each clip will result in the full length)
          long transition_length = transition_end - transition_start;
          if (open != nullptr && close != nullptr) {
            transition_length /= 2;
          }

          VerifyTransitionsAfterCreating(ca, open, close, transition_start, transition_end);

          // finally, add the transition to these clips
          ca->append(new AddTransitionCommand(open,
                                              close,
                                              nullptr,
                                              ParentTimeline()->transition_tool_meta,
                                              transition_length));

          push_undo = true;
        }
      } else if (ParentTimeline()->splitting) {

        QVector<Track*> split_tracks = GetSplitTracksFromMouseCoords(false,
                                                                     ParentTimeline()->drag_frame_start,
                                                                     ParentTimeline()->drag_y_start,
                                                                     event->pos().y());

        for (int i=0;i<split_tracks.size();i++) {
          Clip* split_index = split_tracks.at(i)->GetClipFromPoint(ParentTimeline()->drag_frame_start);
          if (split_index != nullptr
              && sequence()->SplitClipAtPositions(ca, split_index, {ParentTimeline()->drag_frame_start}, !alt)) {
            push_undo = true;
          }
        }
      }

      // remove duplicate selections
      sequence()->TidySelections();

      if (push_undo) {
        olive::undo_stack.push(ca);
      } else {
        delete ca;
      }

      // destroy all ghosts
      ParentTimeline()->ghosts.clear();

      // clear split tracks
      ParentTimeline()->split_tracks.clear();

      ParentTimeline()->selecting = false;
      ParentTimeline()->moving_proc = false;
      ParentTimeline()->moving_init = false;
      ParentTimeline()->splitting = false;
      olive::timeline::snapped = false;
      ParentTimeline()->rect_select_init = false;
      ParentTimeline()->rect_select_proc = false;
      ParentTimeline()->transition_tool_init = false;
      ParentTimeline()->transition_tool_proc = false;
      pre_clips.clear();
      post_clips.clear();

      update_ui(true);
    }
    ParentTimeline()->hand_moving = false;
  }
}

void TimelineView::init_ghosts() {
  for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
    Ghost& g = ParentTimeline()->ghosts[i];
    Clip* c = g.clip;

    g.track = c->track();
    g.clip_in = g.old_clip_in = c->clip_in();

    if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_SLIP) {
      g.clip_in = g.old_clip_in = c->clip_in(true);
      g.in = g.old_in = c->timeline_in(true);
      g.out = g.old_out = c->timeline_out(true);
      g.ghost_length = g.old_out - g.old_in;
    } else if (g.transition == nullptr) {
      // this ghost is for a clip
      g.in = g.old_in = c->timeline_in();
      g.out = g.old_out = c->timeline_out();
      g.ghost_length = g.old_out - g.old_in;
    } else if (g.transition == c->opening_transition) {
      g.in = g.old_in = c->timeline_in(true);
      g.ghost_length = c->opening_transition->get_length();
      g.out = g.old_out = g.in + g.ghost_length;
    } else if (g.transition == c->closing_transition) {
      g.out = g.old_out = c->timeline_out(true);
      g.ghost_length = c->closing_transition->get_length();
      g.in = g.old_in = g.out - g.ghost_length;
      g.clip_in = g.old_clip_in = c->clip_in() + c->length() - c->closing_transition->get_true_length();
    }

    // used for trim ops
    g.media_length = c->media_length();
  }
  /*
  for (int i=0;i<sequence()->selections.size();i++) {
    Selection& s = sequence()->selections[i];
    s.old_in = s.in;
    s.old_out = s.out;
    s.old_track = s.track;
  }
  */
}

void validate_transitions(Clip* c, int transition_type, long& frame_diff) {
  long validator;

  if (transition_type == kTransitionOpening) {
    // prevent from going below 0 on the timeline
    validator = c->timeline_in() + frame_diff;
    if (validator < 0) frame_diff -= validator;

    // prevent from going below 0 for the media
    validator = c->clip_in() + frame_diff;
    if (validator < 0) frame_diff -= validator;

    // prevent transition from exceeding media length
    validator -= c->media_length();
    if (validator > 0) frame_diff -= validator;
  } else {
    // prevent from going below 0 on the timeline
    validator = c->timeline_out() + frame_diff;
    if (validator < 0) frame_diff -= validator;

    // prevent from going below 0 for the media
    validator = c->clip_in() + c->length() + frame_diff;
    if (validator < 0) frame_diff -= validator;

    // prevent transition from exceeding media length
    validator -= c->media_length();
    if (validator > 0) frame_diff -= validator;
  }
}

void TimelineView::update_ghosts(const QPoint& mouse_pos, bool lock_frame) {
  int effective_tool = olive::timeline::current_tool;
  if (ParentTimeline()->importing || ParentTimeline()->creating) effective_tool = olive::timeline::TIMELINE_TOOL_POINTER;

  long frame_diff = (lock_frame) ? 0 : ParentTimeline()->getTimelineFrameFromScreenPoint(mouse_pos.x()) - ParentTimeline()->drag_frame_start;
  long validator;
  long earliest_in_point = LONG_MAX;
  int track_diff = getTrackIndexFromScreenPoint(mouse_pos.y()) - ParentTimeline()->drag_track_start->Index();

  // first try to snap
  long fm;

  if (effective_tool != olive::timeline::TIMELINE_TOOL_SLIP) {
    // slipping doesn't move the clips so we don't bother snapping for it
    for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
      const Ghost& g = ParentTimeline()->ghosts.at(i);

      // snap ghost's in point
      if ((olive::timeline::current_tool != olive::timeline::TIMELINE_TOOL_TRANSITION && ParentTimeline()->trim_target == nullptr)
          || g.trim_type == olive::timeline::TRIM_IN
          || ParentTimeline()->transition_tool_open_clip != nullptr) {
        fm = g.old_in + frame_diff;
        if (sequence()->SnapPoint(&fm, ParentTimeline()->zoom, true, true, true)) {
          frame_diff = fm - g.old_in;
          break;
        }
      }

      // snap ghost's out point
      if ((olive::timeline::current_tool != olive::timeline::TIMELINE_TOOL_TRANSITION && ParentTimeline()->trim_target == nullptr)
          || g.trim_type == olive::timeline::TRIM_OUT
          || ParentTimeline()->transition_tool_close_clip != nullptr) {
        fm = g.old_out + frame_diff;
        if (sequence()->SnapPoint(&fm, ParentTimeline()->zoom, true, true, true)) {
          frame_diff = fm - g.old_out;
          break;
        }
      }

      // if the ghost is attached to a clip, snap its markers too
      if (ParentTimeline()->trim_target == nullptr
          && g.clip != nullptr
          && olive::timeline::current_tool != olive::timeline::TIMELINE_TOOL_TRANSITION) {
        Clip* c = g.clip;
        for (int j=0;j<c->get_markers().size();j++) {
          long marker_real_time = c->get_markers().at(j).frame + c->timeline_in() - c->clip_in();
          fm = marker_real_time + frame_diff;
          if (sequence()->SnapPoint(&fm, ParentTimeline()->zoom, true, true, true)) {
            frame_diff = fm - marker_real_time;
            break;
          }
        }
      }
    }
  }

  bool clips_are_movable = (effective_tool == olive::timeline::TIMELINE_TOOL_POINTER || effective_tool == olive::timeline::TIMELINE_TOOL_SLIDE);

  // validate ghosts
  long temp_frame_diff = frame_diff; // cache to see if we change it (thus cancelling any snap)
  for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
    const Ghost& g = ParentTimeline()->ghosts.at(i);
    Clip* c = nullptr;
    if (g.clip != nullptr) {
      c = g.clip;
    }

    const FootageStream* ms = nullptr;
    if (g.clip != nullptr && c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
      ms = c->media_stream();
    }

    // validate ghosts for trimming
    if (ParentTimeline()->creating) {
      // i feel like we might need something here but we haven't so far?
    } else if (effective_tool == olive::timeline::TIMELINE_TOOL_SLIP) {
      if ((c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE)
          || (ms != nullptr && !ms->infinite_length)) {
        // prevent slip moving a clip below 0 clip_in
        validator = g.old_clip_in - frame_diff;
        if (validator < 0) frame_diff += validator;

        // prevent slip moving clip beyond media length
        validator += g.ghost_length;
        if (validator > g.media_length) frame_diff += validator - g.media_length;
      }
    } else if (g.trim_type != olive::timeline::TRIM_NONE) {
      if (g.trim_type == olive::timeline::TRIM_IN) {
        // prevent clip/transition length from being less than 1 frame long
        validator = g.ghost_length - frame_diff;
        if (validator < 1) frame_diff -= (1 - validator);

        // prevent timeline in from going below 0
        if (effective_tool != olive::timeline::TIMELINE_TOOL_RIPPLE) {
          validator = g.old_in + frame_diff;
          if (validator < 0) frame_diff -= validator;
        }

        // prevent clip_in from going below 0
        if ((c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE)
            || (ms != nullptr && !ms->infinite_length)) {
          validator = g.old_clip_in + frame_diff;
          if (validator < 0) frame_diff -= validator;
        }
      } else {
        // prevent clip length from being less than 1 frame long
        validator = g.ghost_length + frame_diff;
        if (validator < 1) frame_diff += (1 - validator);

        // prevent clip length exceeding media length
        if ((c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE)
            || (ms != nullptr && !ms->infinite_length)) {
          validator = g.old_clip_in + g.ghost_length + frame_diff;
          if (validator > g.media_length) frame_diff -= validator - g.media_length;
        }
      }

      // prevent dual transition from going below 0 on the primary or media length on the secondary
      if (g.transition != nullptr && g.transition->secondary_clip != nullptr) {
        Clip* otc = g.transition->parent_clip;
        Clip* ctc = g.transition->secondary_clip;

        if (g.trim_type == olive::timeline::TRIM_IN) {
          frame_diff -= g.transition->get_true_length();
        } else {
          frame_diff += g.transition->get_true_length();
        }

        validate_transitions(otc, kTransitionOpening, frame_diff);
        validate_transitions(ctc, kTransitionClosing, frame_diff);

        frame_diff = -frame_diff;
        validate_transitions(otc, kTransitionOpening, frame_diff);
        validate_transitions(ctc, kTransitionClosing, frame_diff);
        frame_diff = -frame_diff;

        if (g.trim_type == olive::timeline::TRIM_IN) {
          frame_diff += g.transition->get_true_length();
        } else {
          frame_diff -= g.transition->get_true_length();
        }
      }

      // ripple ops
      if (effective_tool == olive::timeline::TIMELINE_TOOL_RIPPLE) {
        for (int j=0;j<post_clips.size();j++) {
          Clip* post = post_clips.at(j);

          // prevent any rippled clip from going below 0
          if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {
            validator = post->timeline_in() - frame_diff;
            if (validator < 0) frame_diff += validator;
          }

          // prevent any post-clips colliding with pre-clips
          for (int k=0;k<pre_clips.size();k++) {
            Clip* pre = pre_clips.at(k);
            if (pre != post && pre->track() == post->track()) {
              if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {
                validator = post->timeline_in() - frame_diff - pre->timeline_out();
                if (validator < 0) frame_diff += validator;
              } else {
                validator = post->timeline_in() + frame_diff - pre->timeline_out();
                if (validator < 0) frame_diff -= validator;
              }
            }
          }
        }
      }
    } else if (clips_are_movable) { // validate ghosts for moving
      // prevent clips from moving below 0 on the timeline
      validator = g.old_in + frame_diff;
      if (validator < 0) frame_diff -= validator;

      if (g.transition != nullptr) {
        if (g.transition->secondary_clip != nullptr) {
          // prevent dual transitions from going below 0 on the primary or above media length on the secondary

          validator = g.transition->parent_clip->clip_in(true) + frame_diff;
          if (validator < 0) frame_diff -= validator;

          validator = g.transition->secondary_clip->timeline_out(true) - g.transition->secondary_clip->timeline_in(true) - g.transition->get_length() + g.transition->secondary_clip->clip_in(true) + frame_diff;
          if (validator < 0) frame_diff -= validator;

          validator = g.transition->parent_clip->clip_in() + frame_diff - g.transition->parent_clip->media_length() + g.transition->get_true_length();
          if (validator > 0) frame_diff -= validator;

          validator = g.transition->secondary_clip->timeline_out(true) - g.transition->secondary_clip->timeline_in(true) + g.transition->secondary_clip->clip_in(true) + frame_diff - g.transition->secondary_clip->media_length();
          if (validator > 0) frame_diff -= validator;
        } else {
          // prevent clip_in from going below 0
          if ((c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE)
              || (ms != nullptr && !ms->infinite_length)) {
            validator = g.old_clip_in + frame_diff;
            if (validator < 0) frame_diff -= validator;
          }

          // prevent clip length exceeding media length
          if ((c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE)
              || (ms != nullptr && !ms->infinite_length)) {
            validator = g.old_clip_in + g.ghost_length + frame_diff;
            if (validator > g.media_length) frame_diff -= validator - g.media_length;
          }
        }
      }

      // Prevent any clips from going below the "zeroeth" track

      if (ParentTimeline()->importing || g.track->type() == track_list_->type()) {

        int track_validator = g.track->Index() + track_diff;
        if (track_validator < 0) {
          track_diff -= track_validator;
        }

      }

    } else if (effective_tool == olive::timeline::TIMELINE_TOOL_TRANSITION) {
      if (ParentTimeline()->transition_tool_open_clip == nullptr
          || ParentTimeline()->transition_tool_close_clip == nullptr) {
        validate_transitions(c, g.media_stream, frame_diff);
      } else {
        // open transition clip
        Clip* otc = ParentTimeline()->transition_tool_open_clip;

        // close transition clip
        Clip* ctc = ParentTimeline()->transition_tool_close_clip;

        if (g.media_stream == kTransitionClosing) {
          // swap
          Clip* temp = otc;
          otc = ctc;
          ctc = temp;
        }

        // always gets a positive frame_diff
        validate_transitions(otc, kTransitionOpening, frame_diff);
        validate_transitions(ctc, kTransitionClosing, frame_diff);

        // always gets a negative frame_diff
        frame_diff = -frame_diff;
        validate_transitions(otc, kTransitionOpening, frame_diff);
        validate_transitions(ctc, kTransitionClosing, frame_diff);
        frame_diff = -frame_diff;
      }
    }
  }

  // if the above validation changed the frame movement, it's unlikely we're still snapped
  if (temp_frame_diff != frame_diff) {
    olive::timeline::snapped = false;
  }

  // apply changes to ghosts
  for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
    Ghost& g = ParentTimeline()->ghosts[i];

    if (effective_tool == olive::timeline::TIMELINE_TOOL_SLIP) {
      g.clip_in = g.old_clip_in - frame_diff;
    } else if (g.trim_type != olive::timeline::TRIM_NONE) {
      long ghost_diff = frame_diff;

      // prevent trimming clips from overlapping each other
      for (int j=0;j<ParentTimeline()->ghosts.size();j++) {
        const Ghost& comp = ParentTimeline()->ghosts.at(j);
        if (i != j && g.track == comp.track) {
          long validator;
          if (g.trim_type == olive::timeline::TRIM_IN && comp.out < g.out) {
            validator = (g.old_in + ghost_diff) - comp.out;
            if (validator < 0) ghost_diff -= validator;
          } else if (comp.in > g.in) {
            validator = (g.old_out + ghost_diff) - comp.in;
            if (validator > 0) ghost_diff -= validator;
          }
        }
      }

      // apply changes
      if (g.transition != nullptr && g.transition->secondary_clip != nullptr) {
        if (g.trim_type == olive::timeline::TRIM_IN) ghost_diff = -ghost_diff;
        g.in = g.old_in - ghost_diff;
        g.out = g.old_out + ghost_diff;
      } else if (g.trim_type == olive::timeline::TRIM_IN) {
        g.in = g.old_in + ghost_diff;
        g.clip_in = g.old_clip_in + ghost_diff;
      } else {
        g.out = g.old_out + ghost_diff;
      }
    } else if (clips_are_movable) {
      g.in = g.old_in + frame_diff;
      g.out = g.old_out + frame_diff;

      if (g.transition != nullptr
          && g.transition == g.clip->opening_transition) {
        g.clip_in = g.old_clip_in + frame_diff;
      }

      if (ParentTimeline()->importing) {

        g.track_movement = getTrackIndexFromScreenPoint(mouse_pos.y());

      } else if (g.track->type() == track_list_->type() && g.transition == nullptr) {

        g.track_movement = track_diff;

      }
    } else if (effective_tool == olive::timeline::TIMELINE_TOOL_TRANSITION) {
      if (ParentTimeline()->transition_tool_open_clip != nullptr
          && ParentTimeline()->transition_tool_close_clip != nullptr) {
        g.in = g.old_in - frame_diff;
        g.out = g.old_out + frame_diff;
      } else if (ParentTimeline()->transition_tool_open_clip == g.clip) {
        g.out = g.old_out + frame_diff;
      } else {
        g.in = g.old_in + frame_diff;
      }
    }

    earliest_in_point = qMin(earliest_in_point, g.in);
  }

  // apply changes to selections
  /*
  if (effective_tool != olive::timeline::TIMELINE_TOOL_SLIP && !ParentTimeline()->importing && !ParentTimeline()->creating) {
    for (int i=0;i<sequence()->selections.size();i++) {
      Selection& s = sequence()->selections[i];
      if (ParentTimeline()->trim_target > -1) {
        if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {
          s.in = s.old_in + frame_diff;
        } else {
          s.out = s.old_out + frame_diff;
        }
      } else if (clips_are_movable) {
        for (int i=0;i<sequence()->selections.size();i++) {
          Selection& s = sequence()->selections[i];
          s.in = s.old_in + frame_diff;
          s.out = s.old_out + frame_diff;
          s.track = s.old_track;

          if (ParentTimeline()->importing) {
            int abs_track_diff = abs(track_diff);
            if (s.old_track < 0) {
              s.track -= abs_track_diff;
            } else {
              s.track += abs_track_diff;
            }
          } else {
            if (same_sign(s.track, ParentTimeline()->drag_track_start)) s.track += track_diff;
          }
        }
      }
    }
  }
  */

  if (ParentTimeline()->importing) {
    QToolTip::showText(mapToGlobal(mouse_pos), frame_to_timecode(earliest_in_point, olive::config.timecode_view, sequence()->frame_rate));
  } else {
    QString tip = ((frame_diff < 0) ? "-" : "+") + frame_to_timecode(qAbs(frame_diff), olive::config.timecode_view, sequence()->frame_rate);

    if (ParentTimeline()->trim_target != nullptr) {
      // find which clip is being moved
      const Ghost* g = nullptr;
      for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
        if (ParentTimeline()->ghosts.at(i).clip == ParentTimeline()->trim_target) {
          g = &ParentTimeline()->ghosts.at(i);
          break;
        }
      }

      if (g != nullptr) {
        tip += " " + tr("Duration:") + " ";
        long len = (g->old_out-g->old_in);
        if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {
          len -= frame_diff;
        } else {
          len += frame_diff;
        }
        tip += frame_to_timecode(len, olive::config.timecode_view, sequence()->frame_rate);
      }
    }

    QToolTip::showText(mapToGlobal(mouse_pos), tip);
  }
}

void TimelineView::mouseMoveEvent(QMouseEvent *event) {

  // interrupt any potential tooltip about to show
  tooltip_timer.stop();

  if (sequence() != nullptr) {
    bool alt = (event->modifiers() & Qt::AltModifier);

    // store current frame/track corresponding to the cursor
    ParentTimeline()->cursor_frame = ParentTimeline()->getTimelineFrameFromScreenPoint(event->pos().x());
    ParentTimeline()->cursor_track = getTrackFromScreenPoint(event->pos().y());

    // if holding the mouse button down, let's scroll to that location
    if (event->buttons() != 0 && olive::timeline::current_tool != olive::timeline::TIMELINE_TOOL_HAND) {
      ParentTimeline()->scroll_to_frame(ParentTimeline()->cursor_frame);
    }

    // determine if the action should be "inserting" rather than "overwriting"
    // Default behavior is to replace/overwrite clips under any clips we're dropping over them. Inserting will
    // split and move existing clips at the drop point to make space for the drop
    ParentTimeline()->move_insert = ((event->modifiers() & Qt::ControlModifier)
                                     && (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER
                                         || ParentTimeline()->importing
                                         || ParentTimeline()->creating));

    // if we're not currently resizing already, default track resizing to false (we'll set it to true later if
    // the user is still hovering over a track line)
    if (!ParentTimeline()->moving_init) {
      track_resizing = false;
    }

    // if the current tool uses an on-screen visible cursor, we snap the cursor to the timeline
    if (current_tool_shows_cursor()) {
      sequence()->SnapPoint(&ParentTimeline()->cursor_frame,
                            ParentTimeline()->zoom,

                            // only snap to the playhead if the edit tool doesn't force the playhead to
                            // follow it (or if we're not selecting since that means the playhead is
                            // static at the moment)
                            !olive::config.edit_tool_also_seeks || !ParentTimeline()->selecting,

                            true,
                            true);
    }

    if (ParentTimeline()->selecting) {

      QVector<Selection> selections = ParentTimeline()->selection_cache;

      if (ParentTimeline()->drag_track_start != nullptr || ParentTimeline()->cursor_track != nullptr) {

        long selection_in = qMin(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame);
        long selection_out = qMax(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame);

        int selection_top = mapToGlobal(QPoint(0, ParentTimeline()->drag_y_start)).y();
        int selection_bottom = mapToGlobal(event->pos()).y();

        QVector<Track*> selected_tracks = ParentTimeline()->GetTracksInRectangle(selection_top,
                                                                                 selection_bottom);

        Track* track;

        foreach (track, selected_tracks) {

          selections.append(Selection(selection_in, selection_out, track));

          // If the config is set to select links as well with the edit tool
          if (olive::config.edit_tool_selects_links) {

            for (int j=0;j<track->ClipCount();j++) {

              Clip* c = track->GetClip(j).get();

              // See if this selection contains this clip
              if (!(c->timeline_in() > selection_out || c->timeline_out() < selection_in)) {

                // If so, select its links as well
                for (int k=0;k<c->linked.size();k++) {
                  Clip* link = c->linked.at(k);

                  // Make sure there isn't already a selection for this link
                  bool found = false;
                  for (int l=0;l<selections.size();l++) {
                    if (selections.at(l).in() == selection_in
                        && selections.at(l).out() == selection_out
                        && selections.at(l).track() == link->track()) {
                      found = true;
                      break;
                    }
                  }
                  // If not, make one now
                  if (!found) {
                    selections.append(Selection(selection_in, selection_out, link->track()));
                  }
                }
              }
            }
          }
        }
      }

      sequence()->SetSelections(selections);

      /*
      // get number of selections based on tracks in selection area
      int selection_tool_count = 1
          + qMax(ParentTimeline()->cursor_track->Index(), ParentTimeline()->drag_track_start->Index())
          - qMin(ParentTimeline()->cursor_track->Index(), ParentTimeline()->drag_track_start->Index());

      // add count to selection offset for the total number of selection objects
      // (offset is usually 0, unless the user is holding shift in which case we add to existing selections)
      int selection_count = selection_tool_count + ParentTimeline()->selection_offset;

      // resize selection object array to new count
      if (sequence()->selections.size() != selection_count) {
        sequence()->selections.resize(selection_count);
      }

      // loop through tracks in selection area and adjust them accordingly
      int minimum_selection_track = qMin(ParentTimeline()->cursor_track, ParentTimeline()->drag_track_start);
      int maximum_selection_track = qMax(ParentTimeline()->cursor_track, ParentTimeline()->drag_track_start);
      long selection_in = qMin(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame);
      long selection_out = qMax(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame);
      for (int i=ParentTimeline()->selection_offset;i<selection_count;i++) {
        Selection& s = sequence()->selections[i];
        s.track = minimum_selection_track + i - ParentTimeline()->selection_offset;
        s.in = selection_in;
        s.out = selection_out;
      }

      // If the config is set to select links as well with the edit tool
      if (olive::config.edit_tool_selects_links) {

        // find which clips are selected
        for (int j=0;j<sequence()->clips.size();j++) {

          Clip* c = sequence()->clips.at(j).get();

          if (c != nullptr && c->IsSelected(false)) {

            // loop through linked clips
            for (int k=0;k<c->linked.size();k++) {

              ClipPtr link = sequence()->clips.at(c->linked.at(k));

              // see if one of the selections is already covering this track
              if (!(link->track() >= minimum_selection_track
                    && link->track() <= maximum_selection_track)) {

                // clip is not in selectin area, time to select it
                Selection link_sel;
                link_sel.in = selection_in;
                link_sel.out = selection_out;
                link_sel.track = link->track();
                sequence()->selections.append(link_sel);

              }

            }

          }
        }
      }
      */

      // if the config is set to seek with the edit too, do so now
      if (olive::config.edit_tool_also_seeks) {
        panel_sequence_viewer->seek(qMin(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame));
      } else {
        // if not, repaint (seeking will trigger a repaint)
        ParentTimeline()->repaint_timeline();
      }

    } else if (ParentTimeline()->hand_moving) {

      // if we're hand moving, we'll be adding values directly to the scrollbars

      // the scrollbars trigger repaints when they scroll, which is unnecessary here so we block them
      ParentTimeline()->block_repaints = true;
      ParentTimeline()->horizontalScrollBar->setValue(ParentTimeline()->horizontalScrollBar->value() + ParentTimeline()->drag_x_start - event->pos().x());
      emit requestScrollChange(scroll + ParentTimeline()->drag_y_start - event->pos().y());
      ParentTimeline()->block_repaints = false;

      // finally repaint
      ParentTimeline()->repaint_timeline();

      // store current cursor position for next hand move event
      ParentTimeline()->drag_x_start = event->pos().x();
      ParentTimeline()->drag_y_start = event->pos().y();

    } else if (ParentTimeline()->moving_init) {

      if (track_resizing) {

        // get cursor movement
        int diff = (event->pos().y() - ParentTimeline()->drag_y_start);

        if (alignment_ == olive::timeline::kAlignmentBottom) {
          diff = -diff;
        }

        // add it to the current track height
        int new_height = track_target->height() + diff;

        // limit track height to track minimum height constant
        new_height = qMax(new_height, olive::timeline::kTrackMinHeight);

        // set the track height
        track_target->set_height(new_height);

        // store current cursor position for next track resize event
        ParentTimeline()->drag_y_start = event->pos().y();

        update();

      } else if (ParentTimeline()->moving_proc) {

        // we're currently dragging ghosts
        update_ghosts(event->pos(), event->modifiers() & Qt::ShiftModifier);

      } else {

        // Prepare to start moving clips in some capacity. We create Ghost objects to store movement data before we
        // actually apply it to the clips (in mouseReleaseEvent)

        // loop through clips for any currently selected
        QVector<Clip*> partially_selected_clips = sequence()->SelectedClips(false);
        for (int i=0;i<partially_selected_clips.size();i++) {

          Clip* c = partially_selected_clips.at(i);

          if (c != nullptr) {
            Ghost g;

            // check if whole clip is selected
            bool add = c->IsSelected();

            if (!add) {
              // check if a transition is selected
              // (only the pointer tool supports moving transitions)
              if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER
                  && (c->opening_transition != nullptr || c->closing_transition != nullptr)) {

                // check if any selections contain a whole transition
                if (c->IsTransitionSelected(kTransitionOpening)) {
                  g.transition = c->opening_transition;
                  add = true;
                } else if (c->IsTransitionSelected(kTransitionClosing)) {
                  g.transition = c->closing_transition;
                  add = true;
                }

              }
            }

            if (add && g.transition != nullptr) {

              // transition may be a shared transition, check if it's already been added elsewhere
              for (int j=0;j<ParentTimeline()->ghosts.size();j++) {
                if (ParentTimeline()->ghosts.at(j).transition == g.transition) {
                  add = false;
                  break;
                }
              }
            }

            if (add) {
              g.clip = c;
              g.trim_type = ParentTimeline()->trim_type;
              ParentTimeline()->ghosts.append(g);
            }
          }
        }

        if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_SLIDE) {

          // for the slide tool, we add the surrounding clips as ghosts that are getting trimmed the opposite way

          // store original array size since we'll be adding to it
          int ghost_arr_size = ParentTimeline()->ghosts.size();

          // loop through clips for any that are "touching" the selected clips
          for (int i=0;i<ghost_arr_size;i++) {
            Clip* ghost_clip = ParentTimeline()->ghosts.at(i).clip;

            Clip* pre_clip = ghost_clip->track()->GetClipFromPoint(ghost_clip->timeline_in() - 1);
            Clip* post_clip = ghost_clip->track()->GetClipFromPoint(ghost_clip->timeline_out() + 1);

            // Check if this clip is already in the ghosts, in which case don't add it
            for (int j=0;j<ghost_arr_size;j++) {
              if (ParentTimeline()->ghosts.at(j).clip == pre_clip) {
                pre_clip = nullptr;
              } else if (ParentTimeline()->ghosts.at(j).clip == post_clip) {
                post_clip = nullptr;
              }
            }

            Ghost gh;
            gh.transition = nullptr;

            if (pre_clip != nullptr) {
              gh.clip = pre_clip;
              gh.trim_type = olive::timeline::TRIM_OUT;
              ParentTimeline()->ghosts.append(gh);
            }

            if (post_clip != nullptr) {
              gh.clip = post_clip;
              gh.trim_type = olive::timeline::TRIM_IN;
              ParentTimeline()->ghosts.append(gh);
            }
          }
        }

        // set up ghost defaults
        init_ghosts();

        // if the ripple tool is selected, prepare to ripple
        if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_RIPPLE) {

          long axis = LONG_MAX;

          // find the earliest point within the selected clips which is the point we'll ripple around
          // also store the currently selected clips so we don't have to do it later
          QVector<Clip*> ghost_clips;
          ghost_clips.resize(ParentTimeline()->ghosts.size());

          for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
            Clip* c = ParentTimeline()->ghosts.at(i).clip;
            if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) {
              axis = qMin(axis, c->timeline_in());
            } else {
              axis = qMin(axis, c->timeline_out());
            }

            // store clip reference
            ghost_clips[i] = c;
          }

          // loop through clips and cache which are earlier than the axis and which after after
          QVector<Clip*> sequence_clips = sequence()->GetAllClips();
          for (int i=0;i<sequence_clips.size();i++) {
            Clip* c = sequence_clips.at(i);
            if (!ghost_clips.contains(c)) {
              bool clip_is_post = (c->timeline_in() >= axis);

              // construct the list of pre and post clips
              QVector<Clip*>& clip_list = (clip_is_post) ? post_clips : pre_clips;

              // check if there's already a clip in this list on this track, and if this clip is closer or not
              bool found = false;
              for (int j=0;j<clip_list.size();j++) {

                Clip* compare = clip_list.at(j);

                if (compare->track() == c->track()) {

                  // if the clip is closer, use this one instead of the current one in the list
                  if ((!clip_is_post && compare->timeline_out() < c->timeline_out())
                      || (clip_is_post && compare->timeline_in() > c->timeline_in())) {
                    clip_list[j] = c;
                  }

                  found = true;
                  break;
                }

              }

              // if there is no clip on this track in the list, add it
              if (!found) {
                clip_list.append(c);
              }
            }
          }
        }

        // store selections
        /*
        selection_command = new SetSelectionsCommand(sequence().get());
        selection_command->old_data = sequence()->selections;
        */

        // ready to start moving clips
        ParentTimeline()->moving_proc = true;
      }

      update_ui(false);

    } else if (ParentTimeline()->splitting) {

      ParentTimeline()->split_tracks = GetSplitTracksFromMouseCoords(!alt,
                                                                     ParentTimeline()->drag_frame_start,
                                                                     ParentTimeline()->drag_y_start,
                                                                     event->pos().y());
      update_ui(false);

    } else if (ParentTimeline()->rect_select_init) {

      // set if the user started dragging at point where there was no clip

      if (ParentTimeline()->rect_select_proc) {

        // we're currently rectangle selecting

        QVector<Selection> selections = ParentTimeline()->selection_cache;

        // set the right/bottom coords to the current mouse position
        // (left/top were set to the starting drag position earlier)
        ParentTimeline()->rect_select_rect.setBottomRight(mapToGlobal(event->pos()));

        QVector<Clip*> selected_clips;

        QVector<Track*> selected_tracks = ParentTimeline()->GetTracksInRectangle(ParentTimeline()->rect_select_rect.top(),
                                                                                 ParentTimeline()->rect_select_rect.bottom());

        long frame_min = qMin(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame);
        long frame_max = qMax(ParentTimeline()->drag_frame_start, ParentTimeline()->cursor_frame);

        Track* track;

        foreach (track, selected_tracks) {
          // Loop through track's clips for clips touching this rectangle
          for (int i=0;i<track->ClipCount();i++) {
            Clip* clip = track->GetClip(i).get();
            if (!(clip->timeline_out() < frame_min || clip->timeline_in() > frame_max) ) {

              // create a group of the clip (and its links if alt is not pressed)
              QVector<Clip*> session_clips;
              session_clips.append(clip);

              if (!alt) {
                session_clips.append(clip->linked);
              }

              // for each of these clips, see if clip has already been added -
              // this can easily happen due to adding linked clips
              for (int j=0;j<session_clips.size();j++) {
                Clip* c = session_clips.at(j);

                if (!selected_clips.contains(c)) {
                  selected_clips.append(c);
                }
              }
            }
          }
        }

        // add each of the selected clips to the main sequence's selections
        for (int i=0;i<selected_clips.size();i++) {
          selections.append(selected_clips.at(i)->ToSelection());
        }

        sequence()->SetSelections(selections);

        ParentTimeline()->repaint_timeline();
      } else {

        // set up rectangle selecting
        ParentTimeline()->rect_select_rect.setTopLeft(mapToGlobal(event->pos()));
        ParentTimeline()->rect_select_rect.setSize(QSize(0, 0));

        ParentTimeline()->rect_select_proc = true;

      }
    } else if (current_tool_shows_cursor()) {

      // we're not currently performing an action (click is not pressed), but redraw because we have an on-screen cursor
      ParentTimeline()->repaint_timeline();

    } else if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER ||
               olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_RIPPLE ||
               olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_ROLLING) {

      // hide any tooltip that may be currently showing
      QToolTip::hideText();

      // cache cursor position
      QPoint pos = event->pos();

      //
      // check to see if the cursor is on a clip edge
      //

      // threshold around a trim point that the cursor can be within and still considered "trimming"
      int lim = 10; // FIXME Magic number for the clip trimming threshold
      int mouse_frame_lower = pos.x() - lim;
      int mouse_frame_upper = pos.x() + lim;

      // used to determine whether we the cursor found a trim point or not
      bool found = false;

      // used to determine whether the cursor is within the rect of a clip
      bool cursor_contains_clip = false;

      // used to determine how close the cursor is to a trim point
      // (and more specifically, whether another point is closer or not)
      long closeness = LONG_MAX;

      // we default to selecting no transition, but set this accordingly if the cursor is on a transition
      ParentTimeline()->transition_select = kTransitionNone;

      // we also default to no trimming which may be changed later in this function
      ParentTimeline()->trim_type = olive::timeline::TRIM_NONE;

      // set currently trimming clip to -1 (aka null)
      ParentTimeline()->trim_target = nullptr;

      // loop through current clips in the sequence
      QVector<Clip*> sequence_clips = sequence()->GetAllClips();
      for (int i=0;i<sequence_clips.size();i++) {
        Clip* c = sequence_clips.at(i);

        // if this clip is on the same track the mouse is
        if (c->track() == ParentTimeline()->cursor_track) {

          // if this cursor is inside the boundaries of this clip (hovering over the clip)
          if (ParentTimeline()->cursor_frame >= c->timeline_in() &&
              ParentTimeline()->cursor_frame <= c->timeline_out()) {

            // acknowledge that we are hovering over a clip
            cursor_contains_clip = true;

            // start a timer to show a tooltip about this clip
            tooltip_timer.start();
            tooltip_clip = c;

            // check if the cursor is specifically hovering over one of the clip's transitions
            if (c->opening_transition != nullptr
                && ParentTimeline()->cursor_frame <= c->timeline_in() + c->opening_transition->get_true_length()) {

              ParentTimeline()->transition_select = kTransitionOpening;

            } else if (c->closing_transition != nullptr
                       && ParentTimeline()->cursor_frame >= c->timeline_out() - c->closing_transition->get_true_length()) {

              ParentTimeline()->transition_select = kTransitionClosing;

            }
          }

          int visual_in_point = ParentTimeline()->getTimelineScreenPointFromFrame(c->timeline_in());
          int visual_out_point = ParentTimeline()->getTimelineScreenPointFromFrame(c->timeline_out());

          // is the cursor hovering around the clip's IN point?
          if (visual_in_point > mouse_frame_lower && visual_in_point < mouse_frame_upper) {

            // test how close this IN point is to the cursor
            int nc = qAbs(visual_in_point + 1 - pos.x());

            // and test whether it's closer than the last in/out point we found
            if (nc < closeness) {

              // if so, this is the point we'll make active for now (unless we find a closer one later)
              ParentTimeline()->trim_target = c;
              ParentTimeline()->trim_type = olive::timeline::TRIM_IN;
              closeness = nc;
              found = true;

            }
          }

          // is the cursor hovering around the clip's OUT point?
          if (visual_out_point > mouse_frame_lower && visual_out_point < mouse_frame_upper) {

            // test how close this OUT point is to the cursor
            int nc = qAbs(visual_out_point - 1 - pos.x());

            // and test whether it's closer than the last in/out point we found
            if (nc < closeness) {

              // if so, this is the point we'll make active for now (unless we find a closer one later)
              ParentTimeline()->trim_target = c;
              ParentTimeline()->trim_type = olive::timeline::TRIM_OUT;
              closeness = nc;
              found = true;

            }
          }

          // the pointer can be used to resize/trim transitions, here we test if the
          // cursor is within the trim point of one of the clip's transitions
          if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_POINTER) {

            // if the clip has an opening transition
            if (c->opening_transition != nullptr) {

              // cache the timeline frame where the transition ends
              int transition_point = ParentTimeline()->getTimelineScreenPointFromFrame(c->timeline_in()
                                                                                     + c->opening_transition->get_true_length());

              // check if the cursor is hovering around it (within the threshold)
              if (transition_point > mouse_frame_lower && transition_point < mouse_frame_upper) {

                // similar to above, test how close it is and if it's closer, make this active
                int nc = qAbs(transition_point - 1 - pos.x());
                if (nc < closeness) {
                  ParentTimeline()->trim_target = c;
                  ParentTimeline()->trim_type = olive::timeline::TRIM_OUT;
                  ParentTimeline()->transition_select = kTransitionOpening;
                  closeness = nc;
                  found = true;
                }
              }
            }

            // if the clip has a closing transition
            if (c->closing_transition != nullptr) {

              // cache the timeline frame where the transition starts
              int transition_point = ParentTimeline()->getTimelineScreenPointFromFrame(c->timeline_out()
                                                                                     - c->closing_transition->get_true_length());

              // check if the cursor is hovering around it (within the threshold)
              if (transition_point > mouse_frame_lower && transition_point < mouse_frame_upper) {

                // similar to above, test how close it is and if it's closer, make this active
                int nc = qAbs(transition_point + 1 - pos.x());
                if (nc < closeness) {
                  ParentTimeline()->trim_target = c;
                  ParentTimeline()->trim_type = olive::timeline::TRIM_IN;
                  ParentTimeline()->transition_select = kTransitionClosing;
                  closeness = nc;
                  found = true;
                }
              }
            }
          }
        }
      }

      // if the cursor is indeed on a clip edge, we set the cursor accordingly
      if (found) {

        if (ParentTimeline()->trim_type == olive::timeline::TRIM_IN) { // if we're trimming an IN point
          setCursor(olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_RIPPLE ? olive::cursor::LeftRipple : olive::cursor::LeftTrim);
        } else { // if we're trimming an OUT point
          setCursor(olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_RIPPLE ? olive::cursor::RightRipple : olive::cursor::RightTrim);
        }

      } else {
        // we didn't find a trim target, so we must be doing something else
        // (e.g. dragging a clip or resizing the track heights)

        unsetCursor();

        // check to see if we're resizing a track height
        int mouse_pos = event->pos().y();

        // cursor range for resizing a track
        int test_range = 10; // FIXME magic number

        for (int i=0;i<track_list_->TrackCount();i++) {
          Track* track = track_list_->TrackAt(i);

          int effective_track_index = i;

          // If alignment is top, use the next track's screen point as the resize anchor so we resize the bottom
          // of the track
          if (alignment_ == olive::timeline::kAlignmentTop) {
            effective_track_index++;
          }

          int resize_point = getScreenPointFromTrackIndex(effective_track_index);

          if (mouse_pos > resize_point - test_range
              && mouse_pos < resize_point + test_range) {
            track_resizing = true;
            track_target = track;
            setCursor(Qt::SizeVerCursor);
            break;
          }
        }

      }
    } else if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_SLIP) {

      // we're not currently performing any slipping, all we do here is set the cursor if mouse is hovering over a
      // cursor
      if (GetClipAtCursor() != nullptr) {
        setCursor(olive::cursor::Slip);
      } else {
        unsetCursor();
      }

    } else if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_TRANSITION) {

      if (ParentTimeline()->transition_tool_init) {

        // the transition tool has started

        if (ParentTimeline()->transition_tool_proc) {

          // ghosts have been set up, so just run update
          update_ghosts(event->pos(), event->modifiers() & Qt::ShiftModifier);

        } else {

          // transition tool is being used but ghosts haven't been set up yet, set them up now
          TransitionType primary_type = kTransitionOpening;
          Clip* primary = ParentTimeline()->transition_tool_open_clip;
          if (primary == nullptr) {
            primary_type = kTransitionClosing;
            primary = ParentTimeline()->transition_tool_close_clip;
          }

          Ghost g;

          g.in = g.old_in = g.out = g.old_out = (primary_type == kTransitionOpening) ?
                primary->timeline_in()
              : primary->timeline_out();

          g.track = primary->track();
          g.clip = primary;
          g.media_stream = primary_type;
          g.trim_type = olive::timeline::TRIM_NONE;

          ParentTimeline()->ghosts.append(g);

          ParentTimeline()->transition_tool_proc = true;

        }

      } else {

        // transition tool has been selected but is not yet active, so we show screen feedback to the user on
        // possible transitions

        Clip* mouse_clip = GetClipAtCursor();

        // set default transition tool references to no clip
        ParentTimeline()->transition_tool_open_clip = nullptr;
        ParentTimeline()->transition_tool_close_clip = nullptr;

        if (mouse_clip != nullptr) {

          // cursor is hovering over a clip

          // check if the clip and transition are both the same sign (meaning video/audio are the same)
          if (track_list_->type() == ParentTimeline()->transition_tool_side) {

            // the range within which the transition tool will assume the user wants to make a shared transition
            // between two clips rather than just one transition on one clip
            long between_range = getFrameFromScreenPoint(ParentTimeline()->zoom, TRANSITION_BETWEEN_RANGE) + 1;

            // set whether the transition is opening or closing based on whether the cursor is on the left half
            // or right half of the clip
            if (ParentTimeline()->cursor_frame > (mouse_clip->timeline_in() + (mouse_clip->length()/2))) {
              ParentTimeline()->transition_tool_close_clip = mouse_clip;

              // if the cursor is within this range, set the post_clip to be the next clip touching
              //
              // getClipIndexFromCoords() will automatically set to -1 if there's no clip there which means the
              // end result will be the same as not setting a clip here at all
              if (ParentTimeline()->cursor_frame > mouse_clip->timeline_out() - between_range) {
                ParentTimeline()->transition_tool_open_clip = mouse_clip->track()->GetClipFromPoint(mouse_clip->timeline_out()+1);
              }
            } else {
              ParentTimeline()->transition_tool_open_clip = mouse_clip;

              if (ParentTimeline()->cursor_frame < mouse_clip->timeline_in() + between_range) {
                ParentTimeline()->transition_tool_close_clip = mouse_clip->track()->GetClipFromPoint(mouse_clip->timeline_in()-1);
              }
            }

          }
        }
      }

      ParentTimeline()->repaint_timeline();
    }
  }
}

void TimelineView::leaveEvent(QEvent*) {
  tooltip_timer.stop();
}

void TimelineView::draw_transition(QPainter& p, Clip* c, const QRect& clip_rect, QRect& text_rect, int transition_type) {
  TransitionPtr t = (transition_type == kTransitionOpening) ? c->opening_transition : c->closing_transition;
  if (t != nullptr) {
    QColor transition_color(255, 0, 0, 16);
    int transition_width = getScreenPointFromFrame(ParentTimeline()->zoom, t->get_true_length());
    int transition_height = clip_rect.height();
    int tr_y = clip_rect.y();
    int tr_x = 0;
    if (transition_type == kTransitionOpening) {
      tr_x = clip_rect.x();
      text_rect.setX(text_rect.x()+transition_width);
    } else {
      tr_x = clip_rect.right()-transition_width;
      text_rect.setWidth(text_rect.width()-transition_width);
    }
    QRect transition_rect = QRect(tr_x, tr_y, transition_width, transition_height);
    p.fillRect(transition_rect, transition_color);
    QRect transition_text_rect(transition_rect.x() + olive::timeline::kClipTextPadding, transition_rect.y() + olive::timeline::kClipTextPadding, transition_rect.width() - olive::timeline::kClipTextPadding, transition_rect.height() - olive::timeline::kClipTextPadding);
    if (transition_text_rect.width() > MAX_TEXT_WIDTH) {
      bool draw_text = true;

      p.setPen(QColor(0, 0, 0, 96));
      if (t->secondary_clip == nullptr) {
        if (transition_type == kTransitionOpening) {
          p.drawLine(transition_rect.bottomLeft(), transition_rect.topRight());
        } else {
          p.drawLine(transition_rect.topLeft(), transition_rect.bottomRight());
        }
      } else {
        if (transition_type == kTransitionOpening) {
          p.drawLine(QPoint(transition_rect.left(), transition_rect.center().y()), transition_rect.topRight());
          p.drawLine(QPoint(transition_rect.left(), transition_rect.center().y()), transition_rect.bottomRight());
          draw_text = false;
        } else {
          p.drawLine(QPoint(transition_rect.right(), transition_rect.center().y()), transition_rect.topLeft());
          p.drawLine(QPoint(transition_rect.right(), transition_rect.center().y()), transition_rect.bottomLeft());
        }
      }

      if (draw_text) {
        p.setPen(Qt::white);
        p.drawText(transition_text_rect, 0, t->meta->name, &transition_text_rect);
      }
    }
    p.setPen(Qt::black);
    p.drawRect(transition_rect);
  }

}

void TimelineView::paintEvent(QPaintEvent*) {
  // Draw clips
  if (track_list_ != nullptr) {
    QPainter p(this);

    // get widget width and height
    emit setScrollMaximum(GetTotalAreaHeight());

    for (int i=0;i<track_list_->TrackCount();i++) {

      Track* track = track_list_->TrackAt(i);

      int track_top = getScreenPointFromTrack(track);
      int track_bottom = track_top + track->height();

      if (track_bottom > 0 && track_top < height()) {
        for (int j=0;j<track->ClipCount();j++) {

          Clip* clip = track->GetClip(j).get();

          QRect clip_rect(ParentTimeline()->getTimelineScreenPointFromFrame(clip->timeline_in()),
                          track_top,
                          getScreenPointFromFrame(ParentTimeline()->zoom, clip->length()),
                          track->height());

          if (alignment_ == olive::timeline::kAlignmentTop) {
            clip_rect.setHeight(track->height() - 1);
          } else if (alignment_ == olive::timeline::kAlignmentBottom) {
            clip_rect.setTop(track_top + 1);
          }

          QRect text_rect(clip_rect.left() + olive::timeline::kClipTextPadding,
                          clip_rect.top() + olive::timeline::kClipTextPadding,
                          clip_rect.width() - olive::timeline::kClipTextPadding - 1,
                          clip_rect.height() - olive::timeline::kClipTextPadding - 1);
          if (clip_rect.left() < width() && clip_rect.right() >= 0 && clip_rect.top() < height() && clip_rect.bottom() >= 0) {
            QRect actual_clip_rect = clip_rect;
            if (actual_clip_rect.x() < 0) actual_clip_rect.setX(0);
            if (actual_clip_rect.right() > width()) actual_clip_rect.setRight(width());
            if (actual_clip_rect.y() < 0) actual_clip_rect.setY(0);
            if (actual_clip_rect.bottom() > height()) actual_clip_rect.setBottom(height());
            p.fillRect(actual_clip_rect, (clip->enabled()) ? clip->color() : QColor(96, 96, 96));

            int thumb_x = clip_rect.x() + 1;

            if (clip->media() != nullptr && clip->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
              bool draw_checkerboard = false;
              QRect checkerboard_rect(clip_rect);
              FootageStream* ms = clip->media_stream();
              if (ms == nullptr) {
                draw_checkerboard = true;
              } else if (ms->preview_done) {
                // draw top and tail triangles
                int triangle_size = olive::timeline::kTrackMinHeight >> 2;
                if (!ms->infinite_length && clip_rect.width() > triangle_size) {
                  p.setPen(Qt::NoPen);
                  p.setBrush(QColor(80, 80, 80));
                  if (clip->clip_in() == 0
                      && clip_rect.x() + triangle_size > 0
                      && clip_rect.y() + triangle_size > 0
                      && clip_rect.x() < width()
                      && clip_rect.y() < height()) {
                    const QPoint points[3] = {
                      QPoint(clip_rect.x(), clip_rect.y()),
                      QPoint(clip_rect.x() + triangle_size, clip_rect.y()),
                      QPoint(clip_rect.x(), clip_rect.y() + triangle_size)
                    };
                    p.drawPolygon(points, 3);
                    text_rect.setLeft(text_rect.left() + (triangle_size >> 2));
                  }
                  if (clip->timeline_out() - clip->timeline_in() + clip->clip_in() == clip->media_length()
                      && clip_rect.right() - triangle_size < width()
                      && clip_rect.y() + triangle_size > 0
                      && clip_rect.right() > 0
                      && clip_rect.y() < height()) {
                    const QPoint points[3] = {
                      QPoint(clip_rect.right(), clip_rect.y()),
                      QPoint(clip_rect.right() - triangle_size, clip_rect.y()),
                      QPoint(clip_rect.right(), clip_rect.y() + triangle_size)
                    };
                    p.drawPolygon(points, 3);
                    text_rect.setRight(text_rect.right() - (triangle_size >> 2));
                  }
                }

                p.setBrush(Qt::NoBrush);

                // draw thumbnail/waveform
                long media_length = clip->media_length();

                if (clip->type() == Track::kTypeVideo) {
                  // draw thumbnail
                  int thumb_y = p.fontMetrics().height()+olive::timeline::kClipTextPadding+olive::timeline::kClipTextPadding;
                  if (thumb_x < width() && thumb_y < height()) {
                    int space_for_thumb = clip_rect.width()-1;
                    if (clip->opening_transition != nullptr) {
                      int ot_width = getScreenPointFromFrame(ParentTimeline()->zoom, clip->opening_transition->get_true_length());
                      thumb_x += ot_width;
                      space_for_thumb -= ot_width;
                    }
                    if (clip->closing_transition != nullptr) {
                      space_for_thumb -= getScreenPointFromFrame(ParentTimeline()->zoom, clip->closing_transition->get_true_length());
                    }
                    int thumb_height = clip_rect.height()-thumb_y;
                    int thumb_width = qRound(thumb_height*(double(ms->video_preview.width())/double(ms->video_preview.height())));
                    if (thumb_x + thumb_width >= 0
                        && thumb_height > thumb_y
                        && thumb_y + thumb_height >= 0
                        && space_for_thumb > MAX_TEXT_WIDTH) {
                      int thumb_clip_width = qMin(thumb_width, space_for_thumb);
                      p.drawImage(QRect(thumb_x,
                                        clip_rect.y()+thumb_y,
                                        thumb_clip_width,
                                        thumb_height),
                                  ms->video_preview,
                                  QRect(0,
                                        0,
                                        qRound(thumb_clip_width*(double(ms->video_preview.width())/double(thumb_width))),
                                        ms->video_preview.height()
                                        )
                                  );
                    }
                  }
                  if (clip->timeline_out() - clip->timeline_in() + clip->clip_in() > clip->media_length()) {
                    draw_checkerboard = true;
                    checkerboard_rect.setLeft(ParentTimeline()->getTimelineScreenPointFromFrame(clip->media_length() + clip->timeline_in() - clip->clip_in()));
                  }
                } else if (clip_rect.height() > olive::timeline::kTrackMinHeight) {
                  // draw waveform
                  p.setPen(QColor(80, 80, 80));

                  int waveform_start = -qMin(clip_rect.x(), 0);
                  int waveform_limit = qMin(clip_rect.width(), getScreenPointFromFrame(ParentTimeline()->zoom, media_length - clip->clip_in()));

                  if ((clip_rect.x() + waveform_limit) > width()) {
                    waveform_limit -= (clip_rect.x() + waveform_limit - width());
                  } else if (waveform_limit < clip_rect.width()) {
                    draw_checkerboard = true;
                    if (waveform_limit > 0) checkerboard_rect.setLeft(checkerboard_rect.left() + waveform_limit);
                  }

                  olive::ui::DrawWaveform(clip, ms, media_length, &p, clip_rect, waveform_start, waveform_limit, ParentTimeline()->zoom);
                }
              }
              if (draw_checkerboard) {
                checkerboard_rect.setLeft(qMax(checkerboard_rect.left(), 0));
                checkerboard_rect.setRight(qMin(checkerboard_rect.right(), width()));
                checkerboard_rect.setTop(qMax(checkerboard_rect.top(), 0));
                checkerboard_rect.setBottom(qMin(checkerboard_rect.bottom(), height()));

                if (checkerboard_rect.left() < width()
                    && checkerboard_rect.right() >= 0
                    && checkerboard_rect.top() < height()
                    && checkerboard_rect.bottom() >= 0) {
                  // draw "error lines" if media stream is missing
                  p.setPen(QPen(QColor(64, 64, 64), 2));
                  int limit = checkerboard_rect.width();
                  int clip_height = checkerboard_rect.height();
                  for (int j=-clip_height;j<limit;j+=15) {
                    int lines_start_x = checkerboard_rect.left()+j;
                    int lines_start_y = checkerboard_rect.bottom();
                    int lines_end_x = lines_start_x + clip_height;
                    int lines_end_y = checkerboard_rect.top();
                    if (lines_start_x < checkerboard_rect.left()) {
                      lines_start_y -= (checkerboard_rect.left() - lines_start_x);
                      lines_start_x = checkerboard_rect.left();
                    }
                    if (lines_end_x > checkerboard_rect.right()) {
                      lines_end_y -= (checkerboard_rect.right() - lines_end_x);
                      lines_end_x = checkerboard_rect.right();
                    }
                    p.drawLine(lines_start_x, lines_start_y, lines_end_x, lines_end_y);
                  }
                }
              }
            }

            // draw clip markers
            for (int j=0;j<clip->get_markers().size();j++) {
              const Marker& m = clip->get_markers().at(j);

              // convert marker time (in clip time) to sequence time
              long marker_time = m.frame + clip->timeline_in() - clip->clip_in();
              int marker_x = ParentTimeline()->getTimelineScreenPointFromFrame(marker_time);
              if (marker_x > clip_rect.x() && marker_x < clip_rect.right()) {
                Marker::Draw(p, marker_x, clip_rect.bottom()-p.fontMetrics().height(), clip_rect.bottom(), false);
              }
            }
            p.setBrush(Qt::NoBrush);

            // draw clip transitions
            draw_transition(p, clip, clip_rect, text_rect, kTransitionOpening);
            draw_transition(p, clip, clip_rect, text_rect, kTransitionClosing);

            // top left bevel
            p.setPen(Qt::white);
            if (clip_rect.x() >= 0 && clip_rect.x() < width()) p.drawLine(clip_rect.bottomLeft(), clip_rect.topLeft());
            if (clip_rect.y() >= 0 && clip_rect.y() < height()) p.drawLine(QPoint(qMax(0, clip_rect.left()), clip_rect.top()), QPoint(qMin(width(), clip_rect.right()), clip_rect.top()));

            // draw text
            if (text_rect.width() > MAX_TEXT_WIDTH && text_rect.right() > 0 && text_rect.left() < width()) {
              if (!clip->enabled()) {
                p.setPen(Qt::gray);
              } else if (clip->color().lightness() > 160) {
                // set to black if color is bright
                p.setPen(Qt::black);
              }
              if (clip->linked.size() > 0) {
                int underline_y = olive::timeline::kClipTextPadding + p.fontMetrics().height() + clip_rect.top();
                int underline_width = qMin(text_rect.width() - 1, p.fontMetrics().width(clip->name()));
                p.drawLine(text_rect.x(), underline_y, text_rect.x() + underline_width, underline_y);
              }
              QString name = clip->name();
              if (clip->speed().value != 1.0 || clip->reversed()) {
                name += " (";
                if (clip->reversed()) name += "-";
                name += QString::number(clip->speed().value*100) + "%)";
              }
              p.drawText(text_rect, 0, name, &text_rect);
            }

            // bottom right gray
            p.setPen(QColor(0, 0, 0, 128));
            if (clip_rect.right() >= 0 && clip_rect.right() < width()) p.drawLine(clip_rect.bottomRight(), clip_rect.topRight());
            if (clip_rect.bottom() >= 0 && clip_rect.bottom() < height()) p.drawLine(QPoint(qMax(0, clip_rect.left()), clip_rect.bottom()), QPoint(qMin(width(), clip_rect.right()), clip_rect.bottom()));

            // draw transition tool
            if (olive::timeline::current_tool == olive::timeline::TIMELINE_TOOL_TRANSITION) {

              bool shared_transition = (ParentTimeline()->transition_tool_open_clip != nullptr
                  && ParentTimeline()->transition_tool_close_clip != nullptr);

              QRect transition_tool_rect = clip_rect;
              bool draw_transition_tool_rect = false;

              if (ParentTimeline()->transition_tool_open_clip == clip) {
                if (shared_transition) {
                  transition_tool_rect.setWidth(TRANSITION_BETWEEN_RANGE);
                } else {
                  transition_tool_rect.setWidth(transition_tool_rect.width()>>2);
                }
                draw_transition_tool_rect = true;
              } else if (ParentTimeline()->transition_tool_close_clip == clip) {
                if (shared_transition) {
                  transition_tool_rect.setLeft(transition_tool_rect.right() - TRANSITION_BETWEEN_RANGE);
                } else {
                  transition_tool_rect.setLeft(transition_tool_rect.left() + (3*(transition_tool_rect.width()>>2)));
                }
                draw_transition_tool_rect = true;
              }

              if (draw_transition_tool_rect
                  && transition_tool_rect.left() < width()
                  && transition_tool_rect.right() > 0) {
                if (transition_tool_rect.left() < 0) {
                  transition_tool_rect.setLeft(0);
                }
                if (transition_tool_rect.right() > width()) {
                  transition_tool_rect.setRight(width());
                }
                p.fillRect(transition_tool_rect, QColor(0, 0, 0, 128));
              }
            }
          }
        }

        // Draw recording clip if recording if valid
        if (panel_sequence_viewer->is_recording_cued() && panel_sequence_viewer->recording_track == track) {
          int rec_track_x = ParentTimeline()->getTimelineScreenPointFromFrame(panel_sequence_viewer->recording_start);
          int rec_track_y = getScreenPointFromTrack(panel_sequence_viewer->recording_track);
          int rec_track_height = track->height();
          if (panel_sequence_viewer->recording_start != panel_sequence_viewer->recording_end) {
            QRect rec_rect(
                  rec_track_x,
                  rec_track_y,
                  getScreenPointFromFrame(ParentTimeline()->zoom, panel_sequence_viewer->recording_end - panel_sequence_viewer->recording_start),
                  rec_track_height
                  );
            p.setPen(QPen(QColor(96, 96, 96), 2));
            p.fillRect(rec_rect, QColor(192, 192, 192));
            p.drawRect(rec_rect);
          }
          QRect active_rec_rect(
                rec_track_x,
                rec_track_y,
                getScreenPointFromFrame(ParentTimeline()->zoom, panel_sequence_viewer->seq->playhead - panel_sequence_viewer->recording_start),
                rec_track_height
                );
          p.setPen(QPen(QColor(192, 0, 0), 2));
          p.fillRect(active_rec_rect, QColor(255, 96, 96));
          p.drawRect(active_rec_rect);

          p.setPen(Qt::NoPen);

          if (!panel_sequence_viewer->playing) {
            int rec_marker_size = 6;
            int rec_track_midY = rec_track_y + (rec_track_height >> 1);
            p.setBrush(Qt::white);
            QPoint cue_marker[3] = {
              QPoint(rec_track_x, rec_track_midY - rec_marker_size),
              QPoint(rec_track_x + rec_marker_size, rec_track_midY),
              QPoint(rec_track_x, rec_track_midY + rec_marker_size)
            };
            p.drawPolygon(cue_marker, 3);
          }
        }

        // Draw selections
        QVector<Selection> selections = track->Selections();
        for (int j=0;j<selections.size();j++) {
          const Selection& s = selections.at(j);

          int selection_x = ParentTimeline()->getTimelineScreenPointFromFrame(s.in());
          p.setPen(Qt::NoPen);
          p.setBrush(Qt::NoBrush);
          p.fillRect(selection_x,
                     track_top,
                     ParentTimeline()->getTimelineScreenPointFromFrame(s.out()) - selection_x,
                     track->height(),
                     QColor(0, 0, 0, 64));
        }

        // Draw splitting cursor
        if (ParentTimeline()->splitting && ParentTimeline()->split_tracks.contains(track)) {
          int cursor_x = ParentTimeline()->getTimelineScreenPointFromFrame(ParentTimeline()->drag_frame_start);

          p.setPen(QColor(64, 64, 64));
          p.drawLine(cursor_x,
                     track_top,
                     cursor_x,
                     track_top + track->height());
        }

        // Draw edit cursor
        if (current_tool_shows_cursor() && ParentTimeline()->cursor_track == track) {
          int cursor_x = ParentTimeline()->getTimelineScreenPointFromFrame(ParentTimeline()->cursor_frame);

          p.setPen(Qt::gray);
          p.drawLine(cursor_x,
                     track_top,
                     cursor_x,
                     track_top + track->height());
        }

        // Draw track line
        p.setPen(QColor(0, 0, 0, 96));
        if (alignment_ == olive::timeline::kAlignmentTop) {
          p.drawLine(0, track_bottom, rect().width(), track_bottom);
        } else if (alignment_ == olive::timeline::kAlignmentBottom) {
          p.drawLine(0, track_top, rect().width(), track_top);
        }
      }
    }

    // draw rectangle select
    if (ParentTimeline()->rect_select_proc) {
      QRect relative_rect = QRect(mapFromGlobal(ParentTimeline()->rect_select_rect.topLeft()),
                                  mapFromGlobal(ParentTimeline()->rect_select_rect.bottomRight()));

      olive::ui::DrawSelectionRectangle(p, relative_rect);
    }

    // Draw ghosts
    if (!ParentTimeline()->ghosts.isEmpty()) {
      QVector<int> insert_points;
      long first_ghost = LONG_MAX;
      for (int i=0;i<ParentTimeline()->ghosts.size();i++) {
        const Ghost& g = ParentTimeline()->ghosts.at(i);
        first_ghost = qMin(first_ghost, g.in);
        if (g.track->type() == track_list_->type()) {

          int ghost_x = ParentTimeline()->getTimelineScreenPointFromFrame(g.in);
          int ghost_y = getScreenPointFromTrackIndex(g.track->Index() + g.track_movement);
          int ghost_width = ParentTimeline()->getTimelineScreenPointFromFrame(g.out) - ghost_x - 1;
          int ghost_height = getTrackHeightFromTrackIndex(g.track->Index() + g.track_movement) - 1;

          insert_points.append(ghost_y + (ghost_height>>1));

          p.setPen(QColor(255, 255, 0));
          for (int j=0;j<olive::timeline::kGhostThickness;j++) {
            p.drawRect(ghost_x+j, ghost_y+j, ghost_width-j-j, ghost_height-j-j);
          }
        }
      }

      // draw insert indicator
      if (ParentTimeline()->move_insert && !insert_points.isEmpty()) {
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        int insert_x = ParentTimeline()->getTimelineScreenPointFromFrame(first_ghost);
        int tri_size = olive::timeline::kTrackMinHeight>>2;

        for (int i=0;i<insert_points.size();i++) {
          QPoint points[3] = {
            QPoint(insert_x, insert_points.at(i) - tri_size),
            QPoint(insert_x + tri_size, insert_points.at(i)),
            QPoint(insert_x, insert_points.at(i) + tri_size)
          };
          p.drawPolygon(points, 3);
        }
      }
    }

    // Draw playhead
    p.setPen(Qt::red);
    int playhead_x = ParentTimeline()->getTimelineScreenPointFromFrame(sequence()->playhead);
    p.drawLine(playhead_x, rect().top(), playhead_x, rect().bottom());

    // Draw single frame highlight
    int playhead_frame_width = ParentTimeline()->getTimelineScreenPointFromFrame(sequence()->playhead+1) - playhead_x;
    if (playhead_frame_width > 5){ //hardcoded for now, maybe better way to do this?
      QRectF singleFrameRect(playhead_x, rect().top(), playhead_frame_width, rect().bottom());
      p.fillRect(singleFrameRect, QColor(255,255,255,15));
    }

    // draw border
    /*
    p.setPen(QColor(0, 0, 0, 64));
    int edge_y = 0;
    p.drawLine(0, edge_y, rect().width(), edge_y);
    edge_y = rect().height()-1;
    p.drawLine(0, edge_y, rect().width(), edge_y);
    */

    // draw snap point
    if (olive::timeline::snapped) {
      p.setPen(Qt::white);
      int snap_x = ParentTimeline()->getTimelineScreenPointFromFrame(olive::timeline::snap_point);
      p.drawLine(snap_x, 0, snap_x, height());
    }
  }
}

// **************************************
// screen point <-> frame/track functions
// **************************************

Track *TimelineView::getTrackFromScreenPoint(int y) {

  int index = getTrackIndexFromScreenPoint(y);

  if (index < track_list_->TrackCount()) {
    return track_list_->TrackAt(index);
  }

  return nullptr;

}

int TimelineView::getScreenPointFromTrack(Track *track) {
  return getScreenPointFromTrackIndex(track_list_->IndexOfTrack(track));
}

int TimelineView::getTrackIndexFromScreenPoint(int y)
{
  if (alignment_ == olive::timeline::kAlignmentSingle) {
    return 0;
  }

  if (alignment_ == olive::timeline::kAlignmentBottom) {
    y = -(y + 1 + scroll - qMax(height(), GetTotalAreaHeight()));
  } else {
    y += scroll;
  }

  if (y < 0) {
    return 0;
  }

  int heights = 0;

  int i = 0;
  while (true) {

    int new_heights = heights;

    if (i < track_list_->TrackCount()) {
      new_heights += track_list_->TrackAt(i)->height();
    } else {
      new_heights += olive::timeline::kTrackDefaultHeight;
    }

    if (y >= heights && y < new_heights) {
      return i;
    }

    heights = new_heights;

    i++;
  }

}

int TimelineView::getScreenPointFromTrackIndex(int track)
{
  if (alignment_ == olive::timeline::kAlignmentSingle) {
    return 0;
  }

  int point = 0;

  for (int i=0;i<track;i++) {
    point += getTrackHeightFromTrackIndex(i) + 1;
  }

  if (alignment_ == olive::timeline::kAlignmentBottom) {
    return qMax(height(), GetTotalAreaHeight()) - point - scroll - track_list_->First()->height() - 1;
  }

  return point - scroll;
}

int TimelineView::getTrackHeightFromTrackIndex(int track)
{
  if (track < track_list_->TrackCount()) {
    return track_list_->TrackAt(track)->height();
  } else {
    return olive::timeline::kTrackDefaultHeight;
  }
}

Timeline *TimelineView::ParentTimeline()
{
  return timeline_;
}

Sequence *TimelineView::sequence()
{
  if (track_list_ == nullptr) {
    return nullptr;
  }

  return track_list_->GetParent();
}

void TimelineView::setScroll(int s) {
  scroll = s;
  update();
}

void TimelineView::reveal_media() {
  panel_project.first()->reveal_media(rc_reveal_media);
}
