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

#include "keyframeview.h"

#include <QMouseEvent>
#include <QtMath>
#include <QMenu>

#include "effects/effect.h"
#include "ui/collapsiblewidget.h"
#include "panels/panels.h"
#include "timeline/clip.h"
#include "panels/timeline.h"
#include "ui/timelineheader.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "timeline/sequence.h"
#include "panels/grapheditor.h"
#include "ui/keyframedrawing.h"
#include "ui/clickablelabel.h"
#include "ui/resizablescrollbar.h"
#include "ui/rectangleselect.h"
#include "effects/keyframe.h"
#include "ui/graphview.h"

KeyframeView::KeyframeView(QWidget *parent) :
  QWidget(parent),
  visible_in(0),
  visible_out(0),
  mousedown(false),
  dragging(false),
  keys_selected(false),
  select_rect(false),
  x_scroll(0),
  y_scroll(0),
  scroll_drag(false)
{
  setFocusPolicy(Qt::ClickFocus);
  setMouseTracking(true);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));
}

void KeyframeView::SetEffects(const QVector<EffectUI *> &open_effects)
{
  open_effects_ = open_effects;
}

void KeyframeView::show_context_menu(const QPoint& pos) {
  if (selected_fields.size() > 0) {
    QMenu menu(this);

    QAction* linear = menu.addAction(tr("Linear"));
    linear->setData(EFFECT_KEYFRAME_LINEAR);
    QAction* bezier = menu.addAction(tr("Bezier"));
    bezier->setData(EFFECT_KEYFRAME_BEZIER);
    QAction* hold = menu.addAction(tr("Hold"));
    hold->setData(EFFECT_KEYFRAME_HOLD);
    menu.addSeparator();
    menu.addAction("Graph Editor");

    connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(menu_set_key_type(QAction*)));
    menu.exec(mapToGlobal(pos));
  }
}

void KeyframeView::menu_set_key_type(QAction* a) {
  if (a->data().isNull()) {
    // load graph editor
    panel_graph_editor->show();
  } else {
    ComboAction* ca = new ComboAction();
    for (int i=0;i<selected_fields.size();i++) {
      EffectField* f = selected_fields.at(i);
      ca->append(new SetInt(&f->keyframes[selected_keyframes.at(i)].type, a->data().toInt()));
    }
    olive::UndoStack.push(ca);
    update_ui(false);
  }
}

void KeyframeView::paintEvent(QPaintEvent*) {
  QPainter p(this);

  rowY.clear();
  rows.clear();

  if (!open_effects_.isEmpty()) {
    visible_in = LONG_MAX;
    visible_out = 0;

    for (int i=0;i<open_effects_.size();i++) {
      Clip* c = open_effects_.at(i)->GetEffect()->parent_clip;
      visible_in = qMin(visible_in, c->timeline_in());
      visible_out = qMax(visible_out, c->timeline_out());
    }

    for (int j=0;j<open_effects_.size();j++) {

      EffectUI* container = open_effects_.at(j);
      Effect* e = container->GetEffect();

      if (container->IsExpanded()) {
        for (int j=0;j<e->row_count();j++) {
          EffectRow* row = e->row(j);

          int keyframe_y = container->GetRowY(j, this);

          QVector<long> key_times;

          for (int l=0;l<row->FieldCount();l++) {
            EffectField* f = row->Field(l);
            for (int k=0;k<f->keyframes.size();k++) {
              if (!key_times.contains(f->keyframes.at(k).time)) {
                bool keyframe_selected = keyframeIsSelected(f, k);
                long keyframe_frame = adjust_row_keyframe(row, f->keyframes.at(k).time, visible_in);

                // see if any other keyframes have this time
                int appearances = 0;
                for (int m=0;m<row->FieldCount();m++) {
                  EffectField* compf = row->Field(m);
                  for (int n=0;n<compf->keyframes.size();n++) {
                    if (f->keyframes.at(k).time == compf->keyframes.at(n).time) {
                      appearances++;
                    }
                  }
                }

                if (appearances != row->FieldCount()) {
                  QColor cc = get_curve_color(l, row->FieldCount());
                  draw_keyframe(p, f->keyframes.at(k).type, getScreenPointFromFrame(panel_effect_controls->zoom, keyframe_frame) - x_scroll, keyframe_y, keyframe_selected, cc.red(), cc.green(), cc.blue());
                } else {
                  draw_keyframe(p, f->keyframes.at(k).type, getScreenPointFromFrame(panel_effect_controls->zoom, keyframe_frame) - x_scroll, keyframe_y, keyframe_selected);
                }

                key_times.append(f->keyframes.at(k).time);
              }
            }
          }

          rows.append(row);
          rowY.append(keyframe_y);
        }
      }
    }

    int max_width = getScreenPointFromFrame(panel_effect_controls->zoom, visible_out - visible_in);
    if (max_width < width()) {
      p.fillRect(QRect(max_width, 0, width(), height()), QColor(0, 0, 0, 64));
    }
    panel_effect_controls->horizontalScrollBar->setMaximum(qMax(max_width - width(), 0));
    header->set_visible_in(visible_in);

    int playhead_x = getScreenPointFromFrame(panel_effect_controls->zoom, olive::ActiveSequence->playhead-visible_in) - x_scroll;
    if (dragging && panel_timeline->snapped) {
      p.setPen(Qt::white);
    } else {
      p.setPen(Qt::red);
    }
    p.drawLine(playhead_x, 0, playhead_x, height());
  }

  if (select_rect) {
    draw_selection_rectangle(p, QRect(rect_select_x, rect_select_y, rect_select_w, rect_select_h));
  }

  /*if (mouseover && mouseover_row < rowY.size()) {
    draw_keyframe(p, getScreenPointFromFrame(panel_effect_controls->zoom, mouseover_frame - visible_in), rowY.at(mouseover_row), true);
    }*/
}

void KeyframeView::wheelEvent(QWheelEvent *e) {
  emit wheel_event_signal(e);
}

bool KeyframeView::keyframeIsSelected(EffectField *field, int keyframe) {
  for (int i=0;i<selected_fields.size();i++) {
    if (selected_fields.at(i) == field && selected_keyframes.at(i) == keyframe) {
      return true;
    }
  }
  return false;
}

void KeyframeView::update_keys() {
  //	panel_graph_editor->update_panel();
  update();
}

void KeyframeView::delete_selected_keyframes() {
  delete_keyframes(selected_fields, selected_keyframes);
}

void KeyframeView::set_x_scroll(int s) {
  x_scroll = s;
  update_keys();
}

void KeyframeView::set_y_scroll(int s) {
  y_scroll = s;
  update_keys();
}

void KeyframeView::resize_move(double d) {
  panel_effect_controls->zoom *= d;
  header->update_zoom(panel_effect_controls->zoom);
  update();
}

void KeyframeView::mousePressEvent(QMouseEvent *event) {
  rect_select_x = event->x();
  rect_select_y = event->y();
  rect_select_w = 0;
  rect_select_h = 0;

  if (panel_timeline->tool == TIMELINE_TOOL_HAND || event->buttons() & Qt::MiddleButton) {
    scroll_drag = true;
    return;
  }

  old_key_vals.clear();

  int mouse_x = event->x() + x_scroll;
  int mouse_y = event->y();
  int row_index = -1;
  int field_index = -1;
  int keyframe_index = -1;
  long frame_diff = 0;
  long frame_min = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x-KEYFRAME_SIZE);
  drag_frame_start = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x);
  long frame_max = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x+KEYFRAME_SIZE);
  for (int i=0;i<rowY.size();i++) {
    if (mouse_y > rowY.at(i)-KEYFRAME_SIZE-KEYFRAME_SIZE && mouse_y < rowY.at(i)+KEYFRAME_SIZE+KEYFRAME_SIZE) {
      EffectRow* row = rows.at(i);

      row->FocusRow();

      for (int k=0;k<row->FieldCount();k++) {
        EffectField* f = row->Field(k);
        for (int j=0;j<f->keyframes.size();j++) {
          long eval_keyframe_time = f->keyframes.at(j).time-row->GetParentEffect()->parent_clip->clip_in()+(row->GetParentEffect()->parent_clip->timeline_in()-visible_in);
          if (eval_keyframe_time >= frame_min && eval_keyframe_time <= frame_max) {
            long eval_frame_diff = qAbs(eval_keyframe_time - drag_frame_start);
            if (keyframe_index == -1 || eval_frame_diff < frame_diff) {
              row_index = i;
              field_index = k;
              keyframe_index = j;
              frame_diff = eval_frame_diff;
            }
          }
        }
      }
      break;
    }
  }
  bool already_selected = false;
  keys_selected = false;
  if (keyframe_index > -1) {
    already_selected = keyframeIsSelected(rows.at(row_index)->Field(field_index), keyframe_index);
  } else {
    select_rect = true;
  }
  if (!already_selected) {
    if (!(event->modifiers() & Qt::ShiftModifier)) {
      selected_fields.clear();
      selected_keyframes.clear();
    }
    if (keyframe_index > -1) {
      selected_fields.append(rows.at(row_index)->Field(field_index));
      selected_keyframes.append(keyframe_index);

      // find other field with keyframes at the same time
      long comp_time = rows.at(row_index)->Field(field_index)->keyframes.at(keyframe_index).time;
      for (int i=0;i<rows.at(row_index)->FieldCount();i++) {
        if (i != field_index) {
          EffectField* f = rows.at(row_index)->Field(i);
          for (int j=0;j<f->keyframes.size();j++) {
            if (f->keyframes.at(j).time == comp_time) {
              selected_fields.append(f);
              selected_keyframes.append(j);
            }
          }
        }
      }
    }
  }

  if (selected_fields.size() > 0) {
    for (int i=0;i<selected_fields.size();i++) {
      old_key_vals.append(selected_fields.at(i)->keyframes.at(selected_keyframes.at(i)).time);
    }
    keys_selected = true;
  }

  rect_select_offset = selected_fields.size();

  update_keys();

  if (event->button() == Qt::LeftButton) {
    mousedown = true;
  }
}

void KeyframeView::mouseMoveEvent(QMouseEvent* event) {
  if (panel_timeline->tool == TIMELINE_TOOL_HAND) {
    setCursor(Qt::OpenHandCursor);
  } else {
    unsetCursor();
  }
  if (scroll_drag) {
    panel_effect_controls->horizontalScrollBar->setValue(panel_effect_controls->horizontalScrollBar->value() + rect_select_x - event->pos().x());
    panel_effect_controls->verticalScrollBar->setValue(panel_effect_controls->verticalScrollBar->value() + rect_select_y - event->pos().y());
    rect_select_x = event->pos().x();
    rect_select_y = event->pos().y();
  } else if (mousedown) {
    int mouse_x = event->x() + x_scroll;

    long current_frame = getFrameFromScreenPoint(panel_effect_controls->zoom, mouse_x);
    panel_effect_controls->scroll_to_frame(current_frame + visible_in);

    if (select_rect) {
      // do a rect select
      selected_fields.resize(rect_select_offset);
      selected_keyframes.resize(rect_select_offset);

      rect_select_w = event->x() - rect_select_x;
      rect_select_h = event->y() - rect_select_y;

      int min_row = qMin(rect_select_y, event->y())-KEYFRAME_SIZE;
      int max_row = qMax(rect_select_y, event->y())+KEYFRAME_SIZE;

      long frame_start = getFrameFromScreenPoint(panel_effect_controls->zoom, rect_select_x+x_scroll);
      long min_frame = qMin(frame_start, current_frame)-KEYFRAME_SIZE;
      long max_frame = qMax(frame_start, current_frame)+KEYFRAME_SIZE;

      for (int i=0;i<rowY.size();i++) {
        if (rowY.at(i) >= min_row && rowY.at(i) <= max_row) {
          EffectRow* row = rows.at(i);
          for (int k=0;k<row->FieldCount();k++) {
            EffectField* field = row->Field(k);
            for (int j=0;j<field->keyframes.size();j++) {
              long keyframe_frame = adjust_row_keyframe(row, field->keyframes.at(j).time, visible_in);
              if (!keyframeIsSelected(field, j) && keyframe_frame >= min_frame && keyframe_frame <= max_frame) {
                selected_fields.append(field);
                selected_keyframes.append(j);
              }
            }
          }
        }
      }

      update_keys();
    } else if (keys_selected) {
      // move keyframes
      long frame_diff = current_frame - drag_frame_start;

      // snapping to playhead
      panel_timeline->snapped = false;
      if (panel_timeline->snapping) {
        for (int i=0;i<selected_keyframes.size();i++) {
          EffectField* field = selected_fields.at(i);
          Clip* c = field->GetParentRow()->GetParentEffect()->parent_clip;
          long key_time = old_key_vals.at(i) + frame_diff - c->clip_in() + c->timeline_in();
          long key_eval = key_time;
          if (panel_timeline->snap_to_point(olive::ActiveSequence->playhead, &key_eval)) {
            frame_diff += (key_eval - key_time);
            break;
          }
        }
      }

      // validate frame_diff (make sure no keyframes overlap each other)
      for (int i=0;i<selected_fields.size();i++) {
        EffectField* field = selected_fields.at(i);
        long eval_key = old_key_vals.at(i);
        for (int j=0;j<field->keyframes.size();j++) {
          while (!keyframeIsSelected(field, j) && field->keyframes.at(j).time == eval_key + frame_diff) {
            if (last_frame_diff > frame_diff) {
              frame_diff++;
              panel_timeline->snapped = false;
            } else {
              frame_diff--;
              panel_timeline->snapped = false;
            }
          }
        }
      }

      // apply frame_diffs
      for (int i=0;i<selected_keyframes.size();i++) {
        EffectField* field = selected_fields.at(i);
        field->keyframes[selected_keyframes.at(i)].time = old_key_vals.at(i) + frame_diff;
      }

      last_frame_diff = frame_diff;

      dragging = true;

      update_ui(false);
    }
  }
}

void KeyframeView::mouseReleaseEvent(QMouseEvent*) {
  if (dragging) {
    ComboAction* ca = new ComboAction();
    for (int i=0;i<selected_fields.size();i++) {
      ca->append(new SetLong(
                   &selected_fields.at(i)->keyframes[selected_keyframes.at(i)].time,
                 old_key_vals.at(i),
                 selected_fields.at(i)->keyframes.at(selected_keyframes.at(i)).time
                 ));
    }
    olive::UndoStack.push(ca);
  }

  select_rect = false;
  dragging = false;
  mousedown = false;
  scroll_drag = false;
  panel_timeline->snapped = false;
  update_ui(false);
}
