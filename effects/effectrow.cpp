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

#include "undo/undo.h"
#include "undo/undostack.h"
#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/grapheditor.h"
#include "effect.h"
#include "ui/viewerwidget.h"
#include "ui/keyframenavigator.h"
#include "ui/clickablelabel.h"

EffectRow::EffectRow(Effect *parent, const QString &id, const QString &name, bool savable, bool keyframable) :
  QObject(parent),
  id_(id),
  name_(name),
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

  connect(field, SIGNAL(Clicked()), this, SIGNAL(Clicked()));
  connect(field, SIGNAL(Changed()), this, SIGNAL(Changed()));

  fields_.append(field);
}

void EffectRow::AddNodeInput(olive::nodes::DataType type)
{
  accepted_datatypes_.append(type);
}

bool EffectRow::IsKeyframing() {
  return keyframing_;
}

void EffectRow::SetKeyframingInternal(bool b) {
  if (GetParentEffect()->meta->type != EFFECT_TYPE_TRANSITION) {
    keyframing_ = b;
    emit KeyframingSetChanged(keyframing_);
  }
}

bool EffectRow::IsSavable()
{
  return savable_;
}

bool EffectRow::IsKeyframable()
{
  return keyframable_;
}

QVariant EffectRow::GetValueAt(double timecode)
{
  Q_ASSERT(FieldCount() == 1);

  return Field(0)->GetValueAt(timecode);
}

void EffectRow::SetValueAt(double timecode, const QVariant &value)
{
  Q_ASSERT(FieldCount() == 1);

  Field(0)->SetValueAt(timecode, value);
}

void EffectRow::SetEnabled(bool enabled)
{
  for (int i=0;i<FieldCount();i++) {
    Field(i)->SetEnabled(enabled);
  }
}

bool EffectRow::CanConnectNodes()
{
  return !accepted_datatypes_.isEmpty();
}

void EffectRow::SetKeyframingEnabled(bool enabled) {
  if (enabled == keyframing_) {
    return;
  }

  if (enabled) {

    ComboAction* ca = new ComboAction();

    // Enable keyframing setting on this row
    ca->append(new SetIsKeyframing(this, true));

    // Prepare each field's data to start keyframing
    for (int i=0;i<FieldCount();i++) {
      Field(i)->PrepareDataForKeyframing(true, ca);
    }

    olive::undo_stack.push(ca);

    update_ui(false);

  } else {

    // Confirm with the user whether they really want to disable keyframing
    if (QMessageBox::question(panel_effect_controls,
                              tr("Disable Keyframes"),
                              tr("Disabling keyframes will delete all current keyframes. "
                                 "Are you sure you want to do this?"),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {

      ComboAction* ca = new ComboAction();

      // Prepare each field's data to stop keyframing
      for (int i=0;i<FieldCount();i++) {
        Field(i)->PrepareDataForKeyframing(false, ca);
      }

      // Disable keyframing setting on this row
      ca->append(new SetIsKeyframing(this, false));

      olive::undo_stack.push(ca);

      update_ui(false);

    } else {

      SetKeyframingInternal(true);

    }
  }
}

void EffectRow::GoToPreviousKeyframe() {
  long key = LONG_MIN;
  Clip* c = GetParentEffect()->parent_clip;
  long sequence_playhead = c->track()->sequence()->playhead;

  // Used to convert clip frame number to sequence frame number
  long time_adjustment = c->timeline_in() - c->clip_in();

  // Loop through all of this row's fields
  for (int i=0;i<FieldCount();i++) {

    EffectField* f = Field(i);

    // Loop through all of this field's keyframes for a keyframe EARLIER than the playhead
    for (int j=0;j<f->keyframes.size();j++) {
      long comp = f->keyframes.at(j).time + time_adjustment;

      // Get the closest keyframe
      if (comp < sequence_playhead) {
        key = qMax(comp, key);
      }
    }
  }

  // If we found a keyframe less than the playhead, jump to it
  if (key != LONG_MIN) panel_sequence_viewer->seek(key);
}

void EffectRow::ToggleKeyframe() {
  Clip* c = GetParentEffect()->parent_clip;
  long sequence_playhead = c->track()->sequence()->playhead;

  // Used to convert clip frame number to sequence frame number
  long time_adjustment = c->timeline_in() - c->clip_in();

  QVector<EffectField*> key_fields;
  QVector<int> key_field_index;

  // See if any keyframes on any fields are at the current time
  for (int j=0;j<FieldCount();j++) {
    EffectField* f = Field(j);
    for (int i=0;i<f->keyframes.size();i++) {
      long comp = f->keyframes.at(i).time + time_adjustment;

      if (comp == sequence_playhead) {

        // Cache the keyframes if they are at the current time
        key_fields.append(f);
        key_field_index.append(i);

      }
    }
  }

  ComboAction* ca = new ComboAction();

  if (key_fields.isEmpty()) {

    // If we didn't find any current keyframes, create one for each field
    SetKeyframeOnAllFields(ca);

  } else {

    // If we DID find keyframes at this time, delete them

    QVector<EffectField*> sorted_key_fields;
    QVector<int> sorted_key_field_index;

    // Since QVectors shift themselves when removing items, we need to sort these in reverse order
    for (int i=0;i<key_field_index.size();i++) {
      bool found = false;

      for (int j=0;j<sorted_key_field_index.size();j++) {
        if (sorted_key_field_index.at(j) < key_field_index.at(i)) {
          sorted_key_fields.insert(j, key_fields.at(i));
          sorted_key_field_index.insert(j, key_field_index.at(i));

          found = true;
          break;
        }
      }

      if (!found) {
        sorted_key_fields.append(key_fields.at(i));
        sorted_key_field_index.append(key_field_index.at(i));
      }
    }

    for (int i=0;i<sorted_key_fields.size();i++) {
      ca->append(new KeyframeDelete(sorted_key_fields.at(i), sorted_key_field_index.at(i)));
    }

  }

  olive::undo_stack.push(ca);
  update_ui(false);
}

void EffectRow::GoToNextKeyframe() {
  long key = LONG_MAX;
  Clip* c = GetParentEffect()->parent_clip;
  for (int i=0;i<FieldCount();i++) {
    EffectField* f = Field(i);
    for (int j=0;j<f->keyframes.size();j++) {
      long comp = f->keyframes.at(j).time - c->clip_in() + c->timeline_in();
      if (comp > c->track()->sequence()->playhead) {
        key = qMin(comp, key);
      }
    }
  }
  if (key != LONG_MAX) panel_sequence_viewer->seek(key);
}

void EffectRow::FocusRow() {
  panel_graph_editor->set_row(this);
}

void EffectRow::SetKeyframeOnAllFields(ComboAction* ca) {
  for (int i=0;i<FieldCount();i++) {
    EffectField* field = Field(i);

    KeyframeDataChange* kdc = new KeyframeDataChange(field);

    field->SetValueAt(GetParentEffect()->Now(), field->GetValueAt(GetParentEffect()->Now()));

    kdc->SetNewKeyframes();
    ca->append(kdc);
  }

  panel_effect_controls->update_keyframes();
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
