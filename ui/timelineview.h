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
#include "project/footage.h"
#include "project/media.h"
#include "undo/undo.h"
#include "timelinetools.h"

class Timeline;

bool same_sign(int a, int b);
void draw_waveform(ClipPtr clip, const FootageStream *ms, long media_length, QPainter* p, const QRect& clip_rect, int waveform_start, int waveform_limit, double zoom);

class TimelineView : public QWidget {
  Q_OBJECT
public:
  explicit TimelineView(QWidget *parent = nullptr);

  QScrollBar* scrollBar;
  bool bottom_align;

public slots:

protected:
  void paintEvent(QPaintEvent*);

  void resizeEvent(QResizeEvent *event);

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
  bool is_track_visible(int track);
  int getTrackFromScreenPoint(int y);
  int getScreenPointFromTrack(int track);
  int getClipIndexFromCoords(long frame, int track);

  void VerifyTransitionHelper();

  bool track_resizing;
  int track_target;

  QVector<ClipPtr> pre_clips;
  QVector<ClipPtr> post_clips;

  Media* rc_reveal_media;

  SequencePtr self_created_sequence;

  QTimer tooltip_timer;
  Clip* tooltip_clip;

  int scroll;

  SetSelectionsCommand* selection_command;
signals:

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
