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

#include "timelinewidget.h"

#include "oliveglobal.h"
#include "panels/panels.h"
#include "project/projectelements.h"

#include "rendering/audio.h"
#include "io/config.h"
#include "ui/sourcetable.h"
#include "ui/sourceiconview.h"
#include "project/undo.h"
#include "ui/viewerwidget.h"
#include "ui/resizablescrollbar.h"
#include "dialogs/newsequencedialog.h"
#include "mainwindow.h"
#include "ui/rectangleselect.h"
#include "rendering/renderfunctions.h"
#include "ui/cursors.h"
#include "ui/menuhelper.h"
#include "ui/focusfilter.h"
#include "dialogs/clippropertiesdialog.h"
#include "debug.h"

#include "project/effect.h"

#include "effects/internal/solideffect.h"
#include "effects/internal/texteffect.h"

#include <QPainter>
#include <QColor>
#include <QMouseEvent>
#include <QObject>
#include <QVariant>
#include <QPoint>
#include <QMenu>
#include <QMessageBox>
#include <QtMath>
#include <QScrollBar>
#include <QMimeData>
#include <QToolTip>
#include <QInputDialog>
#include <QStatusBar>

#define MAX_TEXT_WIDTH 20
#define TRANSITION_BETWEEN_RANGE 40

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
  selection_command = nullptr;
  self_created_sequence = nullptr;
  scroll = 0;

  bottom_align = false;
  track_resizing = false;
  setMouseTracking(true);

  setAcceptDrops(true);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));

  tooltip_timer.setInterval(500);
  connect(&tooltip_timer, SIGNAL(timeout()), this, SLOT(tooltip_timer_timeout()));
}

void TimelineWidget::show_context_menu(const QPoint& pos) {
  if (olive::ActiveSequence != nullptr) {
    // hack because sometimes right clicking doesn't trigger mouse release event
    panel_timeline->rect_select_init = false;
    panel_timeline->rect_select_proc = false;

    QMenu menu(this);

    // TODO replace with Olive::MenuHelper::make_edit_functions_menu() without losing functionality

    QAction* undoAction = menu.addAction(tr("&Undo"));
    QAction* redoAction = menu.addAction(tr("&Redo"));
    connect(undoAction, SIGNAL(triggered(bool)), olive::Global.get(), SLOT(undo()));
    connect(redoAction, SIGNAL(triggered(bool)), olive::Global.get(), SLOT(redo()));
    undoAction->setEnabled(olive::UndoStack.canUndo());
    redoAction->setEnabled(olive::UndoStack.canRedo());
    menu.addSeparator();

    // collect all the selected clips
    QVector<Clip*> selected_clips = olive::ActiveSequence->SelectedClips();

    if (!selected_clips.isEmpty()) {
      // clips are selected
      menu.addAction(tr("C&ut"), &olive::FocusFilter, SLOT(cut()));
      menu.addAction(tr("Cop&y"), &olive::FocusFilter, SLOT(copy()));
    }

    menu.addAction(tr("&Paste"), olive::Global.get(), SLOT(paste()));

    if (selected_clips.isEmpty()) {
      // no clips are selected

      // determine if we can perform a ripple empty space
      panel_timeline->cursor_frame = panel_timeline->getTimelineFrameFromScreenPoint(pos.x());
      panel_timeline->cursor_track = getTrackFromScreenPoint(pos.y());

      if (panel_timeline->can_ripple_empty_space(panel_timeline->cursor_frame, panel_timeline->cursor_track)) {
        QAction* ripple_delete_action = menu.addAction(tr("R&ipple Delete"));
        connect(ripple_delete_action, SIGNAL(triggered(bool)), panel_timeline, SLOT(ripple_delete_empty_space()));
      }

      QAction* seq_settings = menu.addAction(tr("Sequence Settings"));
      connect(seq_settings, SIGNAL(triggered(bool)), this, SLOT(open_sequence_properties()));
    }

    if (!selected_clips.isEmpty()) {
      menu.addSeparator();
      menu.addAction(tr("&Speed/Duration"), olive::Global.get(), SLOT(open_speed_dialog()));

      QAction* autoscaleAction = menu.addAction(tr("Auto-s&cale"), this, SLOT(toggle_autoscale()));
      autoscaleAction->setCheckable(true);
      // set autoscale to the first selected clip
      autoscaleAction->setChecked(selected_clips.at(0)->autoscaled());

      olive::MenuHelper.make_clip_functions_menu(&menu);

      // stabilizer option
      /*int video_clip_count = 0;
      bool all_video_is_footage = true;
      for (int i=0;i<selected_clips.size();i++) {
        if (selected_clips.at(i)->track() < 0) {
          video_clip_count++;
          if (selected_clips.at(i)->media() == nullptr
              || selected_clips.at(i)->media()->get_type() != MEDIA_TYPE_FOOTAGE) {
            all_video_is_footage = false;
          }
        }
      }
      if (video_clip_count == 1 && all_video_is_footage) {
        QAction* stabilizerAction = menu.addAction("S&tabilizer");
        connect(stabilizerAction, SIGNAL(triggered(bool)), this, SLOT(show_stabilizer_diag()));
      }*/

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

void TimelineWidget::toggle_autoscale() {
  QVector<Clip*> selected_clips = olive::ActiveSequence->SelectedClips();

  if (!selected_clips.isEmpty()) {
    SetClipProperty* action = new SetClipProperty(kSetClipPropertyAutoscale);

    for (int i=0;i<selected_clips.size();i++) {
      Clip* c = selected_clips.at(i);
      action->AddSetting(c, !c->autoscaled());
    }

    olive::UndoStack.push(action);
  }
}

void TimelineWidget::tooltip_timer_timeout() {
  if (olive::ActiveSequence != nullptr) {
    if (tooltip_clip < olive::ActiveSequence->clips.size()) {
      ClipPtr c = olive::ActiveSequence->clips.at(tooltip_clip);
      if (c != nullptr) {
        QToolTip::showText(QCursor::pos(),
                           tr("%1\nStart: %2\nEnd: %3\nDuration: %4").arg(
                             c->name(),
                             frame_to_timecode(c->timeline_in(), olive::CurrentConfig.timecode_view, olive::ActiveSequence->frame_rate),
                             frame_to_timecode(c->timeline_out(), olive::CurrentConfig.timecode_view, olive::ActiveSequence->frame_rate),
                             frame_to_timecode(c->length(), olive::CurrentConfig.timecode_view, olive::ActiveSequence->frame_rate)
                             ));
      }
    }
  }
  tooltip_timer.stop();
}

void TimelineWidget::open_sequence_properties() {
  QList<Media*> sequence_items;
  QList<Media*> all_top_level_items;
  for (int i=0;i<olive::project_model.childCount();i++) {
    all_top_level_items.append(olive::project_model.child(i));
  }
  panel_project->get_all_media_from_table(all_top_level_items, sequence_items, MEDIA_TYPE_SEQUENCE); // find all sequences in project
  for (int i=0;i<sequence_items.size();i++) {
    if (sequence_items.at(i)->to_sequence() == olive::ActiveSequence) {
      NewSequenceDialog nsd(this, sequence_items.at(i));
      nsd.exec();
      return;
    }
  }
  QMessageBox::critical(this, tr("Error"), tr("Couldn't locate media wrapper for sequence."));
}

void TimelineWidget::show_clip_properties()
{
  // get list of selected clips
  QVector<Clip*> selected_clips = olive::ActiveSequence->SelectedClips();

  // if clips are selected, open the clip properties dialog
  if (!selected_clips.isEmpty()) {
    ClipPropertiesDialog cpd(this, selected_clips);
    cpd.exec();
  }
}

bool same_sign(int a, int b) {
  return (a < 0) == (b < 0);
}

void TimelineWidget::dragEnterEvent(QDragEnterEvent *event) {
  bool import_init = false;

  QVector<Media*> media_list;
  panel_timeline->importing_files = false;

  if (event->source() == panel_project->tree_view || event->source() == panel_project->icon_view) {
    QModelIndexList items = panel_project->get_current_selected();
    media_list.resize(items.size());
    for (int i=0;i<items.size();i++) {
      media_list[i] = panel_project->item_to_media(items.at(i));
    }
    import_init = true;
  }

  if (event->source() == panel_footage_viewer->viewer_widget) {
    SequencePtr proposed_seq = panel_footage_viewer->seq;
    if (proposed_seq != olive::ActiveSequence) { // don't allow nesting the same sequence
      media_list.append(panel_footage_viewer->media);
      import_init = true;
    }
  }

  if (olive::CurrentConfig.enable_drag_files_to_timeline && event->mimeData()->hasUrls()) {
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
      QStringList file_list;

      for (int i=0;i<urls.size();i++) {
        file_list.append(urls.at(i).toLocalFile());
      }

      panel_project->process_file_list(file_list);

      for (int i=0;i<panel_project->last_imported_media.size();i++) {
        // waits for media to have a duration
        // TODO would be much nicer if this was multithreaded
        FootagePtr f = panel_project->last_imported_media.at(i)->to_footage();
        f->ready_lock.lock();
        f->ready_lock.unlock();

        if (f->ready) {
          media_list.append(panel_project->last_imported_media.at(i));
        }
      }

      if (media_list.isEmpty()) {
        olive::UndoStack.undo();
      } else {
        import_init = true;
        panel_timeline->importing_files = true;
      }
    }
  }

  if (import_init) {
    event->acceptProposedAction();

    long entry_point;
    SequencePtr seq = olive::ActiveSequence;

    if (seq == nullptr) {
      // if no sequence, we're going to create a new one using the clips as a reference
      entry_point = 0;

      self_created_sequence = create_sequence_from_media(media_list);
      seq = self_created_sequence;
    } else {
      entry_point = panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x());
      panel_timeline->drag_frame_start = entry_point + getFrameFromScreenPoint(panel_timeline->zoom, 50);
      panel_timeline->drag_track_start = (bottom_align) ? -1 : 0;
    }

    panel_timeline->create_ghosts_from_media(seq, entry_point, media_list);

    panel_timeline->importing = true;
  }
}

void TimelineWidget::dragMoveEvent(QDragMoveEvent *event) {
  if (panel_timeline->importing) {
    event->acceptProposedAction();

    if (olive::ActiveSequence != nullptr) {
      QPoint pos = event->pos();
      panel_timeline->scroll_to_frame(panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x()));
      update_ghosts(pos, event->keyboardModifiers() & Qt::ShiftModifier);
      panel_timeline->move_insert = ((event->keyboardModifiers() & Qt::ControlModifier) && (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->importing));
      update_ui(false);
    }
  }
}

void TimelineWidget::wheelEvent(QWheelEvent *event) {

  // TODO: implement pixel scrolling

  bool shift = (event->modifiers() & Qt::ShiftModifier);
  bool ctrl = (event->modifiers() & Qt::ControlModifier);
  bool alt = (event->modifiers() & Qt::AltModifier);

  // "Scroll Zooms" false + Control up  : not zooming
  // "Scroll Zooms" false + Control down:     zooming
  // "Scroll Zooms" true  + Control up  :     zooming
  // "Scroll Zooms" true  + Control down: not zooming
  bool zooming = (olive::CurrentConfig.scroll_zooms != ctrl);

  // Allow shift for axis swap, but don't swap on zoom... Unless
  // we need to override Qt's axis swap via Alt
  bool swap_hv = ((shift != olive::CurrentConfig.invert_timeline_scroll_axes) &
                  !zooming) | (alt & !shift & zooming);

  int delta_h = swap_hv ? event->angleDelta().y() : event->angleDelta().x();
  int delta_v = swap_hv ? event->angleDelta().x() : event->angleDelta().y();

  if (zooming) {

    // Zoom only uses vertical scrolling, to avoid glitches on touchpads.
    // Don't do anything if not scrolling vertically.

    if (delta_v != 0) {

      // delta_v == 120 for one click of a mousewheel. Less or more for a
      // touchpad gesture. Calculate speed to compensate.
      // 120 = ratio of 4/3 (1.33), -120 = ratio of 3/4 (.75)

      double zoom_ratio = 1.0 + (abs(delta_v) * 0.33 / 120);

      if (delta_v < 0) {
        zoom_ratio = 1.0 / zoom_ratio;
      }

      panel_timeline->multiply_zoom(zoom_ratio);
    }

  } else {

    // Use the Timeline's main scrollbar for horizontal scrolling, and this
    // widget's scrollbar for vertical scrolling.

    QScrollBar* bar_v = scrollBar;
    QScrollBar* bar_h = panel_timeline->horizontalScrollBar;

    // Match the wheel events to the size of a step as per
    // https://doc.qt.io/qt-5/qwheelevent.html#angleDelta

    int step_h = bar_h->singleStep() * delta_h / -120;
    int step_v = bar_v->singleStep() * delta_v / -120;

    // Apply to appropriate scrollbars

    bar_h->setValue(bar_h->value() + step_h);
    bar_v->setValue(bar_v->value() + step_v);
  }
}

void TimelineWidget::dragLeaveEvent(QDragLeaveEvent* event) {
  event->accept();
  if (panel_timeline->importing) {
    if (panel_timeline->importing_files) {
      olive::UndoStack.undo();
    }
    panel_timeline->importing_files = false;
    panel_timeline->ghosts.clear();
    panel_timeline->importing = false;
    update_ui(false);
  }
  if (self_created_sequence != nullptr) {
    self_created_sequence.reset();
    self_created_sequence = nullptr;
  }
}

void delete_area_under_ghosts(ComboAction* ca) {
  // delete areas before adding
  QVector<Selection> delete_areas;
  for (int i=0;i<panel_timeline->ghosts.size();i++) {
    const Ghost& g = panel_timeline->ghosts.at(i);
    Selection sel;
    sel.in = g.in;
    sel.out = g.out;
    sel.track = g.track;
    delete_areas.append(sel);
  }
  panel_timeline->delete_areas_and_relink(ca, delete_areas, false);
}

void insert_clips(ComboAction* ca) {
  bool ripple_old_point = true;

  long earliest_old_point = LONG_MAX;
  long latest_old_point = LONG_MIN;

  long earliest_new_point = LONG_MAX;
  long latest_new_point = LONG_MIN;

  QVector<int> ignore_clips;
  for (int i=0;i<panel_timeline->ghosts.size();i++) {
    const Ghost& g = panel_timeline->ghosts.at(i);

    earliest_old_point = qMin(earliest_old_point, g.old_in);
    latest_old_point = qMax(latest_old_point, g.old_out);
    earliest_new_point = qMin(earliest_new_point, g.in);
    latest_new_point = qMax(latest_new_point, g.out);

    if (g.clip >= 0) {
      ignore_clips.append(g.clip);
    } else {
      // don't try to close old gap if importing
      ripple_old_point = false;
    }
  }

  panel_timeline->split_cache.clear();

  for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
    ClipPtr c = olive::ActiveSequence->clips.at(i);
    if (c != nullptr) {
      // don't split any clips that are moving
      bool found = false;
      for (int j=0;j<panel_timeline->ghosts.size();j++) {
        if (panel_timeline->ghosts.at(j).clip == i) {
          found = true;
          break;
        }
      }
      if (!found) {
        if (c->timeline_in() < earliest_new_point && c->timeline_out() > earliest_new_point) {
          panel_timeline->split_clip_and_relink(ca, i, earliest_new_point, true);
        }

        // determine if we should close the gap the old clips left behind
        if (ripple_old_point
            && !((c->timeline_in() < earliest_old_point && c->timeline_out() <= earliest_old_point) || (c->timeline_in() >= latest_old_point && c->timeline_out() > latest_old_point))
            && !ignore_clips.contains(i)) {
          ripple_old_point = false;
        }
      }
    }
  }

  long ripple_length = (latest_new_point - earliest_new_point);

  ripple_clips(ca, olive::ActiveSequence, earliest_new_point, ripple_length, ignore_clips);

  if (ripple_old_point) {
    // works for moving later clips earlier but not earlier to later
    long second_ripple_length = (earliest_old_point - latest_old_point);

    ripple_clips(ca, olive::ActiveSequence, latest_old_point, second_ripple_length, ignore_clips);

    if (earliest_old_point < earliest_new_point) {
      for (int i=0;i<panel_timeline->ghosts.size();i++) {
        Ghost& g = panel_timeline->ghosts[i];
        g.in += second_ripple_length;
        g.out += second_ripple_length;
      }
      for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
        Selection& s = olive::ActiveSequence->selections[i];
        s.in += second_ripple_length;
        s.out += second_ripple_length;
      }
    }
  }
}

void TimelineWidget::dropEvent(QDropEvent* event) {
  if (panel_timeline->importing && panel_timeline->ghosts.size() > 0) {
    event->acceptProposedAction();

    ComboAction* ca = new ComboAction();

    SequencePtr s = olive::ActiveSequence;

    // if we're dropping into nothing, create a new sequences based on the clip being dragged
    if (s == nullptr) {
      s = self_created_sequence;
      panel_project->create_sequence_internal(ca, self_created_sequence, true, nullptr);
      self_created_sequence = nullptr;
    } else if (event->keyboardModifiers() & Qt::ControlModifier) {
      insert_clips(ca);
    } else {
      delete_area_under_ghosts(ca);
    }

    panel_timeline->add_clips_from_ghosts(ca, s);

    olive::UndoStack.push(ca);

    setFocus();

    update_ui(true);
  }
}

void TimelineWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (olive::ActiveSequence != nullptr) {
    if (panel_timeline->tool == TIMELINE_TOOL_EDIT) {
      int clip_index = getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track);
      if (clip_index >= 0) {
        ClipPtr clip = olive::ActiveSequence->clips.at(clip_index);
        if (!(event->modifiers() & Qt::ShiftModifier)) olive::ActiveSequence->selections.clear();
        Selection s;
        s.in = clip->timeline_in();
        s.out = clip->timeline_out();
        s.track = clip->track();
        olive::ActiveSequence->selections.append(s);
        update_ui(false);
      }
    } else if (panel_timeline->tool == TIMELINE_TOOL_POINTER) {
      int clip_index = getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track);
      if (clip_index >= 0) {
        ClipPtr c = olive::ActiveSequence->clips.at(clip_index);
        if (c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE) {
          olive::Global->set_sequence(c->media()->to_sequence());
        }
      }
    }
  }
}

bool current_tool_shows_cursor() {
  return (panel_timeline->tool == TIMELINE_TOOL_EDIT || panel_timeline->tool == TIMELINE_TOOL_RAZOR || panel_timeline->creating);
}

void TimelineWidget::mousePressEvent(QMouseEvent *event) {
  if (olive::ActiveSequence != nullptr) {

    int effective_tool = panel_timeline->tool;

    // some user actions will override which tool we'll be using
    if (event->button() == Qt::MiddleButton) {
      effective_tool = TIMELINE_TOOL_HAND;
      panel_timeline->creating = false;
    } else if (event->button() == Qt::RightButton) {
      effective_tool = TIMELINE_TOOL_MENU;
      panel_timeline->creating = false;
    }

    // ensure cursor_frame and cursor_track are up to date
    mouseMoveEvent(event);

    // store current cursor positions
    panel_timeline->drag_x_start = event->pos().x();
    panel_timeline->drag_y_start = event->pos().y();

    // store current frame/tracks as the values to start dragging from
    panel_timeline->drag_frame_start = panel_timeline->cursor_frame;
    panel_timeline->drag_track_start = panel_timeline->cursor_track;

    // get the clip the user is currently hovering over, priority to trim_target set from mouseMoveEvent
    int hovered_clip = panel_timeline->trim_target == -1 ?
          getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track)
        : panel_timeline->trim_target;

    bool shift = (event->modifiers() & Qt::ShiftModifier);
    bool alt = (event->modifiers() & Qt::AltModifier);

    // Normal behavior is to reset selections to zero when clicking, but if Shift is held, we add selections
    // to the existing selections. `selection_offset` is the index to change selections from (and we don't touch
    // any prior to that)
    if (shift) {
      panel_timeline->selection_offset = olive::ActiveSequence->selections.size();
    } else {
      panel_timeline->selection_offset = 0;
    }

    // if the user is creating an object
    if (panel_timeline->creating) {
      int comp = 0;
      switch (panel_timeline->creating_object) {
      case ADD_OBJ_TITLE:
      case ADD_OBJ_SOLID:
      case ADD_OBJ_BARS:
        comp = -1;
        break;
      case ADD_OBJ_TONE:
      case ADD_OBJ_NOISE:
      case ADD_OBJ_AUDIO:
        comp = 1;
        break;
      }

      // if the track the user clicked is correct for the type of object we're adding

      if ((panel_timeline->drag_track_start < 0) == (comp < 0)) {
        Ghost g;
        g.in = g.old_in = g.out = g.old_out = panel_timeline->drag_frame_start;
        g.track = g.old_track = panel_timeline->drag_track_start;
        g.transition = nullptr;
        g.clip = -1;
        g.trim_type = TRIM_OUT;
        panel_timeline->ghosts.append(g);

        panel_timeline->moving_init = true;
        panel_timeline->moving_proc = true;
      }
    } else {

      // pass through tools to determine what action we'll be starting
      switch (effective_tool) {

      // many tools share pointer-esque behavior
      case TIMELINE_TOOL_POINTER:
      case TIMELINE_TOOL_RIPPLE:
      case TIMELINE_TOOL_SLIP:
      case TIMELINE_TOOL_ROLLING:
      case TIMELINE_TOOL_SLIDE:
      case TIMELINE_TOOL_MENU:
      {
        if (track_resizing && effective_tool != TIMELINE_TOOL_MENU) {

          // if the cursor is currently hovering over a track, init track resizing
          panel_timeline->moving_init = true;

        } else {

          // check if we're currently hovering over a clip or not
          if (hovered_clip >= 0) {
            Clip* clip = olive::ActiveSequence->clips.at(hovered_clip).get();

            if (olive::ActiveSequence->IsClipSelected(clip, true)) {

              if (shift) {

                // if the user clicks a selected clip while holding shift, deselect the clip
                panel_timeline->deselect_area(clip->timeline_in(), clip->timeline_out(), clip->track());

                // if the user isn't holding alt, also deselect all of its links as well
                if (!alt) {
                  for (int i=0;i<clip->linked.size();i++) {
                    ClipPtr link = olive::ActiveSequence->clips.at(clip->linked.at(i));
                    panel_timeline->deselect_area(link->timeline_in(), link->timeline_out(), link->track());
                  }
                }

              } else if (panel_timeline->tool == TIMELINE_TOOL_POINTER
                          && panel_timeline->transition_select != kTransitionNone) {

                // if the clip was selected by then the user clicked a transition, de-select the clip and its links
                // and select the transition only

                panel_timeline->deselect_area(clip->timeline_in(), clip->timeline_out(), clip->track());

                for (int i=0;i<clip->linked.size();i++) {
                  ClipPtr link = olive::ActiveSequence->clips.at(clip->linked.at(i));
                  panel_timeline->deselect_area(link->timeline_in(), link->timeline_out(), link->track());
                }

                Selection s;
                s.track = clip->track();

                // select the transition only
                if (panel_timeline->transition_select == kTransitionOpening && clip->opening_transition != nullptr) {
                  s.in = clip->timeline_in();

                  if (clip->opening_transition->secondary_clip != nullptr) {
                    s.in -= clip->opening_transition->get_true_length();
                  }

                  s.out = clip->timeline_in() + clip->opening_transition->get_true_length();
                } else if (panel_timeline->transition_select == kTransitionClosing && clip->closing_transition != nullptr) {
                  s.in = clip->timeline_out() - clip->closing_transition->get_true_length();
                  s.out = clip->timeline_out();

                  if (clip->closing_transition->secondary_clip != nullptr) {
                    s.out += clip->closing_transition->get_true_length();
                  }
                }
                olive::ActiveSequence->selections.append(s);
              }
            } else {

              // if the clip is not already selected

              // if shift is NOT down, we change clear all current selections
              if (!shift) {
                olive::ActiveSequence->selections.clear();
              }

              Selection s;

              s.in = clip->timeline_in();
              s.out = clip->timeline_out();
              s.track = clip->track();

              // if user is using the pointer tool, they may be trying to select a transition
              // check if the use is hovering over a transition
              if (panel_timeline->tool == TIMELINE_TOOL_POINTER) {
                if (panel_timeline->transition_select == kTransitionOpening) {
                  // move the selection to only select the transitoin
                  s.out = clip->timeline_in() + clip->opening_transition->get_true_length();

                  // if the transition is a "shared" transition, adjust the selection to select both sides
                  if (clip->opening_transition->secondary_clip != nullptr) {
                    s.in -= clip->opening_transition->get_true_length();
                  }
                } else if (panel_timeline->transition_select == kTransitionClosing) {
                  // move the selection to only select the transitoin
                  s.in = clip->timeline_out() - clip->closing_transition->get_true_length();

                  // if the transition is a "shared" transition, adjust the selection to select both sides
                  if (clip->closing_transition->secondary_clip != nullptr) {
                    s.out += clip->closing_transition->get_true_length();
                  }
                }
              }

              // add the selection to the array
              olive::ActiveSequence->selections.append(s);

              // if the config is set to also seek with selections, do so now
              if (olive::CurrentConfig.select_also_seeks) {
                panel_sequence_viewer->seek(clip->timeline_in());
              }

              // if alt is not down, select links (provided we're not selecting transitions)
              if (!alt && panel_timeline->transition_select == kTransitionNone) {

                for (int i=0;i<clip->linked.size();i++) {

                  Clip* link = olive::ActiveSequence->clips.at(clip->linked.at(i)).get();

                  // check if the clip is already selected
                  if (!olive::ActiveSequence->IsClipSelected(link, true)) {
                    Selection ss;
                    ss.in = link->timeline_in();
                    ss.out = link->timeline_out();
                    ss.track = link->track();
                    olive::ActiveSequence->selections.append(ss);
                  }

                }

              }
            }

            // authorize the starting of a move action if the mouse moves after this
            if (effective_tool != TIMELINE_TOOL_MENU) {
              panel_timeline->moving_init = true;
            }

          } else {

            // if the user did not click a clip at all, we start a rectangle selection

            if (!shift) {
              olive::ActiveSequence->selections.clear();
            }

            panel_timeline->rect_select_init = true;
          }

          // update everything
          update_ui(false);
        }
      }
        break;
      case TIMELINE_TOOL_HAND:

        // initiate moving with the hand tool
        panel_timeline->hand_moving = true;

        break;
      case TIMELINE_TOOL_EDIT:

        // if the config is set to seek with the edit tool, do so now
        if (olive::CurrentConfig.edit_tool_also_seeks) {
          panel_sequence_viewer->seek(panel_timeline->drag_frame_start);
        }

        // initiate selecting
        panel_timeline->selecting = true;

        break;
      case TIMELINE_TOOL_RAZOR:
      {

        // initiate razor tool
        panel_timeline->splitting = true;

        // add this track as a track being split by the razor
        panel_timeline->split_tracks.append(panel_timeline->drag_track_start);

        update_ui(false);
      }
        break;
      case TIMELINE_TOOL_TRANSITION:
      {

        // if there is a clip to run the transition tool on, initiate the transition tool
        if (panel_timeline->transition_tool_open_clip > -1
              || panel_timeline->transition_tool_close_clip > -1) {
          panel_timeline->transition_tool_init = true;
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

void VerifyTransitionsAfterCreating(ComboAction* ca, Clip* open, Clip* close, long transition_start, long transition_end) {
  // in case the user made the transition larger than the clips, we're going to delete everything under
  // the transition ghost and extend the clips to the transition's coordinates as necessary

  if (open == nullptr && close == nullptr) {
    qWarning() << "VerifyTransitionsAfterCreating() called with two null clips";
    return;
  }

  // determine whether this is a "shared" transition between to clips or not
  bool shared_transition = (open != nullptr && close != nullptr);

  int track = 0;

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
  Selection s;
  s.in = transition_start;
  s.out = transition_end;
  s.track = track;
  areas.append(s);
  panel_timeline->delete_areas_and_relink(ca, areas, false);

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



        clip_ref->move(ca,
                       new_in,
                       new_out,
                       clip_ref->clip_in() - (clip_ref->timeline_in() - new_in),
                       clip_ref->track());
      }
    }
  }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent *event) {
  QToolTip::hideText();
  if (olive::ActiveSequence != nullptr) {
    bool alt = (event->modifiers() & Qt::AltModifier);
    bool shift = (event->modifiers() & Qt::ShiftModifier);
    bool ctrl = (event->modifiers() & Qt::ControlModifier);

    if (event->button() == Qt::LeftButton) {
      ComboAction* ca = new ComboAction();
      bool push_undo = false;

      if (panel_timeline->creating) {
        if (panel_timeline->ghosts.size() > 0) {
          const Ghost& g = panel_timeline->ghosts.at(0);

          if (panel_timeline->creating_object == ADD_OBJ_AUDIO) {
            olive::MainWindow->statusBar()->clearMessage();
            panel_sequence_viewer->cue_recording(qMin(g.in, g.out), qMax(g.in, g.out), g.track);
            panel_timeline->creating = false;
          } else if (g.in != g.out) {
            ClipPtr c = std::make_shared<Clip>(olive::ActiveSequence);
            c->set_media(nullptr, 0);
            c->set_timeline_in(qMin(g.in, g.out));
            c->set_timeline_out(qMax(g.in, g.out));
            c->set_clip_in(0);
            c->set_color(192, 192, 64);
            c->set_track(g.track);

            if (ctrl) {
              insert_clips(ca);
            } else {
              Selection s;
              s.in = c->timeline_in();
              s.out = c->timeline_out();
              s.track = c->track();
              QVector<Selection> areas;
              areas.append(s);
              panel_timeline->delete_areas_and_relink(ca, areas, false);
            }

            QVector<ClipPtr> add;
            add.append(c);
            ca->append(new AddClipCommand(olive::ActiveSequence, add));

            if (c->track() < 0 && olive::CurrentConfig.add_default_effects_to_clips) {
              // default video effects (before custom effects)
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_TRANSFORM, EFFECT_TYPE_EFFECT)));
            }

            switch (panel_timeline->creating_object) {
            case ADD_OBJ_TITLE:
              c->set_name(tr("Title"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_TEXT, EFFECT_TYPE_EFFECT)));
              break;
            case ADD_OBJ_SOLID:
              c->set_name(tr("Solid Color"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_SOLID, EFFECT_TYPE_EFFECT)));
              break;
            case ADD_OBJ_BARS:
            {
              c->set_name(tr("Bars"));
              EffectPtr e = Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_SOLID, EFFECT_TYPE_EFFECT));
              e->row(0)->field(0)->set_combo_index(1);
              c->effects.append(e);
            }
              break;
            case ADD_OBJ_TONE:
              c->set_name(tr("Tone"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_TONE, EFFECT_TYPE_EFFECT)));
              break;
            case ADD_OBJ_NOISE:
              c->set_name(tr("Noise"));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_NOISE, EFFECT_TYPE_EFFECT)));
              break;
            }

            if (c->track() >= 0 && olive::CurrentConfig.add_default_effects_to_clips) {
              // default audio effects (after custom effects)
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_VOLUME, EFFECT_TYPE_EFFECT)));
              c->effects.append(Effect::Create(c.get(), Effect::GetInternalMeta(EFFECT_INTERNAL_PAN, EFFECT_TYPE_EFFECT)));
            }

            push_undo = true;

            if (!shift) {
              panel_timeline->creating = false;
            }
          }
        }
      } else if (panel_timeline->moving_proc) {

        // see if any clips actually moved, otherwise we don't need to do any processing
        // (perhaps this could be moved further up to cover more actions?)

        bool process_moving = false;

        for (int i=0;i<panel_timeline->ghosts.size();i++) {
          const Ghost& g = panel_timeline->ghosts.at(i);
          if (g.in != g.old_in
              || g.out != g.old_out
              || g.clip_in != g.old_clip_in
              || g.track != g.old_track) {
            process_moving = true;
            break;
          }
        }

        if (process_moving) {
          const Ghost& first_ghost = panel_timeline->ghosts.at(0);

          // start a ripple movement
          if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {

            // ripple_length becomes the length/number of frames we trimmed
            // ripple_point is the "axis" around which we move all the clips, any clips after it get moved
            long ripple_length;
            long ripple_point = LONG_MAX;

            if (panel_timeline->trim_type == TRIM_IN) {

              // it's assumed that all the ghosts rippled by the same length, so we just take the difference of the
              // first ghost here
              ripple_length = first_ghost.old_in - first_ghost.in;

              // for in trimming movements we also move the selections forward (unnecessary for out trimming since
              // the selected clips more or less stay in the same place)
              for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
                olive::ActiveSequence->selections[i].in += ripple_length;
                olive::ActiveSequence->selections[i].out += ripple_length;
              }
            } else {

              // use the out points for length if the user trimmed the out point
              ripple_length = first_ghost.old_out - panel_timeline->ghosts.at(0).out;

            }

            // build a list of "ignore clips" that won't get affected by ripple_clips() below
            QVector<int> ignore_clips;
            for (int i=0;i<panel_timeline->ghosts.size();i++) {
              const Ghost& g = panel_timeline->ghosts.at(i);

              // for the same reason that we pushed selections forward above, for in trimming,
              // we push the ghosts forward here
              if (panel_timeline->trim_type == TRIM_IN) {
                ignore_clips.append(g.clip);
                panel_timeline->ghosts[i].in += ripple_length;
                panel_timeline->ghosts[i].out += ripple_length;
              }

              // find the earliest ripple point
              long comp_point = (panel_timeline->trim_type == TRIM_IN) ? g.old_in : g.old_out;
              ripple_point = qMin(ripple_point, comp_point);
            }

            // if this was out trimming, flip the direction of the ripple
            if (panel_timeline->trim_type == TRIM_OUT) ripple_length = -ripple_length;

            // finally, ripple everything
            ripple_clips(ca, olive::ActiveSequence, ripple_point, ripple_length, ignore_clips);
          }

          if (panel_timeline->tool == TIMELINE_TOOL_POINTER
              && (event->modifiers() & Qt::AltModifier)
              && panel_timeline->trim_target == -1) {

            // if the user was holding alt (and not trimming), we duplicate clips rather than move them
            QVector<int> old_clips;
            QVector<ClipPtr> new_clips;
            QVector<Selection> delete_areas;
            for (int i=0;i<panel_timeline->ghosts.size();i++) {
              const Ghost& g = panel_timeline->ghosts.at(i);
              if (g.old_in != g.in || g.old_out != g.out || g.track != g.old_track || g.clip_in != g.old_clip_in) {

                // create copy of clip
                ClipPtr c(olive::ActiveSequence->clips.at(g.clip)->copy(olive::ActiveSequence));

                c->set_timeline_in(g.in);
                c->set_timeline_out(g.out);
                c->set_track(g.track);

                Selection s;
                s.in = g.in;
                s.out = g.out;
                s.track = g.track;
                delete_areas.append(s);

                old_clips.append(g.clip);
                new_clips.append(c);

              }
            }

            if (new_clips.size() > 0) {

              // delete anything under the new clips
              panel_timeline->delete_areas_and_relink(ca, delete_areas, false);

              // relink duplicated clips
              panel_timeline->relink_clips_using_ids(old_clips, new_clips);

              // add them
              ca->append(new AddClipCommand(olive::ActiveSequence, new_clips));

            }

          } else {

            // if we're not holding alt, this will just be a move

            // if the user is holding ctrl, perform an insert rather than an overwrite
            if (panel_timeline->tool == TIMELINE_TOOL_POINTER && ctrl) {

              insert_clips(ca);

            } else if (panel_timeline->tool == TIMELINE_TOOL_POINTER || panel_timeline->tool == TIMELINE_TOOL_SLIDE) {

              // if the user is not holding ctrl, we start standard clip movement

              // delete everything under the new clips
              QVector<Selection> delete_areas;
              for (int i=0;i<panel_timeline->ghosts.size();i++) {
                // step 1 - set clips that are moving to "undeletable" (to avoid step 2 deleting any part of them)
                const Ghost& g = panel_timeline->ghosts.at(i);

                // set clip to undeletable so it's unaffected by delete_areas_and_relink() below
                olive::ActiveSequence->clips.at(g.clip)->undeletable = true;

                // if the user was moving a transition make sure they're undeletable too
                if (g.transition != nullptr) {
                  g.transition->parent_clip->undeletable = true;
                  if (g.transition->secondary_clip != nullptr) {
                    g.transition->secondary_clip->undeletable = true;
                  }
                }

                // set area to delete
                Selection s;
                s.in = g.in;
                s.out = g.out;
                s.track = g.track;
                delete_areas.append(s);
              }

              panel_timeline->delete_areas_and_relink(ca, delete_areas, false);

              // clean up, i.e. make everything not undeletable again
              for (int i=0;i<panel_timeline->ghosts.size();i++) {
                const Ghost& g = panel_timeline->ghosts.at(i);
                olive::ActiveSequence->clips.at(g.clip)->undeletable = false;

                if (g.transition != nullptr) {
                  g.transition->parent_clip->undeletable = false;
                  if (g.transition->secondary_clip != nullptr) {
                    g.transition->secondary_clip->undeletable = false;
                  }
                }
              }
            }

            // finally, perform actual movement of clips
            for (int i=0;i<panel_timeline->ghosts.size();i++) {
              Ghost& g = panel_timeline->ghosts[i];

              Clip* c = olive::ActiveSequence->clips.at(g.clip).get();

              if (g.transition == nullptr) {

                // if this was a clip rather than a transition

                c->move(ca, (g.in - g.old_in), (g.out - g.old_out), (g.clip_in - g.old_clip_in), (g.track - g.old_track), false, true);

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
                  if (g.in != g.old_in && g.trim_type == TRIM_NONE) {
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

                    g.transition->parent_clip->move(ca, movement, timeline_out_movement, movement, 0, false, true);
                    g.transition->secondary_clip->move(ca, timeline_in_movement, movement, timeline_in_movement, 0, false, true);

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

                    c->move(ca, (g.in - g.old_in), timeline_out_movement, (g.clip_in - g.old_clip_in), 0, false, true);
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
                    c->move(ca, timeline_in_movement, (g.out - g.old_out), timeline_in_movement, 0, false, true);
                    clip_length += (g.out - g.old_out);
                  }

                  make_room_for_transition(ca, c, kTransitionClosing, g.in, g.out, false);

                }
              }
            }

            // time to verify the transitions of moved clips
            for (int i=0;i<panel_timeline->ghosts.size();i++) {
              const Ghost& g = panel_timeline->ghosts.at(i);

              // only applies to moving clips, transitions are verified above instead
              if (g.transition == nullptr) {
                ClipPtr c = olive::ActiveSequence->clips.at(g.clip);

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
                          || (t == kTransitionClosing && g.out != g.old_out)) {

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

                        for (int j=0;j<panel_timeline->ghosts.size();j++) {
                          const Ghost& other_clip_ghost = panel_timeline->ghosts.at(j);

                          if (olive::ActiveSequence->clips.at(other_clip_ghost.clip).get() == search_clip) {

                            // we found the other clip in the current ghosts/selections

                            // see if it's destination edge will be equal to this ghost's edge (in which case the
                            // transition doesn't need to change)
                            //
                            // also only do this if j is less than i, because it only needs to happen once and chances are
                            // the other clip already

                            bool edges_still_touch;
                            if (t == kTransitionOpening) {
                              edges_still_touch = (other_clip_ghost.out == g.in);
                            } else {
                              edges_still_touch = (other_clip_ghost.in == g.out);
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
          push_undo = true;
        }
      } else if (panel_timeline->selecting || panel_timeline->rect_select_proc) {
      } else if (panel_timeline->transition_tool_proc) {
        const Ghost& g = panel_timeline->ghosts.at(0);

        // if the transition is greater than 0 length (if it is 0, we make nothing)
        if (g.in != g.out) {

          // get transition coordinates on the timeline
          long transition_start = qMin(g.in, g.out);
          long transition_end = qMax(g.in, g.out);

          // get clip references from tool's cached data
          Clip* open = (panel_timeline->transition_tool_open_clip > -1)
              ? olive::ActiveSequence->clips.at(panel_timeline->transition_tool_open_clip).get()
              : nullptr;

          Clip* close = (panel_timeline->transition_tool_close_clip > -1)
              ? olive::ActiveSequence->clips.at(panel_timeline->transition_tool_close_clip).get()
              : nullptr;



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
                                              panel_timeline->transition_tool_meta,
                                              transition_length));

          push_undo = true;
        }
      } else if (panel_timeline->splitting) {
        bool split = false;
        for (int i=0;i<panel_timeline->split_tracks.size();i++) {
          int split_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->split_tracks.at(i));
          if (split_index > -1 && panel_timeline->split_clip_and_relink(ca, split_index, panel_timeline->drag_frame_start, !alt)) {
            split = true;
          }
        }
        if (split) {
          push_undo = true;
        }
        panel_timeline->split_cache.clear();
      }

      // remove duplicate selections
      panel_timeline->clean_up_selections(olive::ActiveSequence->selections);

      if (selection_command != nullptr) {
        selection_command->new_data = olive::ActiveSequence->selections;
        ca->append(selection_command);
        selection_command = nullptr;
        push_undo = true;
      }

      if (push_undo) {
        olive::UndoStack.push(ca);
      } else {
        delete ca;
      }

      // destroy all ghosts
      panel_timeline->ghosts.clear();

      // clear split tracks
      panel_timeline->split_tracks.clear();

      panel_timeline->selecting = false;
      panel_timeline->moving_proc = false;
      panel_timeline->moving_init = false;
      panel_timeline->splitting = false;
      panel_timeline->snapped = false;
      panel_timeline->rect_select_init = false;
      panel_timeline->rect_select_proc = false;
      panel_timeline->transition_tool_init = false;
      panel_timeline->transition_tool_proc = false;
      pre_clips.clear();
      post_clips.clear();

      update_ui(true);
    }
    panel_timeline->hand_moving = false;
  }
}

void TimelineWidget::init_ghosts() {
  for (int i=0;i<panel_timeline->ghosts.size();i++) {
    Ghost& g = panel_timeline->ghosts[i];
    ClipPtr c = olive::ActiveSequence->clips.at(g.clip);

    g.track = g.old_track = c->track();
    g.clip_in = g.old_clip_in = c->clip_in();

    if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {
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
  for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
    Selection& s = olive::ActiveSequence->selections[i];
    s.old_in = s.in;
    s.old_out = s.out;
    s.old_track = s.track;
  }
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

void TimelineWidget::update_ghosts(const QPoint& mouse_pos, bool lock_frame) {
  int effective_tool = panel_timeline->tool;
  if (panel_timeline->importing || panel_timeline->creating) effective_tool = TIMELINE_TOOL_POINTER;

  int mouse_track = getTrackFromScreenPoint(mouse_pos.y());
  long frame_diff = (lock_frame) ? 0 : panel_timeline->getTimelineFrameFromScreenPoint(mouse_pos.x()) - panel_timeline->drag_frame_start;
  int track_diff = ((effective_tool == TIMELINE_TOOL_SLIDE || panel_timeline->transition_select != kTransitionNone) && !panel_timeline->importing) ? 0 : mouse_track - panel_timeline->drag_track_start;
  long validator;
  long earliest_in_point = LONG_MAX;

  // first try to snap
  long fm;

  if (effective_tool != TIMELINE_TOOL_SLIP) {
    // slipping doesn't move the clips so we don't bother snapping for it
    for (int i=0;i<panel_timeline->ghosts.size();i++) {
      const Ghost& g = panel_timeline->ghosts.at(i);

      // snap ghost's in point
      if ((panel_timeline->tool != TIMELINE_TOOL_TRANSITION && panel_timeline->trim_target == -1)
          || g.trim_type == TRIM_IN
          || panel_timeline->transition_tool_open_clip > -1) {
        fm = g.old_in + frame_diff;
        if (panel_timeline->snap_to_timeline(&fm, true, true, true)) {
          frame_diff = fm - g.old_in;
          break;
        }
      }

      // snap ghost's out point
      if ((panel_timeline->tool != TIMELINE_TOOL_TRANSITION && panel_timeline->trim_target == -1)
          || g.trim_type == TRIM_OUT
          || panel_timeline->transition_tool_close_clip > -1) {
        fm = g.old_out + frame_diff;
        if (panel_timeline->snap_to_timeline(&fm, true, true, true)) {
          frame_diff = fm - g.old_out;
          break;
        }
      }

      // if the ghost is attached to a clip, snap its markers too
      if (panel_timeline->trim_target == -1 && g.clip >= 0 && panel_timeline->tool != TIMELINE_TOOL_TRANSITION) {
        ClipPtr c = olive::ActiveSequence->clips.at(g.clip);
        for (int j=0;j<c->get_markers().size();j++) {
          long marker_real_time = c->get_markers().at(j).frame + c->timeline_in() - c->clip_in();
          fm = marker_real_time + frame_diff;
          if (panel_timeline->snap_to_timeline(&fm, true, true, true)) {
            frame_diff = fm - marker_real_time;
            break;
          }
        }
      }
    }
  }

  bool clips_are_movable = (effective_tool == TIMELINE_TOOL_POINTER || effective_tool == TIMELINE_TOOL_SLIDE);

  // validate ghosts
  long temp_frame_diff = frame_diff; // cache to see if we change it (thus cancelling any snap)
  for (int i=0;i<panel_timeline->ghosts.size();i++) {
    const Ghost& g = panel_timeline->ghosts.at(i);
    Clip* c = nullptr;
    if (g.clip != -1) {
      c = olive::ActiveSequence->clips.at(g.clip).get();
    }

    const FootageStream* ms = nullptr;
    if (g.clip != -1 && c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_FOOTAGE) {
      ms = c->media_stream();
    }

    // validate ghosts for trimming
    if (panel_timeline->creating) {
      // i feel like we might need something here but we haven't so far?
    } else if (effective_tool == TIMELINE_TOOL_SLIP) {
      if ((c->media() != nullptr && c->media()->get_type() == MEDIA_TYPE_SEQUENCE)
          || (ms != nullptr && !ms->infinite_length)) {
        // prevent slip moving a clip below 0 clip_in
        validator = g.old_clip_in - frame_diff;
        if (validator < 0) frame_diff += validator;

        // prevent slip moving clip beyond media length
        validator += g.ghost_length;
        if (validator > g.media_length) frame_diff += validator - g.media_length;
      }
    } else if (g.trim_type != TRIM_NONE) {
      if (g.trim_type == TRIM_IN) {
        // prevent clip/transition length from being less than 1 frame long
        validator = g.ghost_length - frame_diff;
        if (validator < 1) frame_diff -= (1 - validator);

        // prevent timeline in from going below 0
        if (effective_tool != TIMELINE_TOOL_RIPPLE) {
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

        if (g.trim_type == TRIM_IN) {
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

        if (g.trim_type == TRIM_IN) {
          frame_diff += g.transition->get_true_length();
        } else {
          frame_diff -= g.transition->get_true_length();
        }
      }

      // ripple ops
      if (effective_tool == TIMELINE_TOOL_RIPPLE) {
        for (int j=0;j<post_clips.size();j++) {
          ClipPtr post = post_clips.at(j);

          // prevent any rippled clip from going below 0
          if (panel_timeline->trim_type == TRIM_IN) {
            validator = post->timeline_in() - frame_diff;
            if (validator < 0) frame_diff += validator;
          }

          // prevent any post-clips colliding with pre-clips
          for (int k=0;k<pre_clips.size();k++) {
            ClipPtr pre = pre_clips.at(k);
            if (pre != post && pre->track() == post->track()) {
              if (panel_timeline->trim_type == TRIM_IN) {
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

      // prevent clips from crossing tracks
      if (same_sign(g.old_track, panel_timeline->drag_track_start)) {
        while (!same_sign(g.old_track, g.old_track + track_diff)) {
          if (g.old_track < 0) {
            track_diff--;
          } else {
            track_diff++;
          }
        }
      }
    } else if (effective_tool == TIMELINE_TOOL_TRANSITION) {
      if (panel_timeline->transition_tool_open_clip == -1
          || panel_timeline->transition_tool_close_clip == -1) {
        validate_transitions(c, g.media_stream, frame_diff);
      } else {
        // open transition clip
        Clip* otc = olive::ActiveSequence->clips.at(panel_timeline->transition_tool_open_clip).get();

        // close transition clip
        Clip* ctc = olive::ActiveSequence->clips.at(panel_timeline->transition_tool_close_clip).get();

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
    panel_timeline->snapped = false;
  }

  // apply changes to ghosts
  for (int i=0;i<panel_timeline->ghosts.size();i++) {
    Ghost& g = panel_timeline->ghosts[i];

    if (effective_tool == TIMELINE_TOOL_SLIP) {
      g.clip_in = g.old_clip_in - frame_diff;
    } else if (g.trim_type != TRIM_NONE) {
      long ghost_diff = frame_diff;

      // prevent trimming clips from overlapping each other
      for (int j=0;j<panel_timeline->ghosts.size();j++) {
        const Ghost& comp = panel_timeline->ghosts.at(j);
        if (i != j && g.track == comp.track) {
          long validator;
          if (g.trim_type == TRIM_IN && comp.out < g.out) {
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
        if (g.trim_type == TRIM_IN) ghost_diff = -ghost_diff;
        g.in = g.old_in - ghost_diff;
        g.out = g.old_out + ghost_diff;
      } else if (g.trim_type == TRIM_IN) {
        g.in = g.old_in + ghost_diff;
        g.clip_in = g.old_clip_in + ghost_diff;
      } else {
        g.out = g.old_out + ghost_diff;
      }
    } else if (clips_are_movable) {
      g.track = g.old_track;
      g.in = g.old_in + frame_diff;
      g.out = g.old_out + frame_diff;

      if (g.transition != nullptr
          && g.transition == olive::ActiveSequence->clips.at(g.clip)->opening_transition) {
        g.clip_in = g.old_clip_in + frame_diff;
      }

      if (panel_timeline->importing) {
        if ((panel_timeline->video_ghosts && mouse_track < 0)
            || (panel_timeline->audio_ghosts && mouse_track >= 0)) {
          int abs_track_diff = abs(track_diff);
          if (g.old_track < 0) { // clip is video
            g.track -= abs_track_diff;
          } else { // clip is audio
            g.track += abs_track_diff;
          }
        }
      } else if (same_sign(g.old_track, panel_timeline->drag_track_start)) {
        g.track += track_diff;
      }
    } else if (effective_tool == TIMELINE_TOOL_TRANSITION) {
      if (panel_timeline->transition_tool_open_clip > -1
            && panel_timeline->transition_tool_close_clip > -1) {
        g.in = g.old_in - frame_diff;
        g.out = g.old_out + frame_diff;
      } else if (panel_timeline->transition_tool_open_clip == g.clip) {
        g.out = g.old_out + frame_diff;
      } else {
        g.in = g.old_in + frame_diff;
      }
    }

    earliest_in_point = qMin(earliest_in_point, g.in);
  }

  // apply changes to selections
  if (effective_tool != TIMELINE_TOOL_SLIP && !panel_timeline->importing && !panel_timeline->creating) {
    for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
      Selection& s = olive::ActiveSequence->selections[i];
      if (panel_timeline->trim_target > -1) {
        if (panel_timeline->trim_type == TRIM_IN) {
          s.in = s.old_in + frame_diff;
        } else {
          s.out = s.old_out + frame_diff;
        }
      } else if (clips_are_movable) {
        for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
          Selection& s = olive::ActiveSequence->selections[i];
          s.in = s.old_in + frame_diff;
          s.out = s.old_out + frame_diff;
          s.track = s.old_track;

          if (panel_timeline->importing) {
            int abs_track_diff = abs(track_diff);
            if (s.old_track < 0) {
              s.track -= abs_track_diff;
            } else {
              s.track += abs_track_diff;
            }
          } else {
            if (same_sign(s.track, panel_timeline->drag_track_start)) s.track += track_diff;
          }
        }
      }
    }
  }

  if (panel_timeline->importing) {
    QToolTip::showText(mapToGlobal(mouse_pos), frame_to_timecode(earliest_in_point, olive::CurrentConfig.timecode_view, olive::ActiveSequence->frame_rate));
  } else {
    QString tip = ((frame_diff < 0) ? "-" : "+") + frame_to_timecode(qAbs(frame_diff), olive::CurrentConfig.timecode_view, olive::ActiveSequence->frame_rate);
    if (panel_timeline->trim_target > -1) {
      // find which clip is being moved
      const Ghost* g = nullptr;
      for (int i=0;i<panel_timeline->ghosts.size();i++) {
        if (panel_timeline->ghosts.at(i).clip == panel_timeline->trim_target) {
          g = &panel_timeline->ghosts.at(i);
          break;
        }
      }

      if (g != nullptr) {
        tip += " " + tr("Duration:") + " ";
        long len = (g->old_out-g->old_in);
        if (panel_timeline->trim_type == TRIM_IN) {
          len -= frame_diff;
        } else {
          len += frame_diff;
        }
        tip += frame_to_timecode(len, olive::CurrentConfig.timecode_view, olive::ActiveSequence->frame_rate);
      }
    }
    QToolTip::showText(mapToGlobal(mouse_pos), tip);
  }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent *event) {
  // interrupt any potential tooltip about to show
  tooltip_timer.stop();

  if (olive::ActiveSequence != nullptr) {
    bool alt = (event->modifiers() & Qt::AltModifier);

    // store current frame/track corresponding to the cursor
    panel_timeline->cursor_frame = panel_timeline->getTimelineFrameFromScreenPoint(event->pos().x());
    panel_timeline->cursor_track = getTrackFromScreenPoint(event->pos().y());

    // if holding the mouse button down, let's scroll to that location
    if (event->buttons() != 0) {
      panel_timeline->scroll_to_frame(panel_timeline->cursor_frame);
    }

    // determine if the action should be "inserting" rather than "overwriting"
    // Default behavior is to replace/overwrite clips under any clips we're dropping over them. Inserting will
    // split and move existing clips at the drop point to make space for the drop
    panel_timeline->move_insert = ((event->modifiers() & Qt::ControlModifier)
                                   && (panel_timeline->tool == TIMELINE_TOOL_POINTER
                                       || panel_timeline->importing
                                       || panel_timeline->creating));

    // if we're not currently resizing already, default track resizing to false (we'll set it to true later if
    // the user is still hovering over a track line)
    if (!panel_timeline->moving_init) {
      track_resizing = false;
    }

    // if the current tool uses an on-screen visible cursor, we snap the cursor to the timeline
    if (current_tool_shows_cursor()) {
      panel_timeline->snap_to_timeline(&panel_timeline->cursor_frame,

                                       // only snap to the playhead if the edit tool doesn't force the playhead to
                                       // follow it (or if we're not selecting since that means the playhead is
                                       // static at the moment)
                                       !olive::CurrentConfig.edit_tool_also_seeks || !panel_timeline->selecting,

                                       true,
                                       true);
    }

    if (panel_timeline->selecting) {

      // get number of selections based on tracks in selection area
      int selection_tool_count = 1 + qMax(panel_timeline->cursor_track, panel_timeline->drag_track_start) - qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);

      // add count to selection offset for the total number of selection objects
      // (offset is usually 0, unless the user is holding shift in which case we add to existing selections)
      int selection_count = selection_tool_count + panel_timeline->selection_offset;

      // resize selection object array to new count
      if (olive::ActiveSequence->selections.size() != selection_count) {
        olive::ActiveSequence->selections.resize(selection_count);
      }

      // loop through tracks in selection area and adjust them accordingly
      int minimum_selection_track = qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);
      int maximum_selection_track = qMax(panel_timeline->cursor_track, panel_timeline->drag_track_start);
      long selection_in = qMin(panel_timeline->drag_frame_start, panel_timeline->cursor_frame);
      long selection_out = qMax(panel_timeline->drag_frame_start, panel_timeline->cursor_frame);
      for (int i=panel_timeline->selection_offset;i<selection_count;i++) {
        Selection& s = olive::ActiveSequence->selections[i];
        s.track = minimum_selection_track + i - panel_timeline->selection_offset;
        s.in = selection_in;
        s.out = selection_out;
      }

      // If the config is set to select links as well with the edit tool
      if (olive::CurrentConfig.edit_tool_selects_links) {

        // find which clips are selected
        for (int j=0;j<olive::ActiveSequence->clips.size();j++) {

          Clip* c = olive::ActiveSequence->clips.at(j).get();

          if (c != nullptr && olive::ActiveSequence->IsClipSelected(c, false)) {

            // loop through linked clips
            for (int k=0;k<c->linked.size();k++) {

              ClipPtr link = olive::ActiveSequence->clips.at(c->linked.at(k));

              // see if one of the selections is already covering this track
              if (!(link->track() >= minimum_selection_track
                    && link->track() <= maximum_selection_track)) {

                // clip is not in selectin area, time to select it
                Selection link_sel;
                link_sel.in = selection_in;
                link_sel.out = selection_out;
                link_sel.track = link->track();
                olive::ActiveSequence->selections.append(link_sel);

              }

            }

          }
        }
      }

      // if the config is set to seek with the edit too, do so now
      if (olive::CurrentConfig.edit_tool_also_seeks) {
        panel_sequence_viewer->seek(qMin(panel_timeline->drag_frame_start, panel_timeline->cursor_frame));
      } else {
        // if not, repaint (seeking will trigger a repaint)
        panel_timeline->repaint_timeline();
      }

    } else if (panel_timeline->hand_moving) {

      // if we're hand moving, we'll be adding values directly to the scrollbars

      // the scrollbars trigger repaints when they scroll, which is unnecessary here so we block them
      panel_timeline->block_repaints = true;
      panel_timeline->horizontalScrollBar->setValue(panel_timeline->horizontalScrollBar->value() + panel_timeline->drag_x_start - event->pos().x());
      scrollBar->setValue(scrollBar->value() + panel_timeline->drag_y_start - event->pos().y());
      panel_timeline->block_repaints = false;

      // finally repaint
      panel_timeline->repaint_timeline();

      // store current cursor position for next hand move event
      panel_timeline->drag_x_start = event->pos().x();
      panel_timeline->drag_y_start = event->pos().y();

    } else if (panel_timeline->moving_init) {

      if (track_resizing) {

        // get cursor movement
        int diff = (event->pos().y() - panel_timeline->drag_y_start);

        // add it to the current track height
        int new_height = panel_timeline->GetTrackHeight(track_target);
        if (bottom_align) {
          new_height -= diff;
        } else {
          new_height += diff;
        }

        // limit track height to track minimum height constant
        new_height = qMax(new_height, olive::timeline::kTrackMinHeight);

        // set the track height
        panel_timeline->SetTrackHeight(track_target, new_height);

        // store current cursor position for next track resize event
        panel_timeline->drag_y_start = event->pos().y();

        update();
      } else if (panel_timeline->moving_proc) {

        // we're currently dragging ghosts
        update_ghosts(event->pos(), event->modifiers() & Qt::ShiftModifier);

      } else {

        // Prepare to start moving clips in some capacity. We create Ghost objects to store movement data before we
        // actually apply it to the clips (in mouseReleaseEvent)

        // loop through clips for any currently selected
        for (int i=0;i<olive::ActiveSequence->clips.size();i++) {

          Clip* c = olive::ActiveSequence->clips.at(i).get();

          if (c != nullptr) {
            Ghost g;
            g.transition = nullptr;

            // check if whole clip is added
            bool add = false;

            // check if a transition is selected (prioritize transition selection)
            // (only the pointer tool supports moving transitions)
            if (panel_timeline->tool == TIMELINE_TOOL_POINTER
                && (c->opening_transition != nullptr || c->closing_transition != nullptr)) {

              // check if any selections contain a whole transition
              for (int j=0;j<olive::ActiveSequence->selections.size();j++) {

                const Selection& s = olive::ActiveSequence->selections.at(j);

                if (s.track == c->track()) {
                  if (selection_contains_transition(s, c, kTransitionOpening)) {

                    g.transition = c->opening_transition;
                    add = true;
                    break;

                  } else if (selection_contains_transition(s, c, kTransitionClosing)) {

                    g.transition = c->closing_transition;
                    add = true;
                    break;

                  }
                }

              }

            }

            // if a transition isn't selected, check if the whole clip is
            if (!add) {
              add = olive::ActiveSequence->IsClipSelected(c, true);
            }

            if (add) {

              if (g.transition != nullptr) {

                // transition may be a dual transition, check if it's already been added elsewhere
                for (int j=0;j<panel_timeline->ghosts.size();j++) {
                  if (panel_timeline->ghosts.at(j).transition == g.transition) {
                    add = false;
                    break;
                  }
                }

              }

              if (add) {
                g.clip = i;
                g.trim_type = panel_timeline->trim_type;
                panel_timeline->ghosts.append(g);
              }

            }
          }
        }

        if (panel_timeline->tool == TIMELINE_TOOL_SLIDE) {

          // for the slide tool, we add the surrounding clips as ghosts that are getting trimmed the opposite way

          // store original array size since we'll be adding to it
          int ghost_arr_size = panel_timeline->ghosts.size();

          // loop through clips for any that are "touching" the selected clips
          for (int j=0;j<olive::ActiveSequence->clips.size();j++) {

            ClipPtr c = olive::ActiveSequence->clips.at(j);
            if (c != nullptr) {

              for (int i=0;i<ghost_arr_size;i++) {

                Ghost& g = panel_timeline->ghosts[i];
                g.trim_type = TRIM_NONE; // the selected clips will be moving, not trimming

                ClipPtr ghost_clip = olive::ActiveSequence->clips.at(g.clip);

                if (c->track() == ghost_clip->track()) {

                  // see if this clip is currently selected, if so we won't add it as a "touching" clip
                  bool found = false;
                  for (int k=0;k<ghost_arr_size;k++) {
                    if (panel_timeline->ghosts.at(k).clip == j) {
                      found = true;
                      break;
                    }
                  }

                  if (!found) { // the clip is not currently selected

                    // check if this clip is indeed touching
                    bool is_in = (c->timeline_in() == ghost_clip->timeline_out());
                    if (is_in || c->timeline_out() == ghost_clip->timeline_in()) {
                      Ghost gh;
                      gh.transition = nullptr;
                      gh.clip = j;
                      gh.trim_type = is_in ? TRIM_IN : TRIM_OUT;
                      panel_timeline->ghosts.append(gh);
                    }
                  }
                }
              }
            }
          }
        }

        // set up ghost defaults
        init_ghosts();

        // if the ripple tool is selected, prepare to ripple
        if (panel_timeline->tool == TIMELINE_TOOL_RIPPLE) {

          long axis = LONG_MAX;

          // find the earliest point within the selected clips which is the point we'll ripple around
          // also store the currently selected clips so we don't have to do it later
          QVector<ClipPtr> ghost_clips;
          ghost_clips.resize(panel_timeline->ghosts.size());

          for (int i=0;i<panel_timeline->ghosts.size();i++) {
            ClipPtr c = olive::ActiveSequence->clips.at(panel_timeline->ghosts.at(i).clip);
            if (panel_timeline->trim_type == TRIM_IN) {
              axis = qMin(axis, c->timeline_in());
            } else {
              axis = qMin(axis, c->timeline_out());
            }

            // store clip reference
            ghost_clips[i] = c;
          }

          // loop through clips and cache which are earlier than the axis and which after after
          for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
            ClipPtr c = olive::ActiveSequence->clips.at(i);
            if (c != nullptr && !ghost_clips.contains(c)) {
              bool clip_is_post = (c->timeline_in() >= axis);

              // construct the list of pre and post clips
              QVector<ClipPtr>& clip_list = (clip_is_post) ? post_clips : pre_clips;

              // check if there's already a clip in this list on this track, and if this clip is closer or not
              bool found = false;
              for (int j=0;j<clip_list.size();j++) {

                ClipPtr compare = clip_list.at(j);

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
        selection_command = new SetSelectionsCommand(olive::ActiveSequence);
        selection_command->old_data = olive::ActiveSequence->selections;

        // ready to start moving clips
        panel_timeline->moving_proc = true;
      }

      update_ui(false);

    } else if (panel_timeline->splitting) {

      // get the range of tracks currently dragged
      int track_start = qMin(panel_timeline->cursor_track, panel_timeline->drag_track_start);
      int track_end = qMax(panel_timeline->cursor_track, panel_timeline->drag_track_start);
      int track_size = 1 + track_end - track_start;

      // set tracks to be split
      panel_timeline->split_tracks.resize(track_size);
      for (int i=0;i<track_size;i++) {
        panel_timeline->split_tracks[i] = track_start + i;
      }

      // if alt isn't being held, also add the tracks of the clip's links
      if (!alt) {
        for (int i=0;i<track_size;i++) {

          // make sure there's a clip in this track
          int clip_index = getClipIndexFromCoords(panel_timeline->drag_frame_start, panel_timeline->split_tracks[i]);

          if (clip_index > -1) {
            ClipPtr clip = olive::ActiveSequence->clips.at(clip_index);
            for (int j=0;j<clip->linked.size();j++) {

              ClipPtr link = olive::ActiveSequence->clips.at(clip->linked.at(j));

              // if this clip isn't already in the list of tracks to split
              if (link->track() < track_start || link->track() > track_end) {
                panel_timeline->split_tracks.append(link->track());
              }

            }
          }
        }
      }

      update_ui(false);

    } else if (panel_timeline->rect_select_init) {

      // set if the user started dragging at point where there was no clip

      if (panel_timeline->rect_select_proc) {

        // we're currently rectangle selecting

        // set the right/bottom coords to the current mouse position
        // (left/top were set to the starting drag position earlier)
        panel_timeline->rect_select_rect.setRight(event->pos().x());

        if (bottom_align) {
          panel_timeline->rect_select_rect.setBottom(event->pos().y() - height());
        } else {
          panel_timeline->rect_select_rect.setBottom(event->pos().y());
        }

        long frame_min = qMin(panel_timeline->drag_frame_start, panel_timeline->cursor_frame);
        long frame_max = qMax(panel_timeline->drag_frame_start, panel_timeline->cursor_frame);

        int track_min = qMin(panel_timeline->drag_track_start, panel_timeline->cursor_track);
        int track_max = qMax(panel_timeline->drag_track_start, panel_timeline->cursor_track);

        // determine which clips are in this rectangular selection
        QVector<ClipPtr> selected_clips;
        for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
          ClipPtr clip = olive::ActiveSequence->clips.at(i);
          if (clip != nullptr &&
              clip->track() >= track_min &&
              clip->track() <= track_max &&
              !(clip->timeline_in() < frame_min && clip->timeline_out() < frame_min) &&
              !(clip->timeline_in() > frame_max && clip->timeline_out() > frame_max)) {

            // create a group of the clip (and its links if alt is not pressed)
            QVector<ClipPtr> session_clips;
            session_clips.append(clip);

            if (!alt) {
              for (int j=0;j<clip->linked.size();j++) {
                session_clips.append(olive::ActiveSequence->clips.at(clip->linked.at(j)));
              }
            }

            // for each of these clips, see if clip has already been added -
            // this can easily happen due to adding linked clips
            for (int j=0;j<session_clips.size();j++) {
              bool found = false;

              ClipPtr c = session_clips.at(j);
              for (int k=0;k<selected_clips.size();k++) {
                if (selected_clips.at(k) == c) {
                  found = true;
                  break;
                }
              }

              // if the clip isn't already in the selection add it
              if (!found) {
                selected_clips.append(c);
              }
            }
          }
        }

        // add each of the selected clips to the main sequence's selections
        olive::ActiveSequence->selections.resize(selected_clips.size() + panel_timeline->selection_offset);
        for (int i=0;i<selected_clips.size();i++) {
          Selection& s = olive::ActiveSequence->selections[i+panel_timeline->selection_offset];
          ClipPtr clip = selected_clips.at(i);
          s.old_in = s.in = clip->timeline_in();
          s.old_out = s.out = clip->timeline_out();
          s.old_track = s.track = clip->track();
        }

        panel_timeline->repaint_timeline();
      } else {

        // set up rectangle selecting
        panel_timeline->rect_select_rect.setX(event->pos().x());

        if (bottom_align) {
          // bottom aligned widgets start with 0 at the bottom and go down to a negative number
          panel_timeline->rect_select_rect.setY(event->pos().y() - height());
        } else {
          panel_timeline->rect_select_rect.setY(event->pos().y());
        }

        panel_timeline->rect_select_rect.setWidth(0);
        panel_timeline->rect_select_rect.setHeight(0);

        panel_timeline->rect_select_proc = true;

      }
    } else if (current_tool_shows_cursor()) {

      // we're not currently performing an action (click is not pressed), but redraw because we have an on-screen cursor
      panel_timeline->repaint_timeline();

    } else if (panel_timeline->tool == TIMELINE_TOOL_POINTER ||
               panel_timeline->tool == TIMELINE_TOOL_RIPPLE ||
               panel_timeline->tool == TIMELINE_TOOL_ROLLING) {

      // hide any tooltip that may be currently showing
      QToolTip::hideText();

      // cache cursor position
      QPoint pos = event->pos();

      //
      // check to see if the cursor is on a clip edge
      //

      // threshold around a trim point that the cursor can be within and still considered "trimming"
      int lim = 5;
      long mouse_frame_lower = panel_timeline->getTimelineFrameFromScreenPoint(pos.x()-lim)-1;
      long mouse_frame_upper = panel_timeline->getTimelineFrameFromScreenPoint(pos.x()+lim)+1;

      // used to determine whether we the cursor found a trim point or not
      bool found = false;

      // used to determine whether the cursor is within the rect of a clip
      bool cursor_contains_clip = false;

      // used to determine how close the cursor is to a trim point
      // (and more specifically, whether another point is closer or not)
      int closeness = INT_MAX;

      // while we loop through the clips, we cache the maximum/minimum tracks in this sequence
      int min_track = INT_MAX;
      int max_track = INT_MIN;

      // we default to selecting no transition, but set this accordingly if the cursor is on a transition
      panel_timeline->transition_select = kTransitionNone;

      // we also default to no trimming which may be changed later in this function
      panel_timeline->trim_type = TRIM_NONE;

      // set currently trimming clip to -1 (aka null)
      panel_timeline->trim_target = -1;

      // loop through current clips in the sequence
      for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
        ClipPtr c = olive::ActiveSequence->clips.at(i);
        if (c != nullptr) {

          // cache track range
          min_track = qMin(min_track, c->track());
          max_track = qMax(max_track, c->track());

          // if this clip is on the same track the mouse is
          if (c->track() == panel_timeline->cursor_track) {

            // if this cursor is inside the boundaries of this clip (hovering over the clip)
            if (panel_timeline->cursor_frame >= c->timeline_in() &&
                panel_timeline->cursor_frame <= c->timeline_out()) {

              // acknowledge that we are hovering over a clip
              cursor_contains_clip = true;

              // start a timer to show a tooltip about this clip
              tooltip_timer.start();
              tooltip_clip = i;

              // check if the cursor is specifically hovering over one of the clip's transitions
              if (c->opening_transition != nullptr
                  && panel_timeline->cursor_frame <= c->timeline_in() + c->opening_transition->get_true_length()) {

                panel_timeline->transition_select = kTransitionOpening;

              } else if (c->closing_transition != nullptr
                         && panel_timeline->cursor_frame >= c->timeline_out() - c->closing_transition->get_true_length()) {

                panel_timeline->transition_select = kTransitionClosing;

              }
            }

            // is the cursor hovering around the clip's IN point?
            if (c->timeline_in() > mouse_frame_lower && c->timeline_in() < mouse_frame_upper) {

              // test how close this IN point is to the cursor
              int nc = qAbs(c->timeline_in() + 1 - panel_timeline->cursor_frame);

              // and test whether it's closer than the last in/out point we found
              if (nc < closeness) {

                // if so, this is the point we'll make active for now (unless we find a closer one later)
                panel_timeline->trim_target = i;
                panel_timeline->trim_type = TRIM_IN;
                closeness = nc;
                found = true;

              }
            }

            // is the cursor hovering around the clip's OUT point?
            if (c->timeline_out() > mouse_frame_lower && c->timeline_out() < mouse_frame_upper) {

              // test how close this OUT point is to the cursor
              int nc = qAbs(c->timeline_out() - 1 - panel_timeline->cursor_frame);

              // and test whether it's closer than the last in/out point we found
              if (nc < closeness) {

                // if so, this is the point we'll make active for now (unless we find a closer one later)
                panel_timeline->trim_target = i;
                panel_timeline->trim_type = TRIM_OUT;
                closeness = nc;
                found = true;

              }
            }

            // the pointer can be used to resize/trim transitions, here we test if the
            // cursor is within the trim point of one of the clip's transitions
            if (panel_timeline->tool == TIMELINE_TOOL_POINTER) {

              // if the clip has an opening transition
              if (c->opening_transition != nullptr) {

                // cache the timeline frame where the transition ends
                long transition_point = c->timeline_in() + c->opening_transition->get_true_length();

                // check if the cursor is hovering around it (within the threshold)
                if (transition_point > mouse_frame_lower && transition_point < mouse_frame_upper) {

                  // similar to above, test how close it is and if it's closer, make this active
                  int nc = qAbs(transition_point - 1 - panel_timeline->cursor_frame);
                  if (nc < closeness) {
                    panel_timeline->trim_target = i;
                    panel_timeline->trim_type = TRIM_OUT;
                    panel_timeline->transition_select = kTransitionOpening;
                    closeness = nc;
                    found = true;
                  }
                }
              }

              // if the clip has a closing transition
              if (c->closing_transition != nullptr) {

                // cache the timeline frame where the transition starts
                long transition_point = c->timeline_out() - c->closing_transition->get_true_length();

                // check if the cursor is hovering around it (within the threshold)
                if (transition_point > mouse_frame_lower && transition_point < mouse_frame_upper) {

                  // similar to above, test how close it is and if it's closer, make this active
                  int nc = qAbs(transition_point + 1 - panel_timeline->cursor_frame);
                  if (nc < closeness) {
                    panel_timeline->trim_target = i;
                    panel_timeline->trim_type = TRIM_IN;
                    panel_timeline->transition_select = kTransitionClosing;
                    closeness = nc;
                    found = true;
                  }
                }
              }
            }
          }
        }
      }

      // if the cursor is indeed on a clip edge, we set the cursor accordingly
      if (found) {

        if (panel_timeline->trim_type == TRIM_IN) { // if we're trimming an IN point
          setCursor(panel_timeline->tool == TIMELINE_TOOL_RIPPLE ? olive::Cursor_LeftRipple : olive::Cursor_LeftTrim);
        } else { // if we're trimming an OUT point
          setCursor(panel_timeline->tool == TIMELINE_TOOL_RIPPLE ? olive::Cursor_RightRipple : olive::Cursor_RightTrim);
        }

      } else {
        // we didn't find a trim target, so we must be doing something else
        // (e.g. dragging a clip or resizing the track heights)

        unsetCursor();

        // check to see if we're resizing a track height
        int test_range = 5;
        int mouse_pos = event->pos().y();
        int hover_track = getTrackFromScreenPoint(mouse_pos);
        int track_y_edge = getScreenPointFromTrack(hover_track);

        if (!bottom_align) {
          track_y_edge += panel_timeline->GetTrackHeight(hover_track);
        }

        if (mouse_pos > track_y_edge - test_range
            && mouse_pos < track_y_edge + test_range) {
          if (cursor_contains_clip
              || (olive::CurrentConfig.show_track_lines
                  && panel_timeline->cursor_track >= min_track
                  && panel_timeline->cursor_track <= max_track)) {
            track_resizing = true;
            track_target = hover_track;
            setCursor(Qt::SizeVerCursor);
          }
        }
      }
    } else if (panel_timeline->tool == TIMELINE_TOOL_SLIP) {

      // we're not currently performing any slipping, all we do here is set the cursor if mouse is hovering over a
      // cursor
      if (getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track) > -1) {
        setCursor(olive::Cursor_Slip);
      } else {
        unsetCursor();
      }

    } else if (panel_timeline->tool == TIMELINE_TOOL_TRANSITION) {

      if (panel_timeline->transition_tool_init) {

        // the transition tool has started

        if (panel_timeline->transition_tool_proc) {

          // ghosts have been set up, so just run update
          update_ghosts(event->pos(), event->modifiers() & Qt::ShiftModifier);

        } else {

          // transition tool is being used but ghosts haven't been set up yet, set them up now
          int primary_type = kTransitionOpening;
          int primary = panel_timeline->transition_tool_open_clip;
          if (primary == -1) {
            primary_type = kTransitionClosing;
            primary = panel_timeline->transition_tool_close_clip;
          }

          ClipPtr c = olive::ActiveSequence->clips.at(primary);

          Ghost g;

          g.in = g.old_in = g.out = g.old_out = (primary_type == kTransitionOpening) ?
                                                  c->timeline_in()
                                                : c->timeline_out();

          g.track = c->track();
          g.clip = primary;
          g.media_stream = primary_type;
          g.trim_type = TRIM_NONE;

          panel_timeline->ghosts.append(g);

          panel_timeline->transition_tool_proc = true;

        }

      } else {

        // transition tool has been selected but is not yet active, so we show screen feedback to the user on
        // possible transitions

        int mouse_clip = getClipIndexFromCoords(panel_timeline->cursor_frame, panel_timeline->cursor_track);

        // set default transition tool references to no clip
        panel_timeline->transition_tool_open_clip = -1;
        panel_timeline->transition_tool_close_clip = -1;

        if (mouse_clip > -1) {

          // cursor is hovering over a clip

          ClipPtr c = olive::ActiveSequence->clips.at(mouse_clip);

          // check if the clip and transition are both the same sign (meaning video/audio are the same)
          if (same_sign(c->track(), panel_timeline->transition_tool_side)) {

            // the range within which the transition tool will assume the user wants to make a shared transition
            // between two clips rather than just one transition on one clip
            long between_range = getFrameFromScreenPoint(panel_timeline->zoom, TRANSITION_BETWEEN_RANGE) + 1;

            // set whether the transition is opening or closing based on whether the cursor is on the left half
            // or right half of the clip
            if (panel_timeline->cursor_frame > (c->timeline_in() + (c->length()/2))) {
              panel_timeline->transition_tool_close_clip = mouse_clip;

              // if the cursor is within this range, set the post_clip to be the next clip touching
              //
              // getClipIndexFromCoords() will automatically set to -1 if there's no clip there which means the
              // end result will be the same as not setting a clip here at all
              if (panel_timeline->cursor_frame > c->timeline_out() - between_range) {
                panel_timeline->transition_tool_open_clip = getClipIndexFromCoords(c->timeline_out()+1, c->track());
              }
            } else {
              panel_timeline->transition_tool_open_clip = mouse_clip;

              if (panel_timeline->cursor_frame < c->timeline_in() + between_range) {
                panel_timeline->transition_tool_close_clip = getClipIndexFromCoords(c->timeline_in()-1, c->track());
              }
            }

          }
        }
      }

      panel_timeline->repaint_timeline();
    }
  }
}

void TimelineWidget::leaveEvent(QEvent*) {
  tooltip_timer.stop();
}

void draw_waveform(ClipPtr clip, const FootageStream* ms, long media_length, QPainter *p, const QRect& clip_rect, int waveform_start, int waveform_limit, double zoom) {
  // audio channels multiplied by the number of bytes in a 16-bit audio sample
  int divider = ms->audio_channels*2;

  int channel_height = clip_rect.height()/ms->audio_channels;

  int last_waveform_index = -1;

  for (int i=waveform_start;i<waveform_limit;i++) {
    int waveform_index = qFloor((((clip->clip_in() + (double(i)/zoom))/media_length) * ms->audio_preview.size())/divider)*divider;

    if (clip->reversed()) {
      waveform_index = ms->audio_preview.size() - waveform_index - (ms->audio_channels * 2);
    }

    if (last_waveform_index < 0) last_waveform_index = waveform_index;

    for (int j=0;j<ms->audio_channels;j++) {
      int mid = (olive::CurrentConfig.rectified_waveforms) ? clip_rect.top()+channel_height*(j+1) : clip_rect.top()+channel_height*j+(channel_height/2);

      int offset_range_start = last_waveform_index+(j*2);
      int offset_range_end = waveform_index+(j*2);
      int offset_range_min = qMin(offset_range_start, offset_range_end);
      int offset_range_max = qMax(offset_range_start, offset_range_end);

      qint8 min = qint8(qRound(double(ms->audio_preview.at(offset_range_min)) / 128.0 * (channel_height/2)));
      qint8 max = qint8(qRound(double(ms->audio_preview.at(offset_range_min+1)) / 128.0 * (channel_height/2)));

      if ((offset_range_max + 1) < ms->audio_preview.size()) {

        // for waveform drawings, we get the maximum below 0 and maximum above 0 for this waveform range
        for (int k=offset_range_min+2;k<=offset_range_max;k+=2) {
          min = qMin(min, qint8(qRound(double(ms->audio_preview.at(k)) / 128.0 * (channel_height/2))));
          max = qMax(max, qint8(qRound(double(ms->audio_preview.at(k+1)) / 128.0 * (channel_height/2))));
        }

        // draw waveforms
        if (olive::CurrentConfig.rectified_waveforms)  {

          // rectified waveforms start from the bottom and draw upwards
          p->drawLine(clip_rect.left()+i, mid, clip_rect.left()+i, mid - (max - min));
        } else {

          // non-rectified waveforms start from the center and draw outwards
          p->drawLine(clip_rect.left()+i, mid+min, clip_rect.left()+i, mid+max);

        }
      }
    }
    last_waveform_index = waveform_index;
  }
}

void draw_transition(QPainter& p, ClipPtr c, const QRect& clip_rect, QRect& text_rect, int transition_type) {
  TransitionPtr t = (transition_type == kTransitionOpening) ? c->opening_transition : c->closing_transition;
  if (t != nullptr) {
    QColor transition_color(255, 0, 0, 16);
    int transition_width = getScreenPointFromFrame(panel_timeline->zoom, t->get_true_length());
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

void TimelineWidget::paintEvent(QPaintEvent*) {
  // Draw clips
  if (olive::ActiveSequence != nullptr) {
    QPainter p(this);

    // get widget width and height
    int video_track_limit = 0;
    int audio_track_limit = 0;
    for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
      ClipPtr clip = olive::ActiveSequence->clips.at(i);
      if (clip != nullptr) {
        video_track_limit = qMin(video_track_limit, clip->track());
        audio_track_limit = qMax(audio_track_limit, clip->track());
      }
    }

    // start by adding a track height worth of padding
    int panel_height = olive::timeline::kTrackDefaultHeight;

    // loop through tracks for maximum panel height
    if (bottom_align) {
      for (int i=-1;i>=video_track_limit;i--) {
        panel_height += panel_timeline->GetTrackHeight(i);
      }
    } else {
      for (int i=0;i<=audio_track_limit;i++) {
        panel_height += panel_timeline->GetTrackHeight(i);
      }
    }
    if (bottom_align) {
      scrollBar->setMinimum(qMin(0, - panel_height + height()));
    } else {
      scrollBar->setMaximum(qMax(0, panel_height - height()));
    }

    for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
      ClipPtr clip = olive::ActiveSequence->clips.at(i);
      if (clip != nullptr && is_track_visible(clip->track())) {
        QRect clip_rect(panel_timeline->getTimelineScreenPointFromFrame(clip->timeline_in()), getScreenPointFromTrack(clip->track()), getScreenPointFromFrame(panel_timeline->zoom, clip->length()), panel_timeline->GetTrackHeight(clip->track()));
        QRect text_rect(clip_rect.left() + olive::timeline::kClipTextPadding, clip_rect.top() + olive::timeline::kClipTextPadding, clip_rect.width() - olive::timeline::kClipTextPadding - 1, clip_rect.height() - olive::timeline::kClipTextPadding - 1);
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
            FootagePtr m = clip->media()->to_footage();
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

              if (clip->track() < 0) {
                // draw thumbnail
                int thumb_y = p.fontMetrics().height()+olive::timeline::kClipTextPadding+olive::timeline::kClipTextPadding;
                if (thumb_x < width() && thumb_y < height()) {
                  int space_for_thumb = clip_rect.width()-1;
                  if (clip->opening_transition != nullptr) {
                    int ot_width = getScreenPointFromFrame(panel_timeline->zoom, clip->opening_transition->get_true_length());
                    thumb_x += ot_width;
                    space_for_thumb -= ot_width;
                  }
                  if (clip->closing_transition != nullptr) {
                    space_for_thumb -= getScreenPointFromFrame(panel_timeline->zoom, clip->closing_transition->get_true_length());
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
                  checkerboard_rect.setLeft(panel_timeline->getTimelineScreenPointFromFrame(clip->media_length() + clip->timeline_in() - clip->clip_in()));
                }
              } else if (clip_rect.height() > olive::timeline::kTrackMinHeight) {
                // draw waveform
                p.setPen(QColor(80, 80, 80));

                int waveform_start = -qMin(clip_rect.x(), 0);
                int waveform_limit = qMin(clip_rect.width(), getScreenPointFromFrame(panel_timeline->zoom, media_length - clip->clip_in()));

                if ((clip_rect.x() + waveform_limit) > width()) {
                  waveform_limit -= (clip_rect.x() + waveform_limit - width());
                } else if (waveform_limit < clip_rect.width()) {
                  draw_checkerboard = true;
                  if (waveform_limit > 0) checkerboard_rect.setLeft(checkerboard_rect.left() + waveform_limit);
                }

                draw_waveform(clip, ms, media_length, &p, clip_rect, waveform_start, waveform_limit, panel_timeline->zoom);
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
            int marker_x = panel_timeline->getTimelineScreenPointFromFrame(marker_time);
            if (marker_x > clip_rect.x() && marker_x < clip_rect.right()) {
              draw_marker(p, marker_x, clip_rect.bottom()-p.fontMetrics().height(), clip_rect.bottom(), false);
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
          if (panel_timeline->tool == TIMELINE_TOOL_TRANSITION) {

            bool shared_transition = (panel_timeline->transition_tool_open_clip > -1
                                      && panel_timeline->transition_tool_close_clip > -1);

            QRect transition_tool_rect = clip_rect;
            bool draw_transition_tool_rect = false;

            if (panel_timeline->transition_tool_open_clip == i) {
              if (shared_transition) {
                transition_tool_rect.setWidth(TRANSITION_BETWEEN_RANGE);
              } else {
                transition_tool_rect.setWidth(transition_tool_rect.width()>>2);
              }
              draw_transition_tool_rect = true;
            } else if (panel_timeline->transition_tool_close_clip == i) {
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
    }

    // Draw recording clip if recording if valid
    if (panel_sequence_viewer->is_recording_cued() && is_track_visible(panel_sequence_viewer->recording_track)) {
      int rec_track_x = panel_timeline->getTimelineScreenPointFromFrame(panel_sequence_viewer->recording_start);
      int rec_track_y = getScreenPointFromTrack(panel_sequence_viewer->recording_track);
      int rec_track_height = panel_timeline->GetTrackHeight(panel_sequence_viewer->recording_track);
      if (panel_sequence_viewer->recording_start != panel_sequence_viewer->recording_end) {
        QRect rec_rect(
              rec_track_x,
              rec_track_y,
              getScreenPointFromFrame(panel_timeline->zoom, panel_sequence_viewer->recording_end - panel_sequence_viewer->recording_start),
              rec_track_height
              );
        p.setPen(QPen(QColor(96, 96, 96), 2));
        p.fillRect(rec_rect, QColor(192, 192, 192));
        p.drawRect(rec_rect);
      }
      QRect active_rec_rect(
            rec_track_x,
            rec_track_y,
            getScreenPointFromFrame(panel_timeline->zoom, panel_sequence_viewer->seq->playhead - panel_sequence_viewer->recording_start),
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

    // Draw track lines
    if (olive::CurrentConfig.show_track_lines) {
      p.setPen(QColor(0, 0, 0, 96));
      audio_track_limit++;
      if (video_track_limit == 0) video_track_limit--;

      if (bottom_align) {
        // only draw lines for video tracks
        for (int i=video_track_limit;i<0;i++) {
          int line_y = getScreenPointFromTrack(i) - 1;
          p.drawLine(0, line_y, rect().width(), line_y);
        }
      } else {
        // only draw lines for audio tracks
        for (int i=0;i<audio_track_limit;i++) {
          int line_y = getScreenPointFromTrack(i) + panel_timeline->GetTrackHeight(i);
          p.drawLine(0, line_y, rect().width(), line_y);
        }
      }
    }

    // Draw selections
    for (int i=0;i<olive::ActiveSequence->selections.size();i++) {
      const Selection& s = olive::ActiveSequence->selections.at(i);
      if (is_track_visible(s.track)) {
        int selection_y = getScreenPointFromTrack(s.track);
        int selection_x = panel_timeline->getTimelineScreenPointFromFrame(s.in);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::NoBrush);
        p.fillRect(selection_x, selection_y, panel_timeline->getTimelineScreenPointFromFrame(s.out) - selection_x, panel_timeline->GetTrackHeight(s.track), QColor(0, 0, 0, 64));
      }
    }

    // draw rectangle select
    if (panel_timeline->rect_select_proc) {
      QRect rect_select = panel_timeline->rect_select_rect;

      if (bottom_align) {
        rect_select.translate(0, height());
      }

      draw_selection_rectangle(p, rect_select);
    }

    // Draw ghosts
    if (!panel_timeline->ghosts.isEmpty()) {
      QVector<int> insert_points;
      long first_ghost = LONG_MAX;
      for (int i=0;i<panel_timeline->ghosts.size();i++) {
        const Ghost& g = panel_timeline->ghosts.at(i);
        first_ghost = qMin(first_ghost, g.in);
        if (is_track_visible(g.track)) {
          int ghost_x = panel_timeline->getTimelineScreenPointFromFrame(g.in);
          int ghost_y = getScreenPointFromTrack(g.track);
          int ghost_width = panel_timeline->getTimelineScreenPointFromFrame(g.out) - ghost_x - 1;
          int ghost_height = panel_timeline->GetTrackHeight(g.track) - 1;

          insert_points.append(ghost_y + (ghost_height>>1));

          p.setPen(QColor(255, 255, 0));
          for (int j=0;j<olive::timeline::kGhostThickness;j++) {
            p.drawRect(ghost_x+j, ghost_y+j, ghost_width-j-j, ghost_height-j-j);
          }
        }
      }

      // draw insert indicator
      if (panel_timeline->move_insert && !insert_points.isEmpty()) {
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        int insert_x = panel_timeline->getTimelineScreenPointFromFrame(first_ghost);
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


    // Draw splitting cursor
    if (panel_timeline->splitting) {
      for (int i=0;i<panel_timeline->split_tracks.size();i++) {
        if (is_track_visible(panel_timeline->split_tracks.at(i))) {
          int cursor_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->drag_frame_start);
          int cursor_y = getScreenPointFromTrack(panel_timeline->split_tracks.at(i));

          p.setPen(QColor(64, 64, 64));
          p.drawLine(cursor_x, cursor_y, cursor_x, cursor_y + panel_timeline->GetTrackHeight(panel_timeline->split_tracks.at(i)));
        }
      }
    }

    // Draw playhead
    p.setPen(Qt::red);
    int playhead_x = panel_timeline->getTimelineScreenPointFromFrame(olive::ActiveSequence->playhead);
    p.drawLine(playhead_x, rect().top(), playhead_x, rect().bottom());

    // Draw single frame highlight
    int playhead_frame_width = panel_timeline->getTimelineScreenPointFromFrame(olive::ActiveSequence->playhead+1) - playhead_x;
    if (playhead_frame_width > 5){ //hardcoded for now, maybe better way to do this?
        QRectF singleFrameRect(playhead_x, rect().top(), playhead_frame_width, rect().bottom());
        p.fillRect(singleFrameRect, QColor(255,255,255,15));
    }

    // draw border
    p.setPen(QColor(0, 0, 0, 64));
    int edge_y = (bottom_align) ? rect().height()-1 : 0;
    p.drawLine(0, edge_y, rect().width(), edge_y);

    // draw snap point
    if (panel_timeline->snapped) {
      p.setPen(Qt::white);
      int snap_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->snap_point);
      p.drawLine(snap_x, 0, snap_x, height());
    }

    // Draw edit cursor
    if (current_tool_shows_cursor() && is_track_visible(panel_timeline->cursor_track)) {
      int cursor_x = panel_timeline->getTimelineScreenPointFromFrame(panel_timeline->cursor_frame);
      int cursor_y = getScreenPointFromTrack(panel_timeline->cursor_track);

      p.setPen(Qt::gray);
      p.drawLine(cursor_x, cursor_y, cursor_x, cursor_y + panel_timeline->GetTrackHeight(panel_timeline->cursor_track));
    }
  }
}

void TimelineWidget::resizeEvent(QResizeEvent *) {
  scrollBar->setPageStep(height());
}

bool TimelineWidget::is_track_visible(int track) {
  return (bottom_align == (track < 0));
}

// **************************************
// screen point <-> frame/track functions
// **************************************

int TimelineWidget::getTrackFromScreenPoint(int y) {
  int track_candidate = 0;

  y += scroll;

  if (bottom_align) {
    y -= height();
  }

  if (y < 0) {
    track_candidate--;
  }

  int compounded_heights = 0;

  while (true) {
    int track_height = panel_timeline->GetTrackHeight(track_candidate);
    if (olive::CurrentConfig.show_track_lines) track_height++;
    if (y < 0) {
      track_height = -track_height;
    }

    int next_compounded_height = compounded_heights + track_height;


    if (y >= qMin(next_compounded_height, compounded_heights) && y < qMax(next_compounded_height, compounded_heights)) {
      return track_candidate;
    }

    compounded_heights = next_compounded_height;

    if (y < 0) {
      track_candidate--;
    } else {
      track_candidate++;
    }
  }
}

int TimelineWidget::getScreenPointFromTrack(int track) {
  int point = 0;

  int start = (track < 0) ? -1 : 0;
  int interval = (track < 0) ? -1 : 1;

  if (track < 0) track--;

  for (int i=start;i!=track;i+=interval) {
    point += panel_timeline->GetTrackHeight(i);
    if (olive::CurrentConfig.show_track_lines) point++;
  }

  if (bottom_align) {
    return height() - point - scroll;
  } else {
    return point - scroll;
  }
}

int TimelineWidget::getClipIndexFromCoords(long frame, int track) {
  for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
    ClipPtr c = olive::ActiveSequence->clips.at(i);
    if (c != nullptr && c->track() == track && frame >= c->timeline_in() && frame < c->timeline_out()) {
      return i;
    }
  }
  return -1;
}

void TimelineWidget::setScroll(int s) {
  scroll = s;
  update();
}

void TimelineWidget::reveal_media() {
  panel_project->reveal_media(rc_reveal_media);
}
