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

#include "viewer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QtMath>
#include <QAudioOutput>
#include <QPainter>
#include <QStringList>
#include <QTimer>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDrag>
#include <QMimeData>
#include <QMessageBox>

#include "rendering/audio.h"
#include "timeline.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "timeline/sequence.h"
#include "timeline/clip.h"
#include "panels/panels.h"
#include "global/config.h"
#include "global/timing.h"
#include "project/footage.h"
#include "project/media.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "ui/audiomonitor.h"
#include "rendering/renderfunctions.h"
#include "ui/viewercontainer.h"
#include "ui/labelslider.h"
#include "ui/timelineheader.h"
#include "ui/resizablescrollbar.h"
#include "ui/icons.h"
#include "global/global.h"
#include "global/debug.h"

#define FRAMES_IN_ONE_MINUTE 1798 // 1800 - 2
#define FRAMES_IN_TEN_MINUTES 17978 // (FRAMES_IN_ONE_MINUTE * 10) - 2

Viewer::Viewer(QWidget *parent) :
  Panel(parent),
  playing(false),
  media(nullptr),
  seq(nullptr),
  created_sequence(false),
  minimum_zoom(1.0),
  cue_recording_internal(false),
  playback_speed(0),
  mode_(kTimelineMode)
{
  setup_ui();

  headers->viewer = this;
  headers->snapping = false;
  headers->show_text(false);
  viewer_container->viewer = this;
  viewer_widget_ = viewer_container->child;
  viewer_widget_->viewer = this;
  set_media(nullptr);

  current_timecode_slider->setEnabled(false);
  current_timecode_slider->SetMinimum(0);
  current_timecode_slider->SetDefault(qSNaN());
  current_timecode_slider->SetValue(0);
  current_timecode_slider->SetDisplayType(LabelSlider::FrameNumber);
  connect(current_timecode_slider, SIGNAL(valueChanged(double)), this, SLOT(update_playhead()));

  recording_flasher.setInterval(500);

  connect(&playback_updater, SIGNAL(timeout()), this, SLOT(timer_update()));
  connect(&recording_flasher, SIGNAL(timeout()), this, SLOT(recording_flasher_update()));
  connect(horizontal_bar, SIGNAL(valueChanged(int)), headers, SLOT(set_scroll(int)));
  connect(horizontal_bar, SIGNAL(valueChanged(int)), viewer_widget_, SLOT(set_waveform_scroll(int)));
  connect(horizontal_bar, SIGNAL(resize_move(double)), this, SLOT(resize_move(double)));

  update_playhead_timecode(0);
  update_end_timecode();
}

void Viewer::SetMode(Viewer::Mode mode)
{
  mode_ = mode;
}

Viewer::Mode Viewer::mode()
{
  return mode_;
}

void Viewer::Retranslate() {
  /// Viewer panels are retranslated through the MainWindow to differentiate Media and Sequence Viewers
//  update_window_title();
}

bool Viewer::focused() {
  return headers->hasFocus()
      || viewer_widget_->hasFocus()
      || go_to_start_button->hasFocus()
      || prev_frame_button->hasFocus()
      || play_button->hasFocus()
      || next_frame_button->hasFocus()
      || go_to_end_frame->hasFocus();
}

void Viewer::reset_all_audio() {
  // reset all clip audio
  if (seq != nullptr) {
    long last_frame = 0;

    QVector<Clip*> all_clips = seq->GetAllClips();
    for (int i=0;i<all_clips.size();i++) {

      Clip* c = all_clips.at(i);

      c->reset_audio();
      last_frame = qMax(last_frame, c->timeline_out());

    }

    audio_ibuffer_frame = seq->playhead;
    if (playback_speed < 0) {
      audio_ibuffer_frame = last_frame - audio_ibuffer_frame;
    }
    audio_ibuffer_timecode = double(audio_ibuffer_frame) / seq->frame_rate();
  }
  clear_audio_ibuffer();
}

void Viewer::seek(long p) {
  pause();

  if (mode_ == kTimelineMode) {
    seq->playhead = p;
  } else {
    seq->playhead = qMin(seq->GetEndFrame(), qMax(0L, p));
  }

  bool update_fx = false;

  if (mode_ == kTimelineMode) {
    panel_timeline.first()->scroll_to_frame(p);
    panel_effect_controls->scroll_to_frame(p);
    if (olive::config.seek_also_selects) {
      seq->SelectAtPlayhead();
      update_fx = true;
    }
  }

  reset_all_audio();
  audio_scrub = true;
  last_playhead = seq->playhead;
  update_parents(update_fx);
}

void Viewer::go_to_start() {
  if (seq != nullptr) seek(0);
}

void Viewer::go_to_end() {
  if (seq != nullptr) seek(seq->GetEndFrame());
}

void Viewer::close_media() {
  set_media(nullptr);
}

void Viewer::go_to_in() {
  if (seq != nullptr) {
    if (seq->using_workarea) {
      seek(seq->workarea_in);
    } else {
      go_to_start();
    }
  }
}

void Viewer::previous_frame() {
  if (seq != nullptr && seq->playhead > 0) seek(seq->playhead-1);
}

void Viewer::next_frame() {
  if (seq != nullptr) seek(seq->playhead+1);
}

void Viewer::go_to_out() {
  if (seq != nullptr) {
    if (seq->using_workarea) {
      seek(seq->workarea_out);
    } else {
      go_to_end();
    }
  }
}

void Viewer::cue_recording(long start, long end, Track* track) {
  recording_start = start;
  recording_end = end;
  recording_track = track;
  panel_sequence_viewer->seek(recording_start);
  cue_recording_internal = true;
  recording_flasher.start();
}

void Viewer::uncue_recording() {
  cue_recording_internal = false;
  recording_flasher.stop();
  play_button->setStyleSheet(QString());
}

bool Viewer::is_recording_cued() {
  return cue_recording_internal;
}

void Viewer::toggle_play() {
  if (playing) {
    pause();
  } else {
    play();
  }
}

void Viewer::increase_speed() {
  int new_speed = playback_speed+1;
  if (new_speed == 0) {
    new_speed++;
  }
  set_playback_speed(new_speed);
}

void Viewer::decrease_speed() {
  int new_speed = playback_speed-1;
  if (new_speed == 0) {
    new_speed--;
  }
  set_playback_speed(new_speed);
}

void Viewer::play(bool in_to_out) {
  if (seq == nullptr) {
    return;
  }

  if (panel_sequence_viewer->playing) panel_sequence_viewer->pause();
  if (panel_footage_viewer->playing) panel_footage_viewer->pause();

  long sequence_end_frame = seq->GetEndFrame();
  if (sequence_end_frame == 0) {
    return;
  }

  playing_in_to_out = in_to_out;

  if (seq != nullptr) {

    if (playing_in_to_out) {
      uncue_recording();
    }

    bool seek_to_in = (seq->using_workarea && (olive::config.loop || playing_in_to_out));
    if (!is_recording_cued()
        && playback_speed >= 0
        && (playing_in_to_out
            || (olive::config.auto_seek_to_beginning && seq->playhead >= sequence_end_frame)
            || (seek_to_in && seq->playhead >= seq->workarea_out))) {
      seek(seek_to_in ? seq->workarea_in : 0);
    }

    if (playback_speed == 0) {
      playback_speed = 1;
    }

    reset_all_audio();
    if (is_recording_cued() && !start_recording()) {
      qCritical() << "Failed to record audio";
      return;
    }

    playhead_start = seq->playhead;
    playing = true;
    SetAudioWakeObject(this);
    set_playpause_icon(false);
    start_msecs = QDateTime::currentMSecsSinceEpoch();

    timer_update();
  }
}

void Viewer::play_wake() {
  start_msecs = QDateTime::currentMSecsSinceEpoch();
  playback_updater.start();
  if (audio_thread != nullptr) audio_thread->notifyReceiver();
}

void Viewer::pause() {
  playing = false;
  SetAudioWakeObject(nullptr);
  set_playpause_icon(true);
  playback_updater.stop();
  playback_speed = 0;

  if (is_recording_cued()) {
    uncue_recording();

    if (recording) {
      stop_recording();

      // import audio
      QStringList file_list;
      file_list.append(get_recorded_audio_filename());
      olive::project_model.process_file_list(file_list);

      // add it to the sequence
      QVector<Media*> last_imported_media = olive::project_model.GetLastImportedMedia();
      Media* m = last_imported_media.first();
      Footage* f = m->to_footage();

      // wait for footage to be completely ready before taking metadata from it
      f->ready_lock.lock();
      f->ready_lock.unlock();

      // Check if we were able to import the audio we just recorded
      if (f->invalid) {

        QMessageBox::critical(this,
                              tr("Failed to import recorded file"),
                              tr("An error occurred trying to import the recorded audio"),
                              QMessageBox::Ok);

      } else {

        // Make a clip out of it and add it to the Sequence

        ClipPtr c = std::make_shared<Clip>(recording_track);

        c->set_media(m, 0); // latest media
        c->set_timeline_in(recording_start);
        c->set_timeline_out(recording_start + f->get_length_in_frames(seq->frame_rate()));
        c->set_clip_in(0);
        c->set_color(128, 192, 128);
        c->set_name(m->get_name());

        QVector<ClipPtr> add_clips;
        add_clips.append(c);
        olive::undo_stack.push(new AddClipCommand(add_clips)); // add clip

      }



    }
  }
}

void Viewer::update_playhead_timecode(long p) {
  current_timecode_slider->SetValue(p);
}

void Viewer::update_end_timecode() {
  end_timecode->setText((seq == nullptr) ? frame_to_timecode(0, olive::config.timecode_view, 30) : frame_to_timecode(seq->GetEndFrame(), olive::config.timecode_view, seq->frame_rate()));
}

void Viewer::update_header_zoom() {
  if (seq != nullptr) {
    long sequenceEndFrame = seq->GetEndFrame();
    if (cached_end_frame != sequenceEndFrame) {
      minimum_zoom = (sequenceEndFrame > 0) ? (double(headers->width()) / double(sequenceEndFrame)) : 1;
      headers->update_zoom(qMax(headers->get_zoom(), minimum_zoom));
      set_sb_max();
      viewer_widget_->waveform_zoom = headers->get_zoom();
    } else {
      headers->update();
    }
  }
}

void Viewer::update_parents(bool reload_fx) {
  if (mode_ == kTimelineMode) {
    update_ui(reload_fx);
  } else {
    update_viewer();
  }
}

int Viewer::get_playback_speed() {
  return playback_speed;
}

ViewerWidget *Viewer::viewer_widget()
{
  return viewer_widget_;
}

void Viewer::set_marker() {
  Marker::SetOnSequence(seq.get());
}

void Viewer::resizeEvent(QResizeEvent *e) {
  QDockWidget::resizeEvent(e);
  if (seq != nullptr) {
    set_sb_max();
    viewer_widget_->update();
  }
}

void Viewer::update_viewer() {
  update_header_zoom();
  viewer_widget_->frame_update();
  if (seq != nullptr) {
    update_playhead_timecode(seq->playhead);
  }
  update_end_timecode();
}

void Viewer::prev_cut()
{
  if (seq != nullptr
      && seq->playhead > 0) {

    QVector<Clip*> sequence_clips = seq->GetAllClips();

    long p_cut = 0;
    for (int i=0;i<sequence_clips.size();i++) {
      Clip* c = sequence_clips.at(i);
      if (c->timeline_out() > p_cut && c->timeline_out() < seq->playhead) {
        p_cut = c->timeline_out();
      } else if (c->timeline_in() > p_cut && c->timeline_in() < seq->playhead) {
        p_cut = c->timeline_in();
      }
    }
    panel_sequence_viewer->seek(p_cut);
  }
}

void Viewer::next_cut()
{
  if (seq != nullptr) {

    QVector<Clip*> sequence_clips = seq->GetAllClips();

    bool seek_enabled = false;
    long n_cut = LONG_MAX;
    for (int i=0;i<sequence_clips.size();i++) {
      Clip* c = sequence_clips.at(i);
      if (c->timeline_in() < n_cut && c->timeline_in() > seq->playhead) {
        n_cut = c->timeline_in();
        seek_enabled = true;
      } else if (c->timeline_out() < n_cut && c->timeline_out() > seq->playhead) {
        n_cut = c->timeline_out();
        seek_enabled = true;
      }
    }

    if (seek_enabled) {
      panel_sequence_viewer->seek(n_cut);
    }
  }
}

void Viewer::initiate_drag(olive::timeline::MediaImportType drag_type)
{
  // FIXME: This should contain actual metadata rather than fake metadata

  QDrag* drag = new QDrag(this);
  QMimeData* mimeData = new QMimeData;
  mimeData->setText(QString::number(drag_type));
  drag->setMimeData(mimeData);
  drag->exec();
}

void Viewer::clear_in() {
  if (seq != nullptr
      && seq->using_workarea) {
    olive::undo_stack.push(new SetTimelineInOutCommand(seq.get(), true, 0, seq->workarea_out));
    update_parents();
  }
}

void Viewer::clear_out() {
  if (seq != nullptr
      && seq->using_workarea) {
    olive::undo_stack.push(new SetTimelineInOutCommand(seq.get(), true, seq->workarea_in, seq->GetEndFrame()));
    update_parents();
  }
}

void Viewer::clear_inout_point() {
  if (seq != nullptr
      && seq->using_workarea) {
    olive::undo_stack.push(new SetTimelineInOutCommand(seq.get(), false, 0, 0));
    update_parents();
  }
}

void Viewer::set_in_point() {
  if (seq != nullptr) {
    headers->set_in_point(seq->playhead);
  }
}

void Viewer::set_out_point() {
  if (seq != nullptr) {
    headers->set_out_point(seq->playhead);
  }
}

void Viewer::set_zoom(bool in) {
  if (seq != nullptr) {
    set_zoom_value(in ? headers->get_zoom()*2 : qMax(minimum_zoom, headers->get_zoom()*0.5));
  }
}

void Viewer::set_panel_name(const QString &n) {
  panel_name = n;
  update_window_title();
}

void Viewer::show_videoaudio_buttons(bool s)
{
  video_only_button->setVisible(s);
  audio_only_button->setVisible(s);
}

void Viewer::update_window_title() {
  setWindowTitle(panel_name.arg((seq == nullptr) ? tr("(none)") : seq->name()));
}

void Viewer::set_zoom_value(double d) {
  headers->update_zoom(d);
  if (viewer_widget_->waveform) {
    viewer_widget_->waveform_zoom = d;
    viewer_widget_->update();
  }
  if (seq != nullptr) {
    set_sb_max();
    if (!horizontal_bar->is_resizing())
      center_scroll_to_playhead(horizontal_bar, headers->get_zoom(), seq->playhead);
  }
}

void Viewer::set_sb_max() {
  headers->set_scrollbar_max(horizontal_bar, seq->GetEndFrame(), headers->width());
}

void Viewer::set_playback_speed(int s) {
  pause();
  playback_speed = qMin(3, qMax(s, -3));
  if (playback_speed != 0) {
    play();
  }
}

long Viewer::get_seq_in() {
  return ((olive::config.loop || playing_in_to_out) && seq->using_workarea)
      ? seq->workarea_in
      : 0;
}

long Viewer::get_seq_out() {
  return ((olive::config.loop || playing_in_to_out) && seq->using_workarea && previous_playhead < seq->workarea_out)
      ? seq->workarea_out
      : seq->GetEndFrame();
}

void Viewer::setup_ui() {
  QWidget* contents = new QWidget();

  QVBoxLayout* layout = new QVBoxLayout(contents);
  layout->setSpacing(0);
  layout->setMargin(0);

  setWidget(contents);

  viewer_container = new ViewerContainer();
  viewer_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget(viewer_container);

  headers = new TimelineHeader();
  layout->addWidget(headers);

  horizontal_bar = new ResizableScrollBar();
  horizontal_bar->setSingleStep(20);
  horizontal_bar->setOrientation(Qt::Horizontal);
  layout->addWidget(horizontal_bar);

  QWidget* lower_controls = new QWidget();

  QHBoxLayout* lower_control_layout = new QHBoxLayout(lower_controls);
  lower_control_layout->setMargin(0);

  QSizePolicy timecode_container_policy(QSizePolicy::Minimum, QSizePolicy::Maximum);
  QSizePolicy lower_control_policy(QSizePolicy::Maximum, QSizePolicy::Maximum);

  // Current time code container
  QWidget* current_timecode_container = new QWidget();
  current_timecode_container->setSizePolicy(timecode_container_policy);
  QHBoxLayout* current_timecode_container_layout = new QHBoxLayout(current_timecode_container);
  current_timecode_container_layout->setSpacing(0);
  current_timecode_container_layout->setMargin(0);
  current_timecode_slider = new LabelSlider();
  current_timecode_container_layout->addWidget(current_timecode_slider);
  lower_control_layout->addWidget(current_timecode_container);

  // Left controls container
  QWidget* left_controls = new QWidget();
  left_controls->setSizePolicy(lower_control_policy);
  lower_control_layout->addWidget(left_controls);

  // Playback controls container
  QWidget* playback_controls = new QWidget();
  playback_controls->setSizePolicy(lower_control_policy);

  QHBoxLayout* playback_control_layout = new QHBoxLayout(playback_controls);
  playback_control_layout->setSpacing(0);
  playback_control_layout->setMargin(0);
  playback_control_layout->addStretch();

  go_to_start_button = new QPushButton();
  go_to_start_button->setIcon(olive::icon::ViewerGoToStart);
  connect(go_to_start_button, SIGNAL(clicked(bool)), this, SLOT(go_to_in()));
  playback_control_layout->addWidget(go_to_start_button);

  prev_frame_button = new QPushButton();
  prev_frame_button->setIcon(olive::icon::ViewerPrevFrame);
  connect(prev_frame_button, SIGNAL(clicked(bool)), this, SLOT(previous_frame()));
  playback_control_layout->addWidget(prev_frame_button);

  play_button = new QPushButton();
  play_button->setIcon(olive::icon::ViewerPlay);
  connect(play_button, SIGNAL(clicked(bool)), this, SLOT(toggle_play()));
  playback_control_layout->addWidget(play_button);

  next_frame_button = new QPushButton();
  next_frame_button->setIcon(olive::icon::ViewerNextFrame);
  connect(next_frame_button, SIGNAL(clicked(bool)), this, SLOT(next_frame()));
  playback_control_layout->addWidget(next_frame_button);

  go_to_end_frame = new QPushButton();
  go_to_end_frame->setIcon(olive::icon::ViewerGoToEnd);
  connect(go_to_end_frame, SIGNAL(clicked(bool)), this, SLOT(go_to_out()));
  playback_control_layout->addWidget(go_to_end_frame);

  playback_control_layout->addStretch();

  lower_control_layout->addWidget(playback_controls);

  // Right controls container
  QWidget* right_controls = new QWidget();
  right_controls->setSizePolicy(lower_control_policy);

  QHBoxLayout* right_control_layout = new QHBoxLayout(right_controls);
  right_control_layout->setSpacing(0);
  right_control_layout->setMargin(0);
  right_control_layout->addStretch();

  video_only_button = new QPushButton();
  video_only_button->setToolTip(tr("Drag video only"));
  video_only_button->setIcon(olive::icon::MediaVideo);
  video_only_button->setVisible(false);
  right_control_layout->addWidget(video_only_button);
  connect(video_only_button, SIGNAL(pressed()), this, SLOT(drag_video_only()));

  audio_only_button = new QPushButton();
  audio_only_button->setToolTip(tr("Drag audio only"));
  audio_only_button->setIcon(olive::icon::MediaAudio);
  audio_only_button->setVisible(false);
  right_control_layout->addWidget(audio_only_button);
  connect(audio_only_button, SIGNAL(pressed()), this, SLOT(drag_audio_only()));

  right_control_layout->addStretch();

  lower_control_layout->addWidget(right_controls);

  // End time code container
  QWidget* end_timecode_container = new QWidget();
  end_timecode_container->setSizePolicy(timecode_container_policy);

  QHBoxLayout* end_timecode_layout = new QHBoxLayout(end_timecode_container);
  end_timecode_layout->setSpacing(0);
  end_timecode_layout->setMargin(0);

  end_timecode = new QLabel();
  end_timecode->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
  end_timecode_layout->addWidget(end_timecode);

  lower_control_layout->addWidget(end_timecode_container);

  layout->addWidget(lower_controls);
}

void Viewer::set_media(Media* m) {
  media = m;

  SequencePtr new_sequence = nullptr;

  if (media != nullptr) {
    switch (media->get_type()) {
    case MEDIA_TYPE_FOOTAGE:
    {
      Footage* footage = media->to_footage();

      marker_ref = &footage->markers;

      new_sequence = std::make_shared<Sequence>();
      created_sequence = true;
      new_sequence->wrapper_sequence = true;
      new_sequence->set_name(footage->name);

      new_sequence->using_workarea = footage->using_inout;
      if (footage->using_inout) {
        new_sequence->workarea_in = footage->in;
        new_sequence->workarea_out = footage->out;
      }

      new_sequence->set_frame_rate(olive::config.default_sequence_framerate);

      if (footage->video_tracks.size() > 0) {
        const FootageStream& video_stream = footage->video_tracks.at(0);
        new_sequence->set_width(video_stream.video_width);
        new_sequence->set_height(video_stream.video_height);
        if (video_stream.video_frame_rate > 0 && !video_stream.infinite_length) {
          new_sequence->set_frame_rate(video_stream.video_frame_rate * footage->speed);
        }

        Track* first_track = new_sequence->GetTrackList(olive::kTypeVideo).first();

        ClipPtr c = std::make_shared<Clip>(first_track);
        c->set_media(media, video_stream.file_index);
        c->set_timeline_in(0);
        c->set_timeline_out(footage->get_length_in_frames(new_sequence->frame_rate()));
        if (c->timeline_out() <= 0) {
          // FIXME: Move this magic number to Config
          c->set_timeline_out(150);
        }
        c->set_track(first_track);
        c->set_clip_in(0);
        c->refresh();
        first_track->AddClip(c);
      } else {
        new_sequence->set_width(olive::config.default_sequence_width);
        new_sequence->set_height(olive::config.default_sequence_height);
      }

      if (footage->audio_tracks.size() > 0) {
        const FootageStream& audio_stream = footage->audio_tracks.at(0);
        new_sequence->set_audio_frequency(audio_stream.audio_frequency);

        Track* track = new_sequence->GetTrackList(olive::kTypeAudio).first();
        ClipPtr c = std::make_shared<Clip>(track);
        c->set_media(media, audio_stream.file_index);
        c->set_timeline_in(0);
        c->set_timeline_out(footage->get_length_in_frames(new_sequence->frame_rate()));
        c->set_clip_in(0);
        c->refresh();
        track->AddClip(c);

        if (footage->video_tracks.size() == 0) {
          viewer_widget_->waveform = true;
          viewer_widget_->waveform_clip = c;
          viewer_widget_->waveform_ms = &audio_stream;
          viewer_widget_->frame_update();
        }
      } else {
        new_sequence->set_audio_frequency(olive::config.default_sequence_audio_frequency);
      }

      new_sequence->set_audio_layout(AV_CH_LAYOUT_STEREO);
    }
      break;
    case MEDIA_TYPE_SEQUENCE:
      new_sequence = media->to_sequence();
      break;
    }
  }

  set_sequence(new_sequence);
}

void Viewer::update_playhead() {
  seek(current_timecode_slider->value());
}

void Viewer::timer_update() {
  previous_playhead = seq->playhead;

  seq->playhead = qMax(0, qRound(playhead_start + ((QDateTime::currentMSecsSinceEpoch()-start_msecs) * 0.001 * seq->frame_rate() * playback_speed)));

  if (olive::config.seek_also_selects) {
    seq->SelectAtPlayhead();
  }

  update_parents(olive::config.seek_also_selects);

  if (playing) {
    if (playback_speed < 0 && seq->playhead == 0) {
      pause();
    } else if (recording) {
      if (recording_start != recording_end && seq->playhead >= recording_end) {
        pause();
      }
    } else if (playback_speed > 0) {
      long end_frame = seq->GetEndFrame();
      if ((olive::config.auto_seek_to_beginning || previous_playhead < end_frame) && seq->playhead >= end_frame) {
        pause();
      }
      if (seq->using_workarea && seq->playhead >= seq->workarea_out) {
        if (olive::config.loop) {
          // loop
          play();
        } else if (playing_in_to_out) {
          pause();
        }
      }
    }
  }
}

void Viewer::recording_flasher_update() {
  if (play_button->styleSheet().isEmpty()) {
    play_button->setStyleSheet("background: red;");
  } else {
    play_button->setStyleSheet(QString());
  }
}

void Viewer::resize_move(double d) {
  set_zoom_value(headers->get_zoom()*d);
}

void Viewer::drag_video_only()
{
  initiate_drag(olive::timeline::kImportVideoOnly);
}

void Viewer::drag_audio_only()
{
  initiate_drag(olive::timeline::kImportAudioOnly);
}

void Viewer::clean_created_seq() {
  viewer_widget_->waveform = false;

  if (created_sequence) {
    // TODO delete undo commands referencing this sequence to avoid crashes
    /*
    for (int i=0;i<olive::UndoStack.count();i++) {
      const QUndoCommand* oa = olive::UndoStack.command(i);
      if (typeid(*oa) == typeid(SetTimelineInOutCommand)) {
      }
    }
    */

    // Delete the current sequence
    seq.reset();

    created_sequence = false;
  }
}

void Viewer::set_sequence(SequencePtr s) {
  pause();

  reset_all_audio();

  viewer_widget_->wait_until_render_is_paused();

  // If we had a current sequence open, close it
  if (seq != nullptr) {
    seq->Close();
  }

  clean_created_seq();

  seq = s;

  bool null_sequence = (seq == nullptr);

  headers->setEnabled(!null_sequence);
  current_timecode_slider->setEnabled(!null_sequence);
  viewer_widget_->setEnabled(!null_sequence);
  viewer_widget_->setVisible(!null_sequence);
  go_to_start_button->setEnabled(!null_sequence);
  prev_frame_button->setEnabled(!null_sequence);
  play_button->setEnabled(!null_sequence);
  next_frame_button->setEnabled(!null_sequence);
  go_to_end_frame->setEnabled(!null_sequence);
  video_only_button->setEnabled(!null_sequence);
  audio_only_button->setEnabled(!null_sequence);

  if (!null_sequence) {
    current_timecode_slider->SetFrameRate(seq->frame_rate());

    playback_updater.setInterval(qFloor(1000 / seq->frame_rate()));

    update_playhead_timecode(seq->playhead);
    update_end_timecode();

    viewer_container->adjust();

    if (!created_sequence) {
      marker_ref = &seq->markers;
    }
  } else {
    update_playhead_timecode(0);
    update_end_timecode();
  }

  update_window_title();

  update_header_zoom();

  viewer_widget_->frame_update();

  update();
}

void Viewer::set_playpause_icon(bool play) {
  play_button->setIcon(play ? olive::icon::ViewerPlay : olive::icon::ViewerPause);
}
