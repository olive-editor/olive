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
#include <cfloat>

#include "rendering/renderfunctions.h"
#include "global/config.h"
#include "global/timing.h"
#include "nodes/nodeio.h"
#include "nodes/oldeffectnode.h"
#include "undo/undo.h"
#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "global/math.h"
#include "global/debug.h"

EffectField::EffectField(NodeIO* parent, EffectFieldType t) :
  QObject(parent),
  type_(t),
  enabled_(true)
{
  // EffectField MUST be created with a parent.
  Q_ASSERT(parent != nullptr);

  // Set a very base default value
  SetValueAt(0, 0);

  // Connect this field to the effect's changed function
  connect(this, SIGNAL(Changed()), parent->ParentNode(), SLOT(FieldChanged()));
}

NodeIO *EffectField::GetParentRow()
{
  return static_cast<NodeIO*>(parent());
}

QVariant EffectField::ConvertStringToValue(const QString &s)
{
  return s;
}

QString EffectField::ConvertValueToString(const QVariant &v)
{
  return v.toString();
}

void EffectField::UpdateWidgetValue(QWidget *, const rational &) {}

QVariant EffectField::GetValueAt(const rational& timecode)
{
  if (HasKeyframes()) {
    int before_keyframe;
    int after_keyframe;
    double progress;
    GetKeyframeData(timecode, before_keyframe, after_keyframe, progress);

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

          // Hold keyframes will always return the previous keyframe with no interpolation
          value = before_dbl;

        } else if (before_key.type == EFFECT_KEYFRAME_BEZIER || after_key.type == EFFECT_KEYFRAME_BEZIER) {

          double timecode_dbl = timecode.ToDouble();
          double before_time_dbl = before_key.time.ToDouble();
          double after_time_dbl = after_key.time.ToDouble();

          // bezier interpolation
          if (before_key.type == EFFECT_KEYFRAME_BEZIER && after_key.type == EFFECT_KEYFRAME_BEZIER) {

            // cubic bezier
            double t = cubic_t_from_x(timecode_dbl,
                                      before_time_dbl,
                                      before_time_dbl+GetValidKeyframeHandlePosition(before_keyframe, true),
                                      after_time_dbl+GetValidKeyframeHandlePosition(after_keyframe, false),
                                      after_time_dbl);

            value = cubic_from_t(before_dbl,
                                 before_dbl+before_key.post_handle.y(),
                                 after_dbl+after_key.pre_handle.y(),
                                 after_dbl,
                                 t);

          } else if (after_key.type == EFFECT_KEYFRAME_LINEAR) { // quadratic bezier

            // last keyframe is the bezier one
            double t = quad_t_from_x(timecode_dbl,
                                     before_time_dbl,
                                     before_time_dbl+GetValidKeyframeHandlePosition(before_keyframe, true),
                                     after_time_dbl);

            value = quad_from_t(before_dbl,
                                before_dbl+before_key.post_handle.y(),
                                after_dbl,
                                t);

          } else {
            // this keyframe is the bezier one
            double t = quad_t_from_x(timecode_dbl,
                                     before_time_dbl,
                                     after_time_dbl+GetValidKeyframeHandlePosition(after_keyframe, false),
                                     after_time_dbl);

            value = quad_from_t(before_dbl,
                                after_dbl+after_key.pre_handle.y(),
                                after_dbl,
                                t);
          }
        } else {

          // Linear interpolation (default)
          value = double_lerp(before_dbl, after_dbl, progress);

        }
      }
      persistent_data_ = value;
      break;
    }
    case EFFECT_FIELD_COLOR:
    {
      QColor value;
      if (before_keyframe == after_keyframe) {
        value = keyframes.at(before_keyframe).data.value<QColor>();
      } else {
        QColor before_data = keyframes.at(before_keyframe).data.value<QColor>();
        QColor after_data = keyframes.at(after_keyframe).data.value<QColor>();
        value = QColor(lerp(before_data.red(), after_data.red(), progress),
                       lerp(before_data.green(), after_data.green(), progress),
                       lerp(before_data.blue(), after_data.blue(), progress));
      }
      persistent_data_ = value;
      break;
    }
    case EFFECT_FIELD_STRING:
    case EFFECT_FIELD_BOOL:
    case EFFECT_FIELD_COMBO:
    case EFFECT_FIELD_FONT:
    case EFFECT_FIELD_FILE:
      persistent_data_ = before_data;
      break;
    default:
      break;
    }
  }

  return persistent_data_;
}

void EffectField::SetValueAt(const rational &time, const QVariant &value)
{
  if (HasKeyframes()) {

    // Create keyframe here

    // Check array if a keyframe at this time already exists
    int keyframe_index = -1;
    for (int i=0;i<keyframes.size();i++) {
      if (keyframes.at(i).time == time) {
        keyframe_index = i;
        break;
      }
    }

    // If keyframe doesn't exist, make it
    if (keyframe_index == -1) {
      EffectKeyframe key;
      key.time = time;
      key.data = value;
      key.type = (keyframes.isEmpty()) ? EFFECT_KEYFRAME_LINEAR : keyframes.last().type;
      keyframes.append(key);
    } else {
      EffectKeyframe& key = keyframes[keyframe_index];
      key.data = value;
    }

  } else {

    persistent_data_ = value;

  }

  emit Changed();
}

void EffectField::PrepareDataForKeyframing(bool enabled, ComboAction *ca)
{
  if (enabled) {

    // Create keyframe from perpetual data
    EffectKeyframe key;

    key.time = GetParentRow()->ParentNode()->Time();
    key.data = persistent_data_;
    key.type = EFFECT_KEYFRAME_LINEAR;

    keyframes.append(key);

    ca->append(new KeyframeAdd(this, keyframes.size()-1));

  } else {

    // Convert keyframes to one "perpetual" keyframe

    // Set first keyframe to whatever the data is now
    ca->append(new SetQVariant(&persistent_data_, persistent_data_, GetValueAt(GetParentRow()->ParentNode()->Time())));

    // Delete all keyframes
    for (int i=0;i<keyframes.size();i++) {
      ca->append(new KeyframeDelete(this, 0));
    }

  }
}

const EffectField::EffectFieldType &EffectField::type()
{
  return type_;
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

  double adjusted_key = post ? keyframes.at(key).post_handle.x() : keyframes.at(key).pre_handle.x();

  // if this is the earliest/latest keyframe, no validation is required
  if (comp_key == -1) {
    return adjusted_key;
  }

  double comp = keyframes.at(comp_key).time.ToDouble() - keyframes.at(key).time.ToDouble();

  // if comp keyframe is bezier, validate with its accompanying handle
  if (keyframes.at(comp_key).type == EFFECT_KEYFRAME_BEZIER) {
    double relative_comp_handle = comp + (post ? keyframes.at(comp_key).pre_handle.x() : keyframes.at(comp_key).post_handle.x());
    // return an average
    if ((post && keyframes.at(key).post_handle.x() > relative_comp_handle)
        || (!post && keyframes.at(key).pre_handle.x() < relative_comp_handle)) {
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

void EffectField::GetKeyframeData(const rational& timecode, int &before, int &after, double &progress) {
  int before_keyframe_index = -1;
  int after_keyframe_index = -1;
  double before_keyframe_time = DBL_MIN;
  double after_keyframe_time = DBL_MAX;

  for (int i=0;i<keyframes.size();i++) {
    const rational& eval_keyframe_time = keyframes.at(i).time;
    if (eval_keyframe_time == timecode) {
      before = i;
      after = i;
      return;
    } else {

      double eval_keyframe_dbl = eval_keyframe_time.ToDouble();

      if (eval_keyframe_time < timecode && eval_keyframe_dbl > before_keyframe_time) {
        before_keyframe_index = i;
        before_keyframe_time = eval_keyframe_dbl;
      } else if (eval_keyframe_time > timecode && eval_keyframe_dbl < after_keyframe_time) {
        after_keyframe_index = i;
        after_keyframe_time = eval_keyframe_dbl;
      }
    }
  }

  if ((type_ == EFFECT_FIELD_DOUBLE || type_ == EFFECT_FIELD_COLOR)
      && (before_keyframe_index > -1 && after_keyframe_index > -1)) {
    // interpolate
    before = before_keyframe_index;
    after = after_keyframe_index;
    progress = (timecode.ToDouble()-before_keyframe_time)/(after_keyframe_time-before_keyframe_time);
  } else if (before_keyframe_index > -1) {
    before = before_keyframe_index;
    after = before_keyframe_index;
  } else {
    before = after_keyframe_index;
    after = after_keyframe_index;
  }
}

bool EffectField::HasKeyframes() {
  return (GetParentRow()->IsKeyframing() && !keyframes.isEmpty());
}

bool EffectField::IsEnabled() {
  return enabled_;
}

void EffectField::SetEnabled(bool e) {
  enabled_ = e;
  emit EnabledChanged(enabled_);
}
