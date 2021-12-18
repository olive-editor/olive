/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef TIMEBASEDVIEWSELECTIONMANAGER_H
#define TIMEBASEDVIEWSELECTIONMANAGER_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QRubberBand>
#include <QToolTip>

#include "common/rational.h"
#include "common/timecodefunctions.h"
#include "timebasedview.h"

namespace olive {

template <typename T>
class TimeBasedViewSelectionManager
{
public:
  TimeBasedViewSelectionManager(TimeBasedView *view) :
    view_(view),
    rubberband_(nullptr)
  {}

  void ClearDrawnObjects()
  {
    drawn_objects_.clear();
  }

  void DeclareDrawnObject(T *object, const QRectF &pos)
  {
    drawn_objects_.append({object, pos});
  }

  bool Select(T *key)
  {
    Q_ASSERT(key);

    if (!IsSelected(key)) {
      selected_.append(key);
      return true;
    }

    return false;
  }

  bool Deselect(T *key)
  {
    Q_ASSERT(key);

    return selected_.removeOne(key);
  }

  void ClearSelection()
  {
    selected_.clear();
  }

  bool IsSelected(T *key) const
  {
    return selected_.contains(key);
  }

  const QVector<T*> &GetSelectedObjects() const
  {
    return selected_;
  }

  void SetTimebase(const rational &tb)
  {
    timebase_ = tb;
  }

  T *GetObjectAtPoint(const QPointF &scene_pt)
  {
    // Iterate in reverse order because the objects drawn later will appear on top to the user
    for (auto it=drawn_objects_.crbegin(); it!=drawn_objects_.crend(); it++) {
      const DrawnObject &kp = *it;
      if (kp.second.contains(scene_pt)) {
        return kp.first;
      }
    }

    return nullptr;
  }

  T *GetObjectAtPoint(const QPoint &pt)
  {
    return GetObjectAtPoint(view_->mapToScene(pt));
  }

  T *MousePress(QMouseEvent *event)
  {
    T *key_under_cursor = nullptr;

    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
      // See if there's a keyframe in this position
      key_under_cursor = GetObjectAtPoint(event->pos());

      bool holding_shift = event->modifiers() & Qt::ShiftModifier;

      if (!key_under_cursor || !IsSelected(key_under_cursor)) {
        if (!holding_shift) {
          // If not already selecting and not holding shift, clear the current selection
          ClearSelection();
        }

        // Add item to selection, either nothing if shift wasn't held, or the existing selection
        if (key_under_cursor) {
          Select(key_under_cursor);
          view_->SelectionManagerSelectEvent(key_under_cursor);
        }
      } else if (holding_shift) {
        // If selected and holding shift, de-select this item but do nothing else
        Deselect(key_under_cursor);
        view_->SelectionManagerDeselectEvent(key_under_cursor);
        key_under_cursor = nullptr;
      }
    }

    return key_under_cursor;
  }

  bool IsDragging() const
  {
    return !dragging_.isEmpty();
  }

  void DragStart(T *initial_item, QMouseEvent *event)
  {
    initial_drag_item_ = initial_item;

    dragging_.resize(selected_.size());
    for (int i=0; i<selected_.size(); i++) {
      T *obj = selected_.at(i);

      dragging_[i] = {obj->time(), view_->TimeToScene(obj->time())};
    }

    drag_mouse_start_ = view_->mapToScene(event->pos());
  }

  void DragMove(QMouseEvent *event, const QString &tip_format)
  {
    QPointF diff = view_->mapToScene(event->pos()) - drag_mouse_start_;

    for (int i=0; i<selected_.size(); i++) {
      rational proposed_time = view_->SceneToTimeNoGrid(dragging_.at(i).x + diff.x());
      T *sel = selected_.at(i);

      // Magic number: use interval of 1ms to avoid collisions
      rational adj(1, 1000);
      if (dragging_.at(i).time < proposed_time) {
        adj = -adj;
      }

      while (true) {
        NodeKeyframe *key_at_time = sel->parent()->GetKeyframeAtTimeOnTrack(sel->input(), proposed_time, sel->track(), sel->element());
        if (!key_at_time || key_at_time == sel) {
          break;
        }

        proposed_time += adj;
      }

      sel->set_time(proposed_time);
    }

    // Show information about this keyframe
    QString tip = Timecode::time_to_timecode(initial_drag_item_->time(), timebase_,
                                             Core::instance()->GetTimecodeDisplay(), false);

    if (!tip_format.isEmpty()) {
      tip = tip_format.arg(tip);
    }

    QToolTip::hideText();
    QToolTip::showText(QCursor::pos(), tip);
  }

  void DragStop(MultiUndoCommand *command)
  {
    QToolTip::hideText();

    for (int i=0; i<selected_.size(); i++) {
      command->add_child(new SetTimeCommand(selected_.at(i), selected_.at(i)->time(), dragging_.at(i).time));
    }

    dragging_.clear();
  }

  void RubberBandStart(QMouseEvent *event)
  {
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
      rubberband_start_ = event->pos();

      rubberband_ = new QRubberBand(QRubberBand::Rectangle, view_);
      rubberband_->setGeometry(QRect(rubberband_start_.x(), rubberband_start_.y(), 0, 0));
      rubberband_->show();

      rubberband_preselected_ = selected_;
    }
  }

  void RubberBandMove(QMouseEvent *event)
  {
    if (IsRubberBanding()) {
      QRect band_rect = QRect(rubberband_start_, event->pos()).normalized();
      rubberband_->setGeometry(band_rect);

      QRectF scene_rect = view_->mapToScene(band_rect).boundingRect();

      selected_ = rubberband_preselected_;
      foreach (const DrawnObject &kp, drawn_objects_) {
        if (scene_rect.intersects(kp.second)) {
          Select(kp.first);
        }
      }
    }
  }

  void RubberBandStop()
  {
    if (IsRubberBanding()) {
      delete rubberband_;
      rubberband_ = nullptr;
    }
  }

  bool IsRubberBanding() const
  {
    return rubberband_;
  }

private:
  class SetTimeCommand : public UndoCommand
  {
  public:
    SetTimeCommand(T* key, const rational& time)
    {
      key_ = key;
      new_time_ = time;
      old_time_ = key_->time();
    }

    SetTimeCommand(T* key, const rational& new_time, const rational& old_time)
    {
      key_ = key;
      new_time_ = new_time;
      old_time_ = old_time;
    }

    virtual Project* GetRelevantProject() const override
    {
      return key_->parent()->project();
    }

  protected:
    virtual void redo() override
    {
      key_->set_time(new_time_);
    }

    virtual void undo() override
    {
      key_->set_time(old_time_);
    }

  private:
    T* key_;

    rational old_time_;
    rational new_time_;

  };

  TimeBasedView *view_;

  using DrawnObject = QPair<T*, QRectF>;
  QVector<DrawnObject> drawn_objects_;

  QVector<T*> selected_;

  struct DragObject
  {
    rational time;
    double x;
  };

  QVector<DragObject> dragging_;

  T *initial_drag_item_;

  QPointF drag_mouse_start_;

  rational timebase_;

  QRubberBand *rubberband_;
  QPoint rubberband_start_;
  QVector<T*> rubberband_preselected_;

};

}

#endif // TIMEBASEDVIEWSELECTIONMANAGER_H
