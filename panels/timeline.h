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

#include "ui/timelinewidget.h"
#include "ui/timelinetools.h"
#include "timeline/selection.h"
#include "timeline/clip.h"
#include "timeline/mediaimportdata.h"
#include "undo/undo.h"
#include "ui/timelineheader.h"
#include "ui/resizablescrollbar.h"
#include "ui/audiomonitor.h"
#include "ui/panel.h"

enum CreateObjects {
  ADD_OBJ_TITLE,
  ADD_OBJ_SOLID,
  ADD_OBJ_BARS,
  ADD_OBJ_TONE,
  ADD_OBJ_NOISE,
  ADD_OBJ_AUDIO
};

enum TrimType {
  TRIM_NONE,
  TRIM_IN,
  TRIM_OUT
};

namespace olive {
  namespace timeline {
    const int kGhostThickness = 2;
    const int kClipTextPadding = 3;

    /**
     * @brief Set default track sizes
     *
     * Olive has a few default constants used for adjusting track heights in the Timeline. For HiDPI, it makes
     * sense to multiply these by the current DPI scale. It uses a variable from QApplication to do this multiplication,
     * which means the QApplication instance needs to be instantiated before these are calculated. Therefore, call this
     * function ONCE after QApplication is created to multiply the track heights correctly.
     */
    void MultiplyTrackSizesByDPI();

    extern int kTrackDefaultHeight;
    extern int kTrackMinHeight;
    extern int kTrackHeightIncrement;
  }
}

int getScreenPointFromFrame(double zoom, long frame);
long getFrameFromScreenPoint(double zoom, int x);
bool selection_contains_transition(const Selection& s, Clip *c, int type);
void ripple_clips(ComboAction *ca, Sequence *s, long point, long length, const QVector<int>& ignore = QVector<int>());

struct Ghost {
  int clip;
  long in;
  long out;
  int track;
  long clip_in;

  long old_in;
  long old_out;
  int old_track;
  long old_clip_in;

  // importing variables
  Media* media;
  int media_stream;

  // other variables
  long ghost_length;
  long media_length;
  TrimType trim_type;

  // transition trimming
  TransitionPtr transition;
};

class Timeline : public Panel
{
  Q_OBJECT
public:
  explicit Timeline(QWidget *parent = nullptr);
  virtual ~Timeline() override;

  bool focused();
  void multiply_zoom(double m);
  void copy(bool del);
  ClipPtr split_clip(ComboAction* ca, bool transitions, int p, long frame);
  ClipPtr split_clip(ComboAction* ca, bool transitions, int p, long frame, long post_in);
  bool split_selection(ComboAction* ca);
  bool split_all_clips_at_point(ComboAction *ca, long point);
  bool split_clip_and_relink(ComboAction* ca, int clip, long frame, bool relink);
  void split_clip_at_positions(ComboAction* ca, int clip_index, QVector<long> positions);
  void clean_up_selections(QVector<Selection>& areas);
  void deselect_area(long in, long out, int track);
  void delete_areas_and_relink(ComboAction *ca, QVector<Selection>& areas, bool deselect_areas);
  void relink_clips_using_ids(QVector<int>& old_clips, QVector<ClipPtr>& new_clips);
  void update_sequence();

  void edit_to_point_internal(bool in, bool ripple);
  void delete_in_out_internal(bool ripple);

  void create_ghosts_from_media(Sequence *seq, long entry_point, QVector<olive::timeline::MediaImportData> &media_list);
  void add_clips_from_ghosts(ComboAction *ca, Sequence *s);

  int getTimelineScreenPointFromFrame(long frame);
  long getTimelineFrameFromScreenPoint(int x);
  int getDisplayScreenPointFromFrame(long frame);
  long getDisplayFrameFromScreenPoint(int x);

  int get_snap_range();
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

  int GetTrackHeight(int track);
  void SetTrackHeight(int track, int height);

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
  TrimType trim_type;
  int transition_select;

  // splitting
  bool splitting;
  QVector<int> split_tracks;
  QVector<int> split_cache;

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
  QVector<QPushButton*> tool_buttons;
  void decheck_tool_buttons(QObject* sender);
  void set_tool(int tool);
  int scroll;
  void set_sb_max();
  void UpdateTitle();

  void setup_ui();

  // ripple delete empty space variables
  long rc_ripple_min;
  long rc_ripple_max;

  QVector<TimelineTrackHeight> track_heights;

  QWidget* timeline_area;
  TimelineWidget* video_area;
  TimelineWidget* audio_area;
  QWidget* editAreas;
  QScrollBar* videoScrollbar;
  QScrollBar* audioScrollbar;
  QPushButton* zoomInButton;
  QPushButton* zoomOutButton;
  QPushButton* recordButton;
  QPushButton* addButton;
  QWidget* tool_button_widget;
};

#endif // TIMELINE_H
