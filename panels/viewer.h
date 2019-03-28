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

#ifndef VIEWER_H
#define VIEWER_H

#include <QTimer>
#include <QIcon>
#include <QLabel>
#include <QPushButton>

#include "timeline/marker.h"
#include "timeline/mediaimportdata.h"
#include "project/media.h"

#include "ui/panel.h"
#include "ui/viewerwidget.h"
#include "ui/timelinewidget.h"
#include "ui/timelineheader.h"
#include "ui/labelslider.h"
#include "ui/resizablescrollbar.h"

bool frame_rate_is_droppable(double rate);
long timecode_to_frame(const QString& s, int view, double frame_rate);
QString frame_to_timecode(long f, int view, double frame_rate);

class Viewer : public Panel
{
  Q_OBJECT

public:
  explicit Viewer(QWidget *parent = nullptr);
  ~Viewer();

  bool is_focused();
  bool is_main_sequence();
  void set_main_sequence();
  void set_media(Media *m);
  void compose();
  void set_playpause_icon(bool play);
  void update_playhead_timecode(long p);
  void update_end_timecode();
  void update_header_zoom();
  void clear_in();
  void clear_out();
  void clear_inout_point();
  void set_in_point();
  void set_out_point();
  void set_zoom(bool in);
  void set_panel_name(const QString& n);
  void show_videoaudio_buttons(bool s);

  // playback functions
  void seek(long p);
  void play(bool in_to_out = false);
  void pause();
  bool playing;
  long playhead_start;
  qint64 start_msecs;
  QTimer playback_updater;


  void cue_recording(long start, long end, int track);
  void uncue_recording();
  bool is_recording_cued();
  long recording_start;
  long recording_end;
  int recording_track;

  void reset_all_audio();
  void update_parents(bool reload_fx = false);

  int get_playback_speed();

  ViewerWidget* viewer_widget;

  Media* media;
  SequencePtr seq;
  QVector<Marker>* marker_ref;

  void set_marker();

  TimelineHeader* headers;



  void initiate_drag(olive::timeline::MediaImportType drag_type);

  virtual void Retranslate() override;
protected:
  virtual void resizeEvent(QResizeEvent *event) override;

public slots:
  void play_wake();
  void go_to_start();
  void go_to_in();
  void previous_frame();
  void toggle_play();
  void increase_speed();
  void decrease_speed();
  void next_frame();
  void go_to_out();
  void go_to_end();
  void close_media();
  void update_viewer();



private slots:
  void update_playhead();
  void timer_update();
  void recording_flasher_update();
  void resize_move(double d);

  void drag_video_only();
  void drag_audio_only();

private:

  void update_window_title();
  void clean_created_seq();
  void set_sequence(bool main, SequencePtr s);
  bool main_sequence;
  bool created_sequence;
  long cached_end_frame;
  QString panel_name;
  double minimum_zoom;
  bool playing_in_to_out;
  long last_playhead;
  void set_zoom_value(double d);
  void set_sb_max();
  void set_playback_speed(int s);

  long get_seq_in();
  long get_seq_out();

  void setup_ui();

  ResizableScrollBar* horizontal_bar;
  ViewerContainer* viewer_container;
  LabelSlider* current_timecode_slider;
  QLabel* end_timecode;

  QPushButton* go_to_start_button;
  QPushButton* prev_frame_button;
  QPushButton* play_button;
  QPushButton* next_frame_button;
  QPushButton* go_to_end_frame;

  QPushButton* video_only_button;
  QPushButton* audio_only_button;

  bool cue_recording_internal;
  QTimer recording_flasher;

  long previous_playhead;
  int playback_speed;
};

#endif // VIEWER_H
