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

#ifndef TIMELINE_H
#define TIMELINE_H

#include <QVector>
#include <QTime>
#include <QPushButton>

#include "ui/timelinearea.h"
#include "ui/timelinetools.h"
#include "timeline/selection.h"
#include "timeline/clip.h"
#include "timeline/mediaimportdata.h"
#include "timeline/ghost.h"
#include "undo/undo.h"
#include "ui/timelineheader.h"
#include "ui/resizablescrollbar.h"
#include "ui/audiomonitor.h"
#include "ui/panel.h"


int getScreenPointFromFrame(double zoom, long frame);
long getFrameFromScreenPoint(double zoom, int x);
bool selection_contains_transition(const Selection& s, Clip *c, int type);
void ripple_clips(ComboAction *ca, Sequence *s, long point, long length, const QVector<int>& ignore = QVector<int>());



class Timeline : public Panel
{
  Q_OBJECT
public:
  explicit Timeline(QWidget *parent = nullptr);

  virtual bool focused() override;
  void multiply_zoom(double m);
  void copy(bool del);
  void clean_up_selections(QVector<Selection>& areas);
  void deselect_area(long in, long out, int track);
  void delete_areas_and_relink(ComboAction *ca, QVector<Selection>& areas, bool deselect_areas);
  void update_sequence();

  void edit_to_point_internal(bool in, bool ripple);
  void delete_in_out_internal(bool ripple);

  void create_ghosts_from_media(Sequence *seq, long entry_point, QVector<olive::timeline::MediaImportData> &media_list);
  void add_clips_from_ghosts(ComboAction *ca, Sequence *s);

  int getTimelineScreenPointFromFrame(long frame);
  long getTimelineFrameFromScreenPoint(int x);
  int getDisplayScreenPointFromFrame(long frame);
  long getDisplayFrameFromScreenPoint(int x);

  long get_snap_range();
  bool snap_to_point(long point, long* l);
  bool snap_to_timeline(long* l, bool use_playhead, bool use_markers, bool use_workarea);
  void set_marker();

  // shared information
  int tool;
  long cursor_frame;
  int cursor_track;
  double zoom;
  bool zoom_just_changed;
  long drag_frame_start;
  int drag_track_start;
  void update_effect_controls();
  bool showing_all;
  double old_zoom;

  // snapping
  bool snapping;
  bool snapped;
  long snap_point;

  // selecting functions
  bool selecting;
  int selection_offset;
  void delete_selection(QVector<Selection> &selections, bool ripple);
  void select_all();
  bool rect_select_init;
  bool rect_select_proc;
  QRect rect_select_rect;

  // moving
  bool moving_init;
  bool moving_proc;
  QVector<Ghost> ghosts;
  bool video_ghosts;
  bool audio_ghosts;
  bool move_insert;

  // trimming
  int trim_target;
  olive::timeline::TrimType trim_type;
  int transition_select;

  // splitting
  bool splitting;
  QVector<int> split_tracks;

  // importing
  bool importing;
  bool importing_files;

  // creating variables
  bool creating;
  int creating_object;

  // transition variables
  bool transition_tool_init;
  bool transition_tool_proc;
  int transition_tool_open_clip;
  int transition_tool_close_clip;
  const EffectMeta* transition_tool_meta;
  int transition_tool_side;

  // hand tool variables
  bool hand_moving;
  int drag_x_start;
  int drag_y_start;

  bool block_repaints;

  TimelineHeader* headers;
  AudioMonitor* audio_monitor;
  ResizableScrollBar* horizontalScrollBar;

  QVector<QPushButton*> tool_buttons;
  QButtonGroup* tool_button_group;
  QPushButton* toolArrowButton;
  QPushButton* toolEditButton;
  QPushButton* toolRippleButton;
  QPushButton* toolRazorButton;
  QPushButton* toolSlipButton;
  QPushButton* toolSlideButton;
  QPushButton* toolHandButton;
  QPushButton* toolTransitionButton;
  QPushButton* snappingButton;

  void scroll_to_frame(long frame);
  void select_from_playhead();

  bool can_ripple_empty_space(long frame, int track);

  virtual void Retranslate() override;
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
public slots:
  void paste(bool insert = false);
  void repaint_timeline();
  void toggle_show_all();
  void deselect();
  void toggle_links();
  void split_at_playhead();
  void ripple_delete();
  void ripple_delete_empty_space();
  void toggle_enable_on_selected_clips();

  void delete_inout();
  void ripple_delete_inout();

  void ripple_to_in_point();
  void ripple_to_out_point();
  void edit_to_in_point();
  void edit_to_out_point();

  void IncreaseTrackHeight();
  void DecreaseTrackHeight();

  void previous_cut();
  void next_cut();

  void add_transition();

  void nest();

  void zoom_in();
  void zoom_out();

private slots:
  void snapping_clicked(bool checked);
  void add_btn_click();
  void add_menu_item(QAction*);
  void setScroll(int);
  void record_btn_click();
  void transition_tool_click();
  void transition_menu_select(QAction*);
  void resize_move(double d);
  void set_tool();

private:
  void ChangeTrackHeightUniformly(int diff);
  void set_zoom_value(double v);
  void set_tool(int tool);
  int scroll;
  void set_sb_max();
  void UpdateTitle();

  void setup_ui();

  // ripple delete empty space variables
  long rc_ripple_min;
  long rc_ripple_max;

  QWidget* timeline_area;
  TimelineArea* video_area;
  TimelineArea* audio_area;
  QWidget* editAreas;
  QPushButton* zoomInButton;
  QPushButton* zoomOutButton;
  QPushButton* recordButton;
  QPushButton* addButton;
  QWidget* tool_button_widget;
};

#endif // TIMELINE_H
