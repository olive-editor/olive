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

#include "effectfield.h"

#include <QDateTime>
#include <QtMath>

#include "ui/labelslider.h"
#include "ui/colorbutton.h"
#include "ui/texteditex.h"
#include "ui/checkboxex.h"
#include "ui/comboboxex.h"
#include "ui/fontcombobox.h"
#include "ui/embeddedfilechooser.h"

#include "rendering/renderfunctions.h"

#include "io/config.h"

#include "project/effectrow.h"
#include "project/effect.h"

#include "project/undo.h"
#include "project/clip.h"
#include "project/sequence.h"

#include "io/math.h"

#include "debug.h"

EffectField::EffectField(EffectRow* parent, const QString &i, EffectFieldType t) :
  QObject(parent),
  type_(t),
  id_(i),
  enabled_(true),
  colspan_(1)
{
  // EffectField MUST be created with a parent.
  Q_ASSERT(parent != nullptr);
  Q_ASSERT(!i.isEmpty() || t == EFFECT_FIELD_UI);

  // Add this field to the parent row specified
  parent->AddField(this);

  // Set a very base default value
  SetValueAt(0, 0);

  // Connect this field to the effect's changed function
  connect(this, SIGNAL(Changed()), parent->GetParentEffect(), SLOT(FieldChanged()));
}

EffectField::~EffectField() {}

EffectRow *EffectField::GetParentRow()
{
  return static_cast<EffectRow*>(parent());
}

int EffectField::GetColumnSpan()
{
  return colspan_;
}

void EffectField::SetColumnSpan(int i)
{
  Q_ASSERT(i >= 1);
  colspan_ = i;
}

QVariant EffectField::ConvertStringToValue(const QString &s)
{
  return s;
}

QString EffectField::ConvertValueToString(const QVariant &v)
{
  return v.toString();
}

QWidget *EffectField::CreateWidget()
{
  return new QLabel(tr("(Invalid field)"));
}

QVariant EffectField::GetValueAt(double timecode)
{
  Q_ASSERT(!keyframes.isEmpty());

  if (HasKeyframes()) {
    int before_keyframe;
    int after_keyframe;
    double progress;
    get_keyframe_data(timecode, before_keyframe, after_keyframe, progress);

    const QVariant& before_data = keyframes.at(before_keyframe).data;
    switch (type_) {
    case EFFECT_FIELD_DOUBLE:
    {
      double value;
      if (before_keyframe == after_keyframe) {
        value = keyframes.at(before_keyframe).data.toDouble();
      } else {
        const EffectKeyframe& before_key = keyframes.at(before_keyframe);
        const EffectKeyframe& after_key = keyframes.at(after_keyframe);

        double before_dbl = before_key.data.toDouble();
        double after_dbl = after_key.data.toDouble();

        if (before_key.type == EFFECT_KEYFRAME_HOLD) {
          // hold
          value = before_dbl;
        } else if (before_key.type == EFFECT_KEYFRAME_BEZIER || after_key.type == EFFECT_KEYFRAME_BEZIER) {
          // bezier interpolation
          if (before_key.type == EFFECT_KEYFRAME_BEZIER && after_key.type == EFFECT_KEYFRAME_BEZIER) {
            // cubic bezier
            double t = cubic_t_from_x(timecode*GetParentRow()->GetParentEffect()->parent_clip->sequence->frame_rate, before_key.time, before_key.time+GetValidKeyframeHandlePosition(before_keyframe, true), after_key.time+GetValidKeyframeHandlePosition(after_keyframe, false), after_key.time);
            value = cubic_from_t(before_dbl, before_dbl+before_key.post_handle_y, after_dbl+after_key.pre_handle_y, after_dbl, t);
          } else if (after_key.type == EFFECT_KEYFRAME_LINEAR) { // quadratic bezier
            // last keyframe is the bezier one
            double t = quad_t_from_x(timecode*GetParentRow()->GetParentEffect()->parent_clip->sequence->frame_rate, before_key.time, before_key.time+GetValidKeyframeHandlePosition(before_keyframe, true), after_key.time);
            value = quad_from_t(before_dbl, before_dbl+before_key.post_handle_y, after_dbl, t);
          } else {
            // this keyframe is the bezier one
            double t = quad_t_from_x(timecode*GetParentRow()->GetParentEffect()->parent_clip->sequence->frame_rate, before_key.time, after_key.time+GetValidKeyframeHandlePosition(after_keyframe, false), after_key.time);
            value = quad_from_t(before_dbl, after_dbl+after_key.pre_handle_y, after_dbl, t);
          }
        } else {
          // linear
          value = double_lerp(before_dbl, after_dbl, progress);
        }
      }
      return value;
    }
    case EFFECT_FIELD_COLOR:
    {
      QColor value;
      if (before_keyframe == after_keyframe) {
        value = keyframes.at(before_keyframe).data.value<QColor>();
      } else {
        QColor before_data = keyframes.at(before_keyframe).data.value<QColor>();
        QColor after_data = keyframes.at(after_keyframe).data.value<QColor>();
        value = QColor(lerp(before_data.red(), after_data.red(), progress), lerp(before_data.green(), after_data.green(), progress), lerp(before_data.blue(), after_data.blue(), progress));
      }
      return value;
    }
    case EFFECT_FIELD_STRING:
    case EFFECT_FIELD_BOOL:
    case EFFECT_FIELD_COMBO:
    case EFFECT_FIELD_FONT:
    case EFFECT_FIELD_FILE:
      return before_data;
    default:
      break;
    }
  }

  return keyframes.first().data;
}

void EffectField::SetValueAt(double timecode, const QVariant &value)
{
  if (keyframes.isEmpty()) {
    EffectKeyframe key;
    key.data = value;
    keyframes.append(key);
    return;
  }

  if (GetParentRow()->IsKeyframing()) {
    // create keyframe here
  } else {
    keyframes.first().data = value;
  }

  emit Changed();
}

double EffectField::Now()
{
  Clip* c = GetParentRow()->GetParentEffect()->parent_clip;
  return playhead_to_clip_seconds(c, c->sequence->playhead);
}

const EffectField::EffectFieldType &EffectField::type()
{
  return type_;
}

const QString &EffectField::id()
{
  return id_;
}

double EffectField::GetValidKeyframeHandlePosition(int key, bool post) {
  int comp_key = -1;

  // find keyframe before or after this one
  for (int i=0;i<keyframes.size();i++) {
    if (i != key
        && ((keyframes.at(i).time > keyframes.at(key).time) == post)
        && (comp_key == -1
          || ((keyframes.at(i).time < keyframes.at(comp_key).time) == post))) {
      // compare with next keyframe for post or previous frame for pre
      comp_key = i;
    }
  }

  double adjusted_key = post ? keyframes.at(key).post_handle_x : keyframes.at(key).pre_handle_x;

  // if this is the earliest/latest keyframe, no validation is required
  if (comp_key == -1) {
    return adjusted_key;
  }

  double comp = keyframes.at(comp_key).time - keyframes.at(key).time;

  // if comp keyframe is bezier, validate with its accompanying handle
  if (keyframes.at(comp_key).type == EFFECT_KEYFRAME_BEZIER) {
    double relative_comp_handle = comp + (post ? keyframes.at(comp_key).pre_handle_x : keyframes.at(comp_key).post_handle_x);
    // return an average
    if ((post && keyframes.at(key).post_handle_x > relative_comp_handle)
        || (!post && keyframes.at(key).pre_handle_x < relative_comp_handle)) {
      adjusted_key = (adjusted_key + relative_comp_handle)*0.5;
    }
  }

  // don't let handle go beyond the compare keyframe's time
  if (post == (adjusted_key > comp)) {
    return comp;
  }

  if (post == (adjusted_key < 0)) {
    return 0;
  }

  // original value is valid
  return adjusted_key;
}

double EffectField::frameToTimecode(long frame) {
  return (double(frame) / GetParentRow()->GetParentEffect()->parent_clip->sequence->frame_rate);
}

long EffectField::timecodeToFrame(double timecode) {
  return qRound(timecode * GetParentRow()->GetParentEffect()->parent_clip->sequence->frame_rate);
}

void EffectField::get_keyframe_data(double timecode, int &before, int &after, double &progress) {
  int before_keyframe_index = -1;
  int after_keyframe_index = -1;
  long before_keyframe_time = LONG_MIN;
  long after_keyframe_time = LONG_MAX;
  long frame = timecodeToFrame(timecode);

  for (int i=0;i<keyframes.size();i++) {
    long eval_keyframe_time = keyframes.at(i).time;
    if (eval_keyframe_time == frame) {
      before = i;
      after = i;
      return;
    } else if (eval_keyframe_time < frame && eval_keyframe_time > before_keyframe_time) {
      before_keyframe_index = i;
      before_keyframe_time = eval_keyframe_time;
    } else if (eval_keyframe_time > frame && eval_keyframe_time < after_keyframe_time) {
      after_keyframe_index = i;
      after_keyframe_time = eval_keyframe_time;
    }
  }

  if ((type_ == EFFECT_FIELD_DOUBLE || type_ == EFFECT_FIELD_COLOR) && (before_keyframe_index > -1 && after_keyframe_index > -1)) {
    // interpolate
    before = before_keyframe_index;
    after = after_keyframe_index;
    progress = (timecode-frameToTimecode(before_keyframe_time))/(frameToTimecode(after_keyframe_time)-frameToTimecode(before_keyframe_time));
  } else if (before_keyframe_index > -1) {
    before = before_keyframe_index;
    after = before_keyframe_index;
  } else {
    before = after_keyframe_index;
    after = after_keyframe_index;
  }
}

bool EffectField::HasKeyframes() {
  return (GetParentRow()->IsKeyframing() && keyframes.size() > 1);
}

/*
void EffectField::ui_element_change() {
  // TODO address this
  //bool dragging_double = (type_ == EFFECT_FIELD_DOUBLE && static_cast<LabelSlider*>(ui_element)->is_dragging());
  bool dragging_double = false;
  ComboAction* ca = nullptr;
  if (!dragging_double) ca = new ComboAction();
  //make_key_from_change(ca);
  if (!dragging_double) olive::UndoStack.push(ca);
  emit Changed();
}
*/

/*
void EffectField::make_key_from_change(ComboAction* ca) {
  if (GetParentRow()->isKeyframing()) {
    GetParentRow()->set_keyframe_now(ca);
  } else if (ca != nullptr) {
    // set undo
    ca->append(new EffectFieldUndo(this));
  }
}
*/

bool EffectField::IsEnabled() {
  return enabled_;
}

void EffectField::SetEnabled(bool e) {
  enabled_ = e;
  emit EnabledChanged(enabled_);
}
