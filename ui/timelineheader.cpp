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

#include "timelineheader.h"

#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtMath>
#include <QAction>

#include "mainwindow.h"
#include "panels/panels.h"
#include "global/math.h"
#include "timeline/sequence.h"
#include "undo/undo.h"
#include "project/media.h"
#include "global/timing.h"
#include "global/config.h"
#include "global/global.h"
#include "ui/menu.h"
#include "ui/menuhelper.h"
#include "global/debug.h"
#include "undo/undostack.h"

#define CLICK_RANGE 5
#define PLAYHEAD_SIZE 6
#define LINE_MIN_PADDING 50
#define SUBLINE_MIN_PADDING 50 // TODO play with this

// used only if center_timeline_timecodes is FALSE
#define TEXT_PADDING_FROM_LINE 4

bool center_scroll_to_playhead(QScrollBar* bar, double zoom, long playhead) {
  // returns true is the scroll was changed, false if not
  int target_scroll = qMin(bar->maximum(), qMax(0, getScreenPointFromFrame(zoom, playhead)-(bar->width()>>1)));
  if (target_scroll == bar->value()) {
    return false;
  }
  bar->setValue(target_scroll);
  return true;
}

TimelineHeader::TimelineHeader(QWidget *parent) :
  QWidget(parent),
  snapping(true),
  dragging(false),
  resizing_workarea(false),
  zoom(1),
  in_visible(0),
  fm(font()),
  dragging_markers(false),
  scroll(0),
  height_actual(fm.height())
{
  setCursor(Qt::ArrowCursor);
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);
  show_text(true);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(show_context_menu(const QPoint &)));
}

void TimelineHeader::set_scroll(int s) {
  scroll = s;
  update();
}

long TimelineHeader::getHeaderFrameFromScreenPoint(int x) {
  return getFrameFromScreenPoint(zoom, x + scroll) + in_visible;
}

int TimelineHeader::getHeaderScreenPointFromFrame(long frame) {
  return getScreenPointFromFrame(zoom, frame - in_visible) - scroll;
}

void TimelineHeader::set_playhead(int mouse_x) {
  long frame = getHeaderFrameFromScreenPoint(mouse_x);
  if (snapping) viewer->seq->SnapPoint(&frame, zoom, false, true, true);
  if (frame != viewer->seq->playhead) {
    viewer->seek(frame);
  }
}

int TimelineHeader::get_marker_offset() {
  return (text_enabled) ? height()/2 : 0;
}

void TimelineHeader::set_visible_in(long i) {
  in_visible = i;
  update();
}

void TimelineHeader::set_in_point(long new_in) {
  long new_out = viewer->seq->workarea_out;
  if (new_out == new_in) {
    new_in--;
  } else if (new_out < new_in) {
    new_out = viewer->seq->GetEndFrame();
  }

  olive::undo_stack.push(new SetTimelineInOutCommand(viewer->seq.get(), true, new_in, new_out));
  update_parents();
}

void TimelineHeader::set_out_point(long new_out) {
  long new_in = viewer->seq->workarea_in;
  if (new_out == new_in) {
    new_out++;
  } else if (new_in > new_out || new_in < 0) {
    new_in = 0;
  }

  olive::undo_stack.push(new SetTimelineInOutCommand(viewer->seq.get(), true, new_in, new_out));
  update_parents();
}

void TimelineHeader::set_scrollbar_max(QScrollBar* bar, long sequence_end_frame, int offset) {
  bar->setMaximum(qMax(0, getScreenPointFromFrame(zoom, sequence_end_frame) - offset));
}

void TimelineHeader::show_text(bool enable) {
  text_enabled = enable;
  if (enable) {
    setFixedHeight(height_actual*2);
  } else {
    setFixedHeight(height_actual);
  }
  update();
}

void TimelineHeader::mousePressEvent(QMouseEvent* event) {
  if (viewer->seq != nullptr && event->buttons() & Qt::LeftButton) {
    if (resizing_workarea) {
      sequence_end = viewer->seq->GetEndFrame();
    } else {
      /*int
      QPoint start(in_x, height()+2);
      QPainterPath path;
      path.moveTo(start + QPoint(1,0));
      path.lineTo(in_x-PLAYHEAD_SIZE, yoff);
      path.lineTo(in_x+PLAYHEAD_SIZE+1, yoff);*/

      bool shift = (event->modifiers() & Qt::ShiftModifier);
      bool clicked_on_marker = false;
      int playhead_x = getHeaderScreenPointFromFrame(viewer->seq->playhead);

      if (event->pos().y() > get_marker_offset()
          && (event->pos().x() < playhead_x-PLAYHEAD_SIZE
              || event->pos().x() > playhead_x+PLAYHEAD_SIZE)) {
        for (int i=0;i<viewer->marker_ref->size();i++) {
          int marker_pos = getHeaderScreenPointFromFrame(viewer->marker_ref->at(i).frame);
          if (event->pos().x() > marker_pos - MARKER_SIZE && event->pos().x() < marker_pos + MARKER_SIZE) {
            bool found = false;
            for (int j=0;j<selected_markers.size();j++) {
              if (selected_markers.at(j) == i) {
                if (shift) {
                  selected_markers.removeAt(j);
                }
                found = true;
                break;
              }
            }
            if (!found) {
              if (!shift) {
                selected_markers.clear();
              }
              selected_markers.append(i);
            }
            clicked_on_marker = true;
            update();
            break;
          }
        }
      }

      if (clicked_on_marker) {
        selected_marker_original_times.resize(selected_markers.size());
        for (int i=0;i<selected_markers.size();i++) {
          selected_marker_original_times[i] = viewer->marker_ref->at(selected_markers.at(i)).frame;
        }
        drag_start = event->pos().x();
        dragging_markers = true;
      } else {
        if (selected_markers.size() > 0 && !shift) {
          selected_markers.clear();
          update();
        }
        set_playhead(event->pos().x());
      }
    }
    dragging = true;
  }
}

void TimelineHeader::mouseMoveEvent(QMouseEvent* event) {
  if (viewer->seq != nullptr) {
    if (dragging) {
      if (resizing_workarea) {
        long frame = getHeaderFrameFromScreenPoint(event->pos().x());
        if (snapping) viewer->seq->SnapPoint(&frame, zoom, true, true, false);

        if (resizing_workarea_in) {
          temp_workarea_in = qMax(qMin(temp_workarea_out-1, frame), 0L);
        } else {
          temp_workarea_out = qMin(qMax(temp_workarea_in+1, frame), sequence_end);
        }

        update_parents();
      } else if (dragging_markers) {
        long frame_movement = getHeaderFrameFromScreenPoint(event->pos().x()) - getHeaderFrameFromScreenPoint(drag_start);

        // snap markers
        for (int i=0;i<selected_markers.size();i++) {
          long fm = selected_marker_original_times.at(i) + frame_movement;
          if (snapping && viewer->seq->SnapPoint(&fm, zoom, true, false, true)) {
            frame_movement = fm - selected_marker_original_times.at(i);
            break;
          }
        }

        // validate markers (ensure none go below 0)
        long validator;
        for (int i=0;i<selected_markers.size();i++) {
          validator = selected_marker_original_times.at(i) + frame_movement;
          if (validator < 0) {
            frame_movement -= validator;
          }
        }

        // move markers
        for (int i=0;i<selected_markers.size();i++) {
          viewer->marker_ref[0][selected_markers.at(i)].frame = selected_marker_original_times.at(i) + frame_movement;
        }

        update_parents();
      } else {
        set_playhead(event->pos().x());
      }
    } else {
      resizing_workarea = false;
      unsetCursor();
      if (viewer->seq != nullptr && viewer->seq->using_workarea) {
        long min_frame = getHeaderFrameFromScreenPoint(event->pos().x() - CLICK_RANGE) - 1;
        long max_frame = getHeaderFrameFromScreenPoint(event->pos().x() + CLICK_RANGE) + 1;
        if (viewer->seq->workarea_in > min_frame && viewer->seq->workarea_in < max_frame) {
          resizing_workarea = true;
          resizing_workarea_in = true;
        } else if (viewer->seq->workarea_out > min_frame && viewer->seq->workarea_out < max_frame) {
          resizing_workarea = true;
          resizing_workarea_in = false;
        }
        if (resizing_workarea) {
          temp_workarea_in = viewer->seq->workarea_in;
          temp_workarea_out = viewer->seq->workarea_out;
          setCursor(Qt::SizeHorCursor);
        }
      }
    }
  }
}

void TimelineHeader::mouseReleaseEvent(QMouseEvent*) {
  if (viewer->seq != nullptr) {
    dragging = false;
    if (resizing_workarea) {
      olive::undo_stack.push(new SetTimelineInOutCommand(viewer->seq.get(), true, temp_workarea_in, temp_workarea_out));
    } else if (dragging_markers && selected_markers.size() > 0) {
      bool moved = false;
      ComboAction* ca = new ComboAction();
      for (int i=0;i<selected_markers.size();i++) {
        Marker* m = &viewer->marker_ref[0][selected_markers.at(i)];
        if (selected_marker_original_times.at(i) != m->frame) {
          ca->append(new MoveMarkerAction(m, selected_marker_original_times.at(i), m->frame));
          moved = true;
        }
      }
      if (moved) {
        olive::undo_stack.push(ca);
      } else {
        delete ca;
      }
    }

    resizing_workarea = false;
    dragging = false;
    dragging_markers = false;
    olive::timeline::snapped = false;
    update_parents();
  }
}

void TimelineHeader::focusOutEvent(QFocusEvent*) {
  selected_markers.clear();
  update();
}

void TimelineHeader::update_parents() {
  viewer->update_parents();
}

void TimelineHeader::update_zoom(double z) {
  zoom = z;
  update();
}

double TimelineHeader::get_zoom() {
  return zoom;
}

void TimelineHeader::delete_markers() {
  if (selected_markers.size() > 0) {

    // Send command to delete selected markers
    DeleteMarkerAction* dma = new DeleteMarkerAction(viewer->marker_ref);
    dma->markers.append(selected_markers);
    olive::undo_stack.push(dma);

    // remove any indices for the selected markers that no longer exist
    for (int i=0;i<selected_markers.size();i++) {
      if (selected_markers.at(i) >= viewer->marker_ref->size()) {
        selected_markers.removeAt(i);
        i--;
      }
    }

    // if we removed all the indices, re-select the last marker in the array so something is always selected
    // (allows users to hold delete when deleting markers)
    if (selected_markers.isEmpty() && !viewer->marker_ref->isEmpty()) {
      selected_markers.append(viewer->marker_ref->size() - 1);
    }

    update_parents();
  }
}

void TimelineHeader::paintEvent(QPaintEvent*) {
  if (viewer->seq != nullptr && zoom > 0) {
    QPainter p(this);
    int yoff = get_marker_offset();

    double interval = viewer->seq->frame_rate();
    int textWidth = 0;
    int lastTextBoundary = INT_MIN;

    int lastLineX = INT_MIN;

    int sublineCount = 1;
    int sublineTest = qRound(interval*zoom);
    int sublineInterval = 1;
    while (sublineTest > SUBLINE_MIN_PADDING
           && sublineInterval >= 1) {
      sublineCount *= 2;
      sublineInterval = (interval/sublineCount);
      sublineTest = qRound(sublineInterval*zoom);
    }
    sublineCount = qMin(sublineCount, qRound(interval));

    int text_x, fullTextWidth;
    QString timecode;

    // find where to start drawing lines (lineX algorithm reversed if lineX = 0)
    int i = qFloor(double(scroll)/zoom/interval);

    while (true) {
      long frame = qRound(interval*i);
      int lineX = qRound(frame*zoom) - scroll;

      if (lineX > width()) break;

      // draw text
      bool draw_text = false;
      if (text_enabled && lineX-textWidth > lastTextBoundary) {
        timecode = TimeToSMPTE(frame + in_visible, olive::config.timecode_view, viewer->seq->frame_rate());
        fullTextWidth = fm.width(timecode);
        textWidth = fullTextWidth>>1;

        text_x = lineX;

        // centers the text to that point on the timeline, LEFT aligns it if not
        if (olive::config.center_timeline_timecodes) {
          text_x -= textWidth;
        } else {
          text_x += TEXT_PADDING_FROM_LINE;
        }

        lastTextBoundary = lineX+textWidth;
        if (lastTextBoundary >= 0) {
          draw_text = true;
        }
      }

      if (lineX > lastLineX+LINE_MIN_PADDING) {
        if (draw_text) {
          p.setPen(olive::styling::GetIconColor());
          p.drawText(QRect(text_x, 0, fullTextWidth, yoff), timecode);
        }

        // draw line markers
        p.setPen(Qt::gray);
        p.drawLine(lineX, (!olive::config.center_timeline_timecodes && draw_text) ? 0 : yoff, lineX, height());

        // draw sub-line markers
        for (int j=1;j<sublineCount;j++) {
          int sublineX = lineX+(qRound(j*interval/sublineCount)*zoom);
          p.drawLine(sublineX, yoff, sublineX, yoff+(height()/4));
        }

        lastLineX = lineX;
      }

      i++;
    }

    // draw in/out selection
    int in_x;
    if (viewer->seq->using_workarea) {
      in_x = getHeaderScreenPointFromFrame((resizing_workarea ? temp_workarea_in : viewer->seq->workarea_in));
      int out_x = getHeaderScreenPointFromFrame((resizing_workarea ? temp_workarea_out : viewer->seq->workarea_out));
      p.fillRect(QRect(in_x, 0, out_x-in_x, height()), QColor(0, 192, 255, 128));
      p.setPen(olive::styling::GetIconColor());
      p.drawLine(in_x, 0, in_x, height());
      p.drawLine(out_x, 0, out_x, height());
    }

    // draw markers
    for (int i=0;i<viewer->marker_ref->size();i++) {
      const Marker& m = viewer->marker_ref->at(i);

      int marker_x = getHeaderScreenPointFromFrame(m.frame);

      bool selected = false;
      for (int j=0;j<selected_markers.size();j++) {
        if (selected_markers.at(j) == i) {
          selected = true;
          break;
        }
      }

      Marker::Draw(p, marker_x, yoff, height()-1, selected);
    }

    // draw playhead triangle
    p.setRenderHint(QPainter::Antialiasing);
    in_x = getHeaderScreenPointFromFrame(viewer->seq->playhead);
    QPoint start(in_x, height()+2);
    QPainterPath path;
    path.moveTo(start + QPoint(1,0));
    path.lineTo(in_x-PLAYHEAD_SIZE, yoff);
    path.lineTo(in_x+PLAYHEAD_SIZE+1, yoff);
    path.lineTo(start);
    p.fillPath(path, Qt::red);

    // Draw white line at the top for clarity
    p.setPen(Qt::gray);
    p.drawLine(0, 0, width(), 0);
  }
}

void TimelineHeader::show_context_menu(const QPoint &pos) {
  Menu menu(this);

  // Add items for setting the in/out points of a QMenu
  olive::MenuHelper.make_inout_menu(&menu);

  menu.addSeparator();

  QAction* center_timecodes = menu.addAction(tr("Center Timecodes"), &olive::MenuHelper, SLOT(toggle_bool_action()));
  center_timecodes->setCheckable(true);
  center_timecodes->setChecked(olive::config.center_timeline_timecodes);
  center_timecodes->setData(reinterpret_cast<quintptr>(&olive::config.center_timeline_timecodes));

  menu.exec(mapToGlobal(pos));
}

void TimelineHeader::resized_scroll_listener(double d) {
  update_zoom(zoom * d);
}
