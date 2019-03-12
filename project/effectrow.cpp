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

#include "effectrow.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>

#include "project/undo.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/grapheditor.h"
#include "effect.h"
#include "ui/viewerwidget.h"
#include "ui/keyframenavigator.h"
#include "ui/clickablelabel.h"

EffectRow::EffectRow(Effect *parent, const QString &n, bool savable, bool keyframable) :
  QObject(parent),
  name_(n),
  keyframable_(keyframable),
  keyframing_(false),
  savable_(savable)
{
  Q_ASSERT(parent != nullptr);

  parent->AddRow(this);
}

void EffectRow::AddField(EffectField *field)
{
  field->setParent(this);
  fields_.append(field);
}

bool EffectRow::IsKeyframing() {
  return keyframing_;
}

void EffectRow::SetKeyframing(bool b) {
  if (GetParentEffect()->meta->type != EFFECT_TYPE_TRANSITION) {
    keyframing_ = b;
    emit KeyframingSetChanged(keyframing_);
  }
}

bool EffectRow::IsSavable()
{
  return savable_;
}

void EffectRow::SetKeyframingEnabled(bool enabled) {
  if (enabled) {
    ComboAction* ca = new ComboAction();
    ca->append(new SetIsKeyframing(this, true));
    set_keyframe_now(ca);
    olive::UndoStack.push(ca);
  } else {
    if (QMessageBox::question(panel_effect_controls,
                              tr("Disable Keyframes"),
                              tr("Disabling keyframes will delete all current keyframes. Are you sure you want to do this?"),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
      // clear
      ComboAction* ca = new ComboAction();
      for (int i=0;i<FieldCount();i++) {
        EffectField* f = Field(i);
        for (int j=0;j<f->keyframes.size();j++) {
          ca->append(new KeyframeDelete(f, 0));
        }
      }
      ca->append(new SetIsKeyframing(this, false));
      olive::UndoStack.push(ca);
      panel_effect_controls->update_keyframes();
    } else {
      SetKeyframing(true);
    }
  }
}

void EffectRow::GoToPreviousKeyframe() {
  long key = LONG_MIN;
  Clip* c = GetParentEffect()->parent_clip;
  for (int i=0;i<FieldCount();i++) {
    EffectField* f = Field(i);
    for (int j=0;j<f->keyframes.size();j++) {
      long comp = f->keyframes.at(j).time - c->clip_in() + c->timeline_in();
      if (comp < olive::ActiveSequence->playhead) {
        key = qMax(comp, key);
      }
    }
  }
  if (key != LONG_MIN) panel_sequence_viewer->seek(key);
}

void EffectRow::ToggleKeyframe() {
  QVector<EffectField*> key_fields;
  QVector<int> key_field_index;
  Clip* c = GetParentEffect()->parent_clip;
  for (int j=0;j<FieldCount();j++) {
    EffectField* f = Field(j);
    for (int i=0;i<f->keyframes.size();i++) {
      long comp = c->timeline_in() - c->clip_in() + f->keyframes.at(i).time;
      if (comp == olive::ActiveSequence->playhead) {
        key_fields.append(f);
        key_field_index.append(i);
      }
    }
  }

  ComboAction* ca = new ComboAction();
  if (key_fields.size() == 0) {
    // keyframe doesn't exist, set one
    set_keyframe_now(ca);
  } else {
    for (int i=0;i<key_fields.size();i++) {
      ca->append(new KeyframeDelete(key_fields.at(i), key_field_index.at(i)));
    }
  }
  olive::UndoStack.push(ca);
  update_ui(false);
}

void EffectRow::GoToNextKeyframe() {
  long key = LONG_MAX;
  Clip* c = GetParentEffect()->parent_clip;
  for (int i=0;i<FieldCount();i++) {
    EffectField* f = Field(i);
    for (int j=0;j<f->keyframes.size();j++) {
      long comp = f->keyframes.at(j).time - c->clip_in() + c->timeline_in();
      if (comp > olive::ActiveSequence->playhead) {
        key = qMin(comp, key);
      }
    }
  }
  if (key != LONG_MAX) panel_sequence_viewer->seek(key);
}

void EffectRow::FocusRow() {
  panel_graph_editor->set_row(this);
}

void EffectRow::set_keyframe_now(ComboAction* ca) {
  // TODO address this...
  /*
  long time = olive::ActiveSequence->playhead
      - parent_effect->parent_clip->timeline_in()
      + parent_effect->parent_clip->clip_in();

  if (!just_made_unsafe_keyframe) {
    EffectKeyframe key;
    key.time = time;

    unsafe_keys.resize(fieldCount());
    unsafe_old_data.resize(fieldCount());
    key_is_new.resize(fieldCount());

    for (int i=0;i<fieldCount();i++) {
      EffectField* f = field(i);

      int exist_key = -1;
      int closest_key = 0;
      for (int j=0;j<f->keyframes.size();j++) {
        if (f->keyframes.at(j).time == time) {
          exist_key = j;
        } else if (f->keyframes.at(j).time < time
               && f->keyframes.at(closest_key).time < f->keyframes.at(j).time) {
          closest_key = j;
        }
      }
      if (exist_key == -1) {
        key.type = (f->keyframes.size() == 0) ? EFFECT_KEYFRAME_LINEAR : f->keyframes.at(closest_key).type;
        key.data = f->GetCurrentValue();
        unsafe_keys[i] = f->keyframes.size();
        f->keyframes.append(key);
        key_is_new[i] = true;
      } else {
        unsafe_keys[i] = exist_key;
        key_is_new[i] = false;
      }
      unsafe_old_data[i] = f->GetCurrentValue();
    }
    just_made_unsafe_keyframe = true;
  }

  for (int i=0;i<fieldCount();i++) {
    field(i)->keyframes[unsafe_keys.at(i)].data = field(i)->GetCurrentValue();
  }

  if (ca != nullptr)	{
    for (int i=0;i<fieldCount();i++) {
      if (key_is_new.at(i)) ca->append(new KeyframeFieldSet(field(i), unsafe_keys.at(i)));
      ca->append(new SetQVariant(&field(i)->keyframes[unsafe_keys.at(i)].data, unsafe_old_data.at(i), field(i)->GetCurrentValue()));
    }
    unsafe_keys.clear();
    unsafe_old_data.clear();
    just_made_unsafe_keyframe = false;
  }

  panel_effect_controls->update_keyframes();
  */
}

void EffectRow::delete_keyframe_at_time(ComboAction* ca, long time) {
  for (int j=0;j<FieldCount();j++) {
    EffectField* f = Field(j);
    for (int i=0;i<f->keyframes.size();i++) {
      if (f->keyframes.at(i).time == time) {
        ca->append(new KeyframeDelete(f, i));
        break;
      }
    }
  }
}

Effect *EffectRow::GetParentEffect()
{
  return static_cast<Effect*>(parent());
}

const QString &EffectRow::name() {
  return name_;
}

EffectField* EffectRow::Field(int i) {
  return fields_.at(i);
}

int EffectRow::FieldCount() {
  return fields_.size();
}
