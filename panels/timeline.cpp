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

#include "timeline.h"

#include <QTime>
#include <QScrollBar>
#include <QtMath>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStatusBar>
#include <QButtonGroup>

#include "global/global.h"
#include "panels/panels.h"
#include "project/projectelements.h"
#include "ui/timelineview.h"
#include "ui/icons.h"
#include "ui/viewerwidget.h"
#include "rendering/audio.h"
#include "rendering/cacher.h"
#include "rendering/renderfunctions.h"
#include "global/config.h"
#include "global/clipboard.h"
#include "global/math.h"
#include "ui/timelineheader.h"
#include "ui/resizablescrollbar.h"
#include "ui/audiomonitor.h"
#include "ui/flowlayout.h"
#include "ui/cursors.h"
#include "ui/mainwindow.h"
#include "undo/undostack.h"
#include "global/debug.h"
#include "global/timing.h"
#include "ui/menu.h"

Timeline::Timeline(QWidget *parent) :
  Panel(parent),
  cursor_frame(0),
  cursor_track(nullptr),
  zoom(1.0),
  zoom_just_changed(false),
  showing_all(false),
  selecting(false),
  rect_select_init(false),
  rect_select_proc(false),
  moving_init(false),
  moving_proc(false),
  move_insert(false),
  trim_target(nullptr),
  trim_type(olive::timeline::TRIM_NONE),
  splitting(false),
  importing(false),
  importing_files(false),
  creating(false),
  transition_tool_init(false),
  transition_tool_proc(false),
  transition_tool_open_clip(nullptr),
  transition_tool_close_clip(nullptr),
  hand_moving(false),
  block_repaints(false),
  scroll(0),
  sequence_(nullptr)
{
  setup_ui();

  headers->viewer = panel_sequence_viewer;

  video_area->SetAlignment(olive::timeline::kAlignmentBottom);

  tool_buttons.append(toolArrowButton);
  tool_buttons.append(toolEditButton);
  tool_buttons.append(toolRippleButton);
  tool_buttons.append(toolRazorButton);
  tool_buttons.append(toolSlipButton);
  tool_buttons.append(toolSlideButton);
  tool_buttons.append(toolTransitionButton);
  tool_buttons.append(toolHandButton);

  tool_button_group = new QButtonGroup(this);
  tool_button_group->addButton(toolArrowButton);
  tool_button_group->addButton(toolEditButton);
  tool_button_group->addButton(toolRippleButton);
  tool_button_group->addButton(toolRazorButton);
  tool_button_group->addButton(toolSlipButton);
  tool_button_group->addButton(toolSlideButton);
  tool_button_group->addButton(toolTransitionButton);
  tool_button_group->addButton(toolHandButton);

  toolArrowButton->click();

  connect(horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setScroll(int)));
  connect(horizontalScrollBar, SIGNAL(resize_move(double)), this, SLOT(resize_move(double)));
  connect(this, SIGNAL(SequenceChanged(SequencePtr)), panel_sequence_viewer, SLOT(set_sequence(SequencePtr)));
  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(visibility_changed_slot(bool)));

  update_sequence();

  Retranslate();
}

Timeline *Timeline::GetTopTimeline()
{
  for (int i=0;i<panel_timeline.size();i++) {
    if (panel_timeline.at(i)->isVisible()) {
      return panel_timeline.at(i);
    }
  }

  return nullptr;
}

SequencePtr Timeline::GetTopSequence()
{
  Timeline* top_timeline = GetTopTimeline();

  if (top_timeline != nullptr) {
    return top_timeline->sequence_;
  }

  return nullptr;
}

void Timeline::OpenSequence(SequencePtr s)
{
  Q_ASSERT(s != nullptr);

  for (int i=0;i<panel_timeline.size();i++) {
    Timeline* t = panel_timeline.at(i);
    if (t->sequence_ == s) {
      t->raise();
      return;
    } else if (t->sequence_ == nullptr) {
      t->SetSequence(s);
      t->raise();
      return;
    }
  }

  Timeline* t = new Timeline(olive::MainWindow);
  olive::MainWindow->tabifyDockWidget(panel_timeline.last(), t);
  t->SetSequence(s);
  t->show();
  t->raise();
  panel_timeline.append(t);
}

void Timeline::CloseSequence(Sequence *s)
{
  Q_ASSERT(s != nullptr);

  // Don't respond to a null sequence
  if (s == nullptr) {
    return;
  }

  // If there's only one Timeline object left, just set it to nullptr without destroying it
  if (panel_timeline.size() == 1) {
    panel_timeline.first()->SetSequence(nullptr);
    return;
  }

  // If there are multiple, kill the Timeline object that has the specified sequence
  for (int i=0;i<panel_timeline.size();i++) {
    Timeline* t = panel_timeline.at(i);

    if (t->sequence_.get() == s) {
      delete t;
      panel_timeline.removeAt(i);
      i--;
    }
  }
}

void Timeline::CloseAll()
{
  while (panel_timeline.size() > 1) {
    delete panel_timeline.last();
    panel_timeline.removeLast();
  }
  panel_timeline.first()->SetSequence(nullptr);
}

bool Timeline::IsImporting()
{
  for (int i=0;i<panel_timeline.size();i++) {
    if (panel_timeline.at(i)->importing) {
      return true;
    }
  }
  return false;
}

void Timeline::SetSequence(SequencePtr sequence)
{
  if (sequence_ == sequence) {
    return;
  }

  sequence_ = sequence;
  update_sequence();
  video_area->SetTrackList(sequence_.get(), olive::kTypeVideo);
  audio_area->SetTrackList(sequence_.get(), olive::kTypeAudio);
  repaint_timeline();

  emit SequenceChanged(sequence_);
}

void Timeline::Retranslate() {
  toolArrowButton->setToolTip(tr("Pointer Tool") + " (V)");
  toolEditButton->setToolTip(tr("Edit Tool") + " (X)");
  toolRippleButton->setToolTip(tr("Ripple Tool") + " (B)");
  toolRazorButton->setToolTip(tr("Razor Tool") + " (C)");
  toolSlipButton->setToolTip(tr("Slip Tool") + " (Y)");
  toolSlideButton->setToolTip(tr("Slide Tool") + " (U)");
  toolHandButton->setToolTip(tr("Hand Tool") + " (H)");
  toolTransitionButton->setToolTip(tr("Transition Tool") + " (T)");
  snappingButton->setToolTip(tr("Snapping") + " (S)");
  zoomInButton->setToolTip(tr("Zoom In") + " (=)");
  zoomOutButton->setToolTip(tr("Zoom Out") + " (-)");
  recordButton->setToolTip(tr("Record audio"));
  addButton->setToolTip(tr("Add title, solid, bars, etc."));

  UpdateTitle();
}

void Timeline::toggle_show_all() {
  if (sequence_ != nullptr) {
    showing_all = !showing_all;
    if (showing_all) {
      old_zoom = zoom;
      set_zoom_value(double(timeline_area->width() - 200) / double(sequence_->GetEndFrame()));
    } else {
      set_zoom_value(old_zoom);
    }
  }
}

void Timeline::toggle_links()
{
  if (sequence_ != nullptr) {
    sequence_->ToggleLinksOnSelected();
  }
}

void Timeline::add_transition() {
  ComboAction* ca = new ComboAction();
  bool adding = false;

  QVector<Clip*> selected_clips = sequence_->SelectedClips();

  for (int i=0;i<selected_clips.size();i++) {
    Clip* c = selected_clips.at(i);

    NodeType transition_to_add = (c->type() == olive::kTypeVideo) ? kCrossDissolveTransition
                                                                  : kLinearFadeTransition;

    if (c->opening_transition == nullptr) {
      ca->append(new AddTransitionCommand(c,
                                          nullptr,
                                          nullptr,
                                          transition_to_add,
                                          olive::config.default_transition_length));
      adding = true;
    }

    if (c->closing_transition == nullptr) {
      ca->append(new AddTransitionCommand(nullptr,
                                          c,
                                          nullptr,
                                          transition_to_add,
                                          olive::config.default_transition_length));
      adding = true;
    }
  }

  if (adding) {
    olive::undo_stack.push(ca);
  } else {
    delete ca;
  }

  update_ui(true);
}

void Timeline::nest() {
  if (sequence_ != nullptr) {
    // get selected clips
    QVector<Clip*> selected_clips = sequence_->SelectedClips();

    // nest them
    if (!selected_clips.isEmpty()) {

      // get earliest point in selected clips
      long earliest_point = LONG_MAX;
      for (int i=0;i<selected_clips.size();i++) {
        earliest_point = qMin(selected_clips.first()->timeline_in(), earliest_point);
      }

      ComboAction* ca = new ComboAction();

      // create "nest" sequence with the same attributes as the current sequence
      SequencePtr s = std::make_shared<Sequence>();

      s->name = olive::project_model.GetNextSequenceName(tr("Nested Sequence"));
      s->width = sequence_->width;
      s->height = sequence_->height;
      s->frame_rate = sequence_->frame_rate;
      s->audio_frequency = sequence_->audio_frequency;
      s->audio_layout = sequence_->audio_layout;

      QVector<ClipPtr> new_clips;

      // copy all selected clips to the nest
      for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);

        // delete clip from old sequence
        ca->append(new DeleteClipAction(c));

        // copy to new
        Track* track = s->GetTrackList(c->type())->TrackAt(c->track()->Index());
        ClipPtr copy = selected_clips.at(i)->copy(track);
        copy->set_timeline_in(copy->timeline_in() - earliest_point);
        copy->set_timeline_out(copy->timeline_out() - earliest_point);
        track->AddClip(copy);

        new_clips.append(copy);
      }

      // relink clips in new nested sequences
      olive::timeline::RelinkClips(selected_clips, new_clips);

      // add sequence to project
      MediaPtr m = olive::project_model.CreateSequence(ca, s, false, nullptr);

      // add nested sequence to active sequence
      QVector<olive::timeline::MediaImportData> media_list;
      media_list.append(m.get());
      olive::timeline::CreateGhostsFromMedia(sequence_.get(), earliest_point, media_list);

      // ensure ghosts won't overlap anything
      QVector<Clip*> all_sequence_clips = sequence_->GetAllClips();
      for (int j=0;j<all_sequence_clips.size();j++) {
        Clip* c = all_sequence_clips.at(j);
        if (!selected_clips.contains(c)) {
          for (int i=0;i<ghosts.size();i++) {
            Ghost& g = ghosts[i];
            if (c->track() == g.track
                && !((c->timeline_in() < g.in
                      && c->timeline_out() < g.in)
                     || (c->timeline_in() > g.out
                         && c->timeline_out() > g.out))) {

              // There's a clip occupied by the space taken up by this ghost. Move up a track, and seek again.
              g.track = g.track->Next();

              // Restart entire loop again
              j = -1;
              break;

            }
          }
        }
      }


      sequence_->AddClipsFromGhosts(ca, ghosts);

      panel_graph_editor->set_row(nullptr);
      panel_effect_controls->Clear(true);
      sequence_->ClearSelections();

      olive::undo_stack.push(ca);

      update_ui(true);
    }
  }
}

void Timeline::update_sequence() {
  bool null_sequence = (sequence_ == nullptr);

  for (int i=0;i<tool_buttons.count();i++) {
    tool_buttons.at(i)->setEnabled(!null_sequence);
  }
  snappingButton->setEnabled(!null_sequence);
  zoomInButton->setEnabled(!null_sequence);
  zoomOutButton->setEnabled(!null_sequence);
  recordButton->setEnabled(!null_sequence);
  addButton->setEnabled(!null_sequence);
  headers->setEnabled(!null_sequence);

  UpdateTitle();
}

bool Timeline::focused() {
  return (sequence_ != nullptr && (headers->hasFocus() || video_area->hasFocus() || audio_area->hasFocus()));
}

void Timeline::repaint_timeline() {
  if (!block_repaints) {
    bool draw = true;

    if (sequence_ != nullptr
        && !horizontalScrollBar->isSliderDown()
        && !horizontalScrollBar->is_resizing()
        && panel_sequence_viewer->playing
        && !zoom_just_changed) {
      // auto scroll
      if (olive::config.autoscroll == olive::AUTOSCROLL_PAGE_SCROLL) {
        int playhead_x = getTimelineScreenPointFromFrame(sequence_->playhead);
        if (playhead_x < 0 || playhead_x > (editAreas->width())) {
          horizontalScrollBar->setValue(getScreenPointFromFrame(zoom, sequence_->playhead));
          draw = false;
        }
      } else if (olive::config.autoscroll == olive::AUTOSCROLL_SMOOTH_SCROLL) {
        if (center_scroll_to_playhead(horizontalScrollBar, zoom, sequence_->playhead)) {
          draw = false;
        }
      }
    }

    if (draw) {
      headers->update();
      video_area->update();
      audio_area->update();

      if (sequence_ != nullptr
          && !zoom_just_changed) {
        set_sb_max();
      }
    }

    zoom_just_changed = false;
  }
}

void Timeline::select_all() {
  if (sequence_ != nullptr) {
    sequence_->SelectAll();
    repaint_timeline();
  }
}

void Timeline::scroll_to_frame(long frame) {
  scroll_to_frame_internal(horizontalScrollBar, frame, zoom, timeline_area->width());
}

void Timeline::resizeEvent(QResizeEvent *) {
  // adjust maximum scrollbar
  if (sequence_ != nullptr) set_sb_max();


  // resize tool button widget to its contents
  QList<QWidget*> tool_button_children = tool_button_widget->findChildren<QWidget*>();

  int horizontal_spacing = static_cast<FlowLayout*>(tool_button_widget->layout())->horizontalSpacing();
  int vertical_spacing = static_cast<FlowLayout*>(tool_button_widget->layout())->verticalSpacing();
  int total_area = tool_button_widget->height();

  int button_count = tool_button_children.size();
  int button_height = tool_button_children.at(0)->sizeHint().height() + vertical_spacing;

  int cols = 0;

  int col_height;

  if (button_height < total_area) {
    do {
      cols++;
      col_height = (qCeil(double(button_count)/double(cols))*button_height)-vertical_spacing;
    } while (col_height > total_area);
  } else {
    cols = button_count;
  }

  tool_button_widget->setFixedWidth((tool_button_children.at(0)->sizeHint().width())*cols + horizontal_spacing*(cols-1) + 1);
}

void Timeline::toggle_enable_on_selected_clips() {
  if (sequence_ != nullptr) {

    // get currently selected clips
    QVector<Clip*> selected_clips = sequence_->SelectedClips();

    if (!selected_clips.isEmpty()) {
      // if clips are selected, create an undoable action
      SetClipProperty* set_action = new SetClipProperty(kSetClipPropertyEnabled);

      // add each selected clip to the action
      for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);
        set_action->AddSetting(c, !c->enabled());
      }

      // push the action
      olive::undo_stack.push(set_action);
      update_ui(false);
    }
  }
}

void Timeline::set_zoom_value(double v) {
  // set zoom value
  zoom = v;

  // update header zoom to match
  headers->update_zoom(zoom);

  // set flag that zoom has just changed to prevent auto-scrolling since we change the scroll below
  zoom_just_changed = true;

  // set scrollbar to center the playhead
  if (sequence_ != nullptr) {
    // update scrollbar maximum value for new zoom
    set_sb_max();

    if (!horizontalScrollBar->is_resizing()) {
      center_scroll_to_playhead(horizontalScrollBar, zoom, sequence_->playhead);
    }
  }

  // repaint the timeline for the new zoom/location
  repaint_timeline();
}

void Timeline::multiply_zoom(double m) {
  showing_all = false;
  set_zoom_value(zoom * m);
}

void Timeline::zoom_in() {
  multiply_zoom(2.0);
}

void Timeline::zoom_out() {
  multiply_zoom(0.5);
}

void Timeline::ChangeTrackHeightUniformly(int diff) {
  if (sequence_ != nullptr) {
    sequence_->ChangeTrackHeightsRelatively(diff);
  }

  // update the timeline
  repaint_timeline();
}

void Timeline::IncreaseTrackHeight() {
  ChangeTrackHeightUniformly(olive::timeline::kTrackHeightIncrement);
}

void Timeline::DecreaseTrackHeight() {
  ChangeTrackHeightUniformly(-olive::timeline::kTrackHeightIncrement);
}

void Timeline::snapping_clicked(bool checked) {
  olive::timeline::snapping = checked;
}

/*
bool Timeline::split_clip_and_relink(ComboAction *ca, int clip, long frame, bool relink) {
  Clip* c = sequence_->clips.at(clip).get();
  if (c != nullptr) {
    QVector<int> pre_clips;
    QVector<ClipPtr> post_clips;

    ClipPtr post = split_clip(ca, true, clip, frame);

    if (post == nullptr) {
      return false;
    } else {
      post_clips.append(post);

      // if alt is not down, split clips links too
      if (relink) {
        pre_clips.append(clip);

        bool original_clip_is_selected = c->IsSelected();

        // find linked clips of old clip
        for (int i=0;i<c->linked.size();i++) {
          int l = c->linked.at(i);
          Clip* link = sequence_->clips.at(l).get();
          if ((original_clip_is_selected && link->IsSelected()) || !original_clip_is_selected) {
            ClipPtr s = split_clip(ca, true, l, frame);
            if (s != nullptr) {
              pre_clips.append(l);
              post_clips.append(s);
            }
          }
        }

        relink_clips_using_ids(pre_clips, post_clips);
      }
      ca->append(new AddClipCommand(sequence_.get(), post_clips));
      return true;
    }
  }
  return false;
}
*/



void Timeline::copy(bool del) {
  if (sequence_ != nullptr) {
    sequence_->AddSelectionsToClipboard(del);
  }
}

void Timeline::ripple_delete() {

  if (sequence_ != nullptr) {

    QVector<Selection> selections = sequence_->Selections();

    if (!selections.isEmpty()) {

      ComboAction* ca = new ComboAction();
      sequence_->DeleteAreas(ca, selections, true, true);
      olive::undo_stack.push(ca);
      repaint_timeline();

    } else if (olive::config.hover_focus && get_focused_panel() == this) {

      ripple_delete_empty_space();

    }
  }


}

void Timeline::ripple_delete_empty_space()
{
  if (sequence_ != nullptr) {
    ComboAction* ca = new ComboAction();
    sequence_->RippleDeleteEmptySpace(ca, cursor_track, cursor_frame);
    olive::undo_stack.push(ca);

    repaint_timeline();
  }
}

void Timeline::set_marker() {
  // determine if any clips are selected, and if so add markers to clips rather than the sequence

  QVector<Clip*> selected_clips = sequence_->SelectedClips();

  if (selected_clips.isEmpty()) {

    Marker::SetOnSequence(sequence_.get());

  } else {

    // Remove any clips that don't contain the playhead
    for (int i=0;i<selected_clips.size();i++) {
      Clip* c = selected_clips.at(i);

      if (c->timeline_out() < sequence_->playhead
          || c->timeline_in() > sequence_->playhead) {
        selected_clips.removeAt(i);
        i--;
      }
    }

    // Check if we removed them all
    if (selected_clips.isEmpty()) {
      return;
    }

    // If not, let's create markers on them
    Marker::SetOnClips(selected_clips);

  }
}

void Timeline::delete_inout() {
  if (sequence_ != nullptr) {
    sequence_->DeleteInToOut(false);
  }
}

void Timeline::ripple_delete_inout() {
  if (sequence_ != nullptr) {
    sequence_->DeleteInToOut(true);
  }
}

void Timeline::ripple_to_in_point() {
  if (sequence_ != nullptr) {
    sequence_->EditToPoint(true, true);
  }
}

void Timeline::ripple_to_out_point() {
  if (sequence_ != nullptr) {
    sequence_->EditToPoint(false, true);
  }
}

void Timeline::edit_to_in_point() {
  if (sequence_ != nullptr) {
    sequence_->EditToPoint(true, false);
  }
}

void Timeline::edit_to_out_point() {
  if (sequence_ != nullptr) {
    sequence_->EditToPoint(false, false);
  }
}

void Timeline::deselect() {
  if (sequence_ != nullptr) {
    sequence_->ClearSelections();
    repaint_timeline();
  }
}

void Timeline::split_at_playhead()
{
  if (sequence_ != nullptr) {
    sequence_->Split();
    repaint_timeline();
  }
}

long getFrameFromScreenPoint(double zoom, int x) {
  long f = qFloor(double(x) / zoom);
  if (f < 0) {
    return 0;
  }
  return f;
}

int getScreenPointFromFrame(double zoom, long frame) {
  return qRound(double(frame)*zoom);
}

long Timeline::getTimelineFrameFromScreenPoint(int x) {
  return getFrameFromScreenPoint(zoom, x + scroll);
}

QVector<Track *> Timeline::GetTracksInRectangle(int global_top, int global_bottom)
{
  QVector<Track*> tracks;

  TimelineArea* area;

  foreach (area, areas) {
    QPoint relative_tl = area->mapFromGlobal(QPoint(0, global_top));
    QPoint relative_br = area->mapFromGlobal(QPoint(0, global_bottom));

    int rect_top = qMin(relative_tl.y(), relative_br.y());
    int rect_bottom = qMax(relative_tl.y(), relative_br.y());

    // determine which clips are in this rectangular selection
    TrackList* track_list = area->track_list();
    for (int j=0;j<track_list->TrackCount();j++) {
      Track* track = track_list->TrackAt(j);

      int track_top = area->view()->getScreenPointFromTrack(track);
      int track_bottom = track_top + track->height();

      // See if this track touches this rectangle at all
      if (!(track_bottom < rect_top
            || track_top > rect_bottom)) {

        // It does, so we add it to the list
        tracks.append(track);

      }
    }
  }

  return tracks;
}

int Timeline::getTimelineScreenPointFromFrame(long frame) {
  return getScreenPointFromFrame(zoom, frame) - scroll;
}

void Timeline::add_btn_click() {
  Menu add_menu(this);

  QAction* titleMenuItem = new QAction(&add_menu);
  titleMenuItem->setText(tr("Title..."));
  titleMenuItem->setData(olive::timeline::ADD_OBJ_TITLE);
  add_menu.addAction(titleMenuItem);

  QAction* solidMenuItem = new QAction(&add_menu);
  solidMenuItem->setText(tr("Solid Color..."));
  solidMenuItem->setData(olive::timeline::ADD_OBJ_SOLID);
  add_menu.addAction(solidMenuItem);

  QAction* barsMenuItem = new QAction(&add_menu);
  barsMenuItem->setText(tr("Bars..."));
  barsMenuItem->setData(olive::timeline::ADD_OBJ_BARS);
  add_menu.addAction(barsMenuItem);

  add_menu.addSeparator();

  QAction* toneMenuItem = new QAction(&add_menu);
  toneMenuItem->setText(tr("Tone..."));
  toneMenuItem->setData(olive::timeline::ADD_OBJ_TONE);
  add_menu.addAction(toneMenuItem);

  QAction* noiseMenuItem = new QAction(&add_menu);
  noiseMenuItem->setText(tr("Noise..."));
  noiseMenuItem->setData(olive::timeline::ADD_OBJ_NOISE);
  add_menu.addAction(noiseMenuItem);

  connect(&add_menu, SIGNAL(triggered(QAction*)), this, SLOT(add_menu_item(QAction*)));

  add_menu.exec(QCursor::pos());
}

void Timeline::add_menu_item(QAction* action) {
  creating = true;
  creating_object = static_cast<olive::timeline::CreateObjects>(action->data().toInt());
}

void Timeline::setScroll(int s) {
  scroll = s;
  headers->set_scroll(s);
  repaint_timeline();
}

void Timeline::record_btn_click() {
  if (olive::ActiveProjectFilename.isEmpty()) {
    QMessageBox::critical(this,
                          tr("Unsaved Project"),
                          tr("You must save this project before you can record audio in it."),
                          QMessageBox::Ok);
  } else {
    creating = true;
    creating_object = olive::timeline::ADD_OBJ_AUDIO;
    olive::MainWindow->statusBar()->showMessage(
          tr("Click on the timeline where you want to start recording (drag to limit the recording to a certain timeframe)"),
          10000);
  }
}

void Timeline::transition_tool_click() {
  creating = false;

  Menu transition_menu(this);

  transition_menu.addAction(tr("Video Transitions"))->setEnabled(false);

  for (int i=0;i<olive::node_library.size();i++) {
    NodePtr node = olive::node_library.at(i);
    if (node != nullptr && node->type() == EFFECT_TYPE_TRANSITION && node->subtype() == olive::kTypeVideo) {
      QAction* a = transition_menu.addAction(node->name());
      a->setData(i);
    }
  }

  transition_menu.addSeparator();

  transition_menu.addAction(tr("Audio Transitions"))->setEnabled(false);

  for (int i=0;i<olive::node_library.size();i++) {
    NodePtr node = olive::node_library.at(i);
    if (node != nullptr && node->type() == EFFECT_TYPE_TRANSITION && node->subtype() == olive::kTypeAudio) {
      QAction* a = transition_menu.addAction(node->name());
      a->setData(i);
    }
  }

  connect(&transition_menu, SIGNAL(triggered(QAction*)), this, SLOT(transition_menu_select(QAction*)));

  toolTransitionButton->setChecked(false);

  transition_menu.exec(QCursor::pos());
}

void Timeline::transition_menu_select(QAction* a) {
  transition_tool_meta = static_cast<NodeType>(a->data().toInt());
  timeline_area->setCursor(Qt::CrossCursor);
  olive::timeline::current_tool = olive::timeline::TIMELINE_TOOL_TRANSITION;
  toolTransitionButton->setChecked(true);
}

void Timeline::resize_move(double z) {
  set_zoom_value(zoom * z);
}

void Timeline::set_sb_max() {
  headers->set_scrollbar_max(horizontalScrollBar, sequence_->GetEndFrame(), editAreas->width() - getScreenPointFromFrame(zoom, 200));
}

void Timeline::UpdateTitle() {
  QString title = tr("Timeline: ");
  if (sequence_ == nullptr) {
    setWindowTitle(title + tr("(none)"));
  } else {
    setWindowTitle(title + sequence_->name);
    update_ui(false);
  }
}

void Timeline::setup_ui() {
  QWidget* dockWidgetContents = new QWidget();

  QHBoxLayout* horizontalLayout = new QHBoxLayout(dockWidgetContents);
  horizontalLayout->setSpacing(0);
  horizontalLayout->setMargin(0);

  setWidget(dockWidgetContents);

  tool_button_widget = new QWidget();
  tool_button_widget->setObjectName("timeline_toolbar");
  tool_button_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  FlowLayout* tool_buttons_layout = new FlowLayout(tool_button_widget);
  tool_buttons_layout->setSpacing(4);
  tool_buttons_layout->setMargin(0);

  toolArrowButton = new QPushButton();
  toolArrowButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/arrow.svg")));
  toolArrowButton->setCheckable(true);
  toolArrowButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_POINTER);
  connect(toolArrowButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolArrowButton);

  toolEditButton = new QPushButton();
  toolEditButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/beam.svg")));
  toolEditButton->setCheckable(true);
  toolEditButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_EDIT);
  connect(toolEditButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolEditButton);

  toolRippleButton = new QPushButton();
  toolRippleButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/ripple.svg")));
  toolRippleButton->setCheckable(true);
  toolRippleButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_RIPPLE);
  connect(toolRippleButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolRippleButton);

  toolRazorButton = new QPushButton();
  toolRazorButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/razor.svg")));
  toolRazorButton->setCheckable(true);
  toolRazorButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_RAZOR);
  connect(toolRazorButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolRazorButton);

  toolSlipButton = new QPushButton();
  toolSlipButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/slip.svg")));
  toolSlipButton->setCheckable(true);
  toolSlipButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_SLIP);
  connect(toolSlipButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolSlipButton);

  toolSlideButton = new QPushButton();
  toolSlideButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/slide.svg")));
  toolSlideButton->setCheckable(true);
  toolSlideButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_SLIDE);
  connect(toolSlideButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolSlideButton);

  toolHandButton = new QPushButton();
  toolHandButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/hand.svg")));
  toolHandButton->setCheckable(true);

  toolHandButton->setProperty("tool", olive::timeline::TIMELINE_TOOL_HAND);
  connect(toolHandButton, SIGNAL(clicked(bool)), this, SLOT(set_tool()));
  tool_buttons_layout->addWidget(toolHandButton);
  toolTransitionButton = new QPushButton();
  toolTransitionButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/transition-tool.svg")));
  toolTransitionButton->setCheckable(true);
  connect(toolTransitionButton, SIGNAL(clicked(bool)), this, SLOT(transition_tool_click()));
  tool_buttons_layout->addWidget(toolTransitionButton);

  snappingButton = new QPushButton();
  snappingButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/magnet.svg")));
  snappingButton->setCheckable(true);
  snappingButton->setChecked(true);
  connect(snappingButton, SIGNAL(toggled(bool)), this, SLOT(snapping_clicked(bool)));
  tool_buttons_layout->addWidget(snappingButton);

  zoomInButton = new QPushButton();
  zoomInButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/zoomin.svg")));
  connect(zoomInButton, SIGNAL(clicked(bool)), this, SLOT(zoom_in()));
  tool_buttons_layout->addWidget(zoomInButton);

  zoomOutButton = new QPushButton();
  zoomOutButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/zoomout.svg")));
  connect(zoomOutButton, SIGNAL(clicked(bool)), this, SLOT(zoom_out()));
  tool_buttons_layout->addWidget(zoomOutButton);

  recordButton = new QPushButton();
  recordButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/record.svg")));
  connect(recordButton, SIGNAL(clicked(bool)), this, SLOT(record_btn_click()));
  tool_buttons_layout->addWidget(recordButton);

  addButton = new QPushButton();
  addButton->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/add-button.svg")));
  connect(addButton, SIGNAL(clicked()), this, SLOT(add_btn_click()));
  tool_buttons_layout->addWidget(addButton);

  horizontalLayout->addWidget(tool_button_widget);

  timeline_area = new QWidget();
  QSizePolicy timeline_area_policy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  timeline_area_policy.setHorizontalStretch(1);
  timeline_area_policy.setVerticalStretch(0);
  timeline_area_policy.setHeightForWidth(timeline_area->sizePolicy().hasHeightForWidth());
  timeline_area->setSizePolicy(timeline_area_policy);

  QVBoxLayout* timeline_area_layout = new QVBoxLayout(timeline_area);
  timeline_area_layout->setSpacing(0);
  timeline_area_layout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout* timeline_header_layout = new QHBoxLayout();
  timeline_header_layout->addSpacing(olive::timeline::kTimelineLabelFixedWidth);
  headers = new TimelineHeader();
  timeline_header_layout->addWidget(headers);
  timeline_area_layout->addLayout(timeline_header_layout);

  editAreas = new QWidget();
  QHBoxLayout* editAreaLayout = new QHBoxLayout(editAreas);
  editAreaLayout->setSpacing(0);
  editAreaLayout->setContentsMargins(0, 0, 0, 0);

  QSplitter* splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  splitter->setOrientation(Qt::Vertical);

  video_area = new TimelineArea(this);
  areas.append(video_area);
  splitter->addWidget(video_area);

  audio_area = new TimelineArea(this);
  areas.append(audio_area);
  splitter->addWidget(audio_area);

  editAreaLayout->addWidget(splitter);

  timeline_area_layout->addWidget(editAreas);

  horizontalScrollBar = new ResizableScrollBar();
  horizontalScrollBar->setMaximum(0);
  horizontalScrollBar->setSingleStep(20);
  horizontalScrollBar->setOrientation(Qt::Horizontal);

  timeline_area_layout->addWidget(horizontalScrollBar);

  horizontalLayout->addWidget(timeline_area);

  audio_monitor = new AudioMonitor();
  audio_monitor->setMinimumSize(QSize(50, 0));

  horizontalLayout->addWidget(audio_monitor);

  setWidget(dockWidgetContents);
}

void Timeline::set_tool() {
  QPushButton* button = static_cast<QPushButton*>(sender());
  olive::timeline::current_tool = static_cast<olive::timeline::Tool>(button->property("tool").toInt());
  creating = false;
  switch (olive::timeline::current_tool) {
  case olive::timeline::TIMELINE_TOOL_EDIT:
    timeline_area->setCursor(Qt::IBeamCursor);
    break;
  case olive::timeline::TIMELINE_TOOL_RAZOR:
    timeline_area->setCursor(olive::cursor::Razor);
    break;
  case olive::timeline::TIMELINE_TOOL_HAND:
    timeline_area->setCursor(Qt::OpenHandCursor);
    break;
  default:
    timeline_area->setCursor(Qt::ArrowCursor);
  }
}

void Timeline::visibility_changed_slot(bool visibility)
{
  if (visibility) {
    emit SequenceChanged(sequence_);
  }
}

void olive::timeline::MultiplyTrackSizesByDPI()
{
  kTrackDefaultHeight *= QApplication::desktop()->devicePixelRatio();
  kTrackMinHeight *= QApplication::desktop()->devicePixelRatio();
  kTrackHeightIncrement *= QApplication::desktop()->devicePixelRatio();
  kTimelineLabelFixedWidth *= QApplication::desktop()->devicePixelRatio();
}
