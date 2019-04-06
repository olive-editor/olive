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

#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QScrollBar>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>

#include "timeline/sequence.h"
#include "timeline/clip.h"
#include "timeline/timelinetools.h"
#include "timeline/timelinefunctions.h"
#include "project/footage.h"
#include "project/media.h"
#include "undo/undo.h"

class Timeline;

void draw_waveform(Clip* clip, const FootageStream *ms, long media_length, QPainter* p, const QRect& clip_rect, int waveform_start, int waveform_limit, double zoom);

class TimelineView : public QWidget {
  Q_OBJECT
public:
  explicit TimelineView(Timeline *parent);

  void SetAlignment(olive::timeline::Alignment alignment);
  void SetTrackList(TrackList* tl);

  Track* getTrackFromScreenPoint(int y);
  int getScreenPointFromTrack(Track* track);

  int getTrackIndexFromScreenPoint(int y);
  int getScreenPointFromTrackIndex(int track);

  int getTrackHeightFromTrackIndex(int track);

protected:
  void paintEvent(QPaintEvent*);

  void mouseDoubleClickEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void leaveEvent(QEvent *event);

  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent* event);
  void dragMoveEvent(QDragMoveEvent *event);

  void wheelEvent(QWheelEvent *event);
private:
  void init_ghosts();
  void update_ghosts(const QPoint& mouse_pos, bool lock_frame);

  Timeline* ParentTimeline();
  Sequence* sequence();
  void delete_area_under_ghosts(ComboAction* ca, Sequence *s);
  void insert_clips(ComboAction* ca, Sequence *s);
  bool current_tool_shows_cursor();
  void draw_transition(QPainter& p, Clip *c, const QRect& clip_rect, QRect& text_rect, int transition_type);
  Clip* GetClipAtCursor();

  void VerifyTransitionsAfterCreating(ComboAction* ca, Clip* open, Clip* close, long transition_start, long transition_end);
  void VerifyTransitionHelper();

  Timeline* timeline_;

  olive::timeline::Alignment alignment_;
  TrackList* track_list_;

  bool track_resizing;
  Track* track_target;

  QVector<Clip*> pre_clips;
  QVector<Clip*> post_clips;

  Media* rc_reveal_media;

  SequencePtr self_created_sequence;

  QTimer tooltip_timer;
  Clip* tooltip_clip;

  int scroll;

signals:
  void setScrollMaximum(int);
  void requestScrollChange(int);

public slots:
  void setScroll(int);

private slots:
  void reveal_media();
  void show_context_menu(const QPoint& pos);
  void toggle_autoscale();
  void tooltip_timer_timeout();
  void open_sequence_properties();
  void show_clip_properties();
};

#endif // TIMELINEWIDGET_H
