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

#include "undo.h"

#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QCheckBox>
#include <QXmlStreamWriter>

#include "project/clip.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "ui/sourcetable.h"
#include "project/effect.h"
#include "project/transition.h"
#include "project/footage.h"
#include "rendering/renderfunctions.h"
#include "rendering/cacher.h"
#include "io/previewgenerator.h"
#include "ui/labelslider.h"
#include "ui/viewerwidget.h"
#include "project/marker.h"
#include "mainwindow.h"
#include "io/clipboard.h"
#include "project/media.h"
#include "debug.h"
#include "oliveglobal.h"

QUndoStack olive::UndoStack;

MoveClipAction::MoveClipAction(Clip *c, long iin, long iout, long iclip_in, int itrack, bool irelative) {
  clip = c;

  old_in = c->timeline_in();
  old_out = c->timeline_out();
  old_clip_in = c->clip_in();
  old_track = c->track();

  new_in = iin;
  new_out = iout;
  new_clip_in = iclip_in;
  new_track = itrack;

  relative = irelative;
}

void MoveClipAction::doUndo() {
  if (relative) {
    clip->set_timeline_in (clip->timeline_in() - new_in);
    clip->set_timeline_out (clip->timeline_out() - new_out);
    clip->set_clip_in (clip->clip_in() - new_clip_in);
    clip->set_track (clip->track() - new_track);
  } else {
    clip->set_timeline_in(old_in);
    clip->set_timeline_out(old_out);
    clip->set_clip_in(old_clip_in);
    clip->set_track(old_track);
  }
}

void MoveClipAction::doRedo() {
  if (relative) {
    clip->set_timeline_in(clip->timeline_in() + new_in);
    clip->set_timeline_out(clip->timeline_out() + new_out);
    clip->set_clip_in(clip->clip_in() + new_clip_in);
    clip->set_track(clip->track() + new_track);
  } else {
    clip->set_timeline_in(new_in);
    clip->set_timeline_out(new_out);
    clip->set_clip_in(new_clip_in);
    clip->set_track(new_track);
  }
}

DeleteClipAction::DeleteClipAction(Sequence *s, int clip) {
  seq = s;
  index = clip;
  opening_transition = -1;
  closing_transition = -1;
}

DeleteClipAction::~DeleteClipAction() {}

void DeleteClipAction::doUndo() {
  // restore ref to clip
  seq->clips[index] = ref;

  // restore links to this clip
  for (int i=linkClipIndex.size()-1;i>=0;i--) {
    seq->clips.at(linkClipIndex.at(i))->linked.insert(linkLinkIndex.at(i), index);
  }

  ref = nullptr;
}

void DeleteClipAction::doRedo() {
  // remove ref to clip
  ref = seq->clips.at(index);
  if (ref->IsOpen()) {
    ref->Close(true);
  }
  seq->clips[index] = nullptr;

  // delete link to this clip
  linkClipIndex.clear();
  linkLinkIndex.clear();
  for (int i=0;i<seq->clips.size();i++) {
    ClipPtr c = seq->clips.at(i);
    if (c != nullptr) {
      for (int j=0;j<c->linked.size();j++) {
        if (c->linked.at(j) == index) {
          linkClipIndex.append(i);
          linkLinkIndex.append(j);
          c->linked.removeAt(j);
        }
      }
    }
  }
}

ChangeSequenceAction::ChangeSequenceAction(Sequence *s) {
  new_sequence = s;
}

void ChangeSequenceAction::doUndo() {
  olive::Global->set_sequence(old_sequence);
}

void ChangeSequenceAction::doRedo() {
  old_sequence = olive::ActiveSequence;
  olive::Global->set_sequence(new_sequence);
}

SetTimelineInOutCommand::SetTimelineInOutCommand(Sequence* s, bool enabled, long in, long out) {
  seq = s;
  new_enabled = enabled;
  new_in = in;
  new_out = out;
}

void SetTimelineInOutCommand::doUndo() {
  seq->using_workarea = old_enabled;
  seq->workarea_in = old_in;
  seq->workarea_out = old_out;

  // footage viewer functions
  if (seq->wrapper_sequence) {
    Footage* m = seq->clips.at(0)->media()->to_footage();
    m->using_inout = old_enabled;
    m->in = old_in;
    m->out = old_out;
  }
}

void SetTimelineInOutCommand::doRedo() {
  old_enabled = seq->using_workarea;
  old_in = seq->workarea_in;
  old_out = seq->workarea_out;

  seq->using_workarea = new_enabled;
  seq->workarea_in = new_in;
  seq->workarea_out = new_out;

  // footage viewer functions
  if (seq->wrapper_sequence) {
    Footage* m = seq->clips.at(0)->media()->to_footage();
    m->using_inout = new_enabled;
    m->in = new_in;
    m->out = new_out;
  }
}

AddEffectCommand::AddEffectCommand(Clip* c, EffectPtr e, const EffectMeta *m, int insert_pos) {
  clip = c;
  ref = e;
  meta = m;
  pos = insert_pos;
  done = false;
}

void AddEffectCommand::doUndo() {
  clip->effects.last()->close();
  if (pos < 0) {
    clip->effects.removeLast();
  } else {
    clip->effects.removeAt(pos);
  }
  done = false;
}

void AddEffectCommand::doRedo() {
  if (ref == nullptr) {
    ref = Effect::Create(clip, meta);
  }
  if (pos < 0) {
    clip->effects.append(ref);
  } else {
    clip->effects.insert(pos, ref);
  }
  done = true;
}

AddTransitionCommand::AddTransitionCommand(Clip* iopen,
                                           Clip* iclose,
                                           TransitionPtr copy,
                                           const EffectMeta *itransition,
                                           int ilength) {
  open_ = iopen;
  close_ = iclose;
  transition_to_copy_ = copy;
  transition_meta_ = itransition;
  length_ = ilength;
  new_transition_ref_ = nullptr;
}

void AddTransitionCommand::doUndo() {
  if (open_ != nullptr) {
    open_->opening_transition = old_open_transition_;
  }

  if (close_ != nullptr)  {
    close_->closing_transition = old_close_transition_;
  }
}

void AddTransitionCommand::doRedo() {
  // convert open/close clips to primary/secondary for transition object
  Clip* primary = open_;
  Clip* secondary = close_;
  if (primary == nullptr) {
    primary = secondary;
    secondary = nullptr;
  }

  // create new transition object
  if (new_transition_ref_ == nullptr) {
    if (transition_to_copy_ == nullptr) {
      new_transition_ref_ = Transition::CreateFromMeta(primary, secondary, transition_meta_);
    } else {
      new_transition_ref_ = transition_to_copy_->copy(primary, nullptr);
    }
  }

  // set opening clip's opening transition to this and store the old one
  if (open_ != nullptr) {
    old_open_transition_ = open_->opening_transition;

    open_->opening_transition = new_transition_ref_;
  }

  // set closing clip's closing transition to this and store the old one
  if (close_ != nullptr) {
    old_close_transition_ = close_->closing_transition;

    close_->closing_transition = new_transition_ref_;
  }

  // if a length was specified, set it now
  if (length_ > 0) {
    new_transition_ref_->set_length(length_);
  }
}

ModifyTransitionCommand::ModifyTransitionCommand(TransitionPtr t, long ilength) {
  transition_ref_ = t;
  new_length_ = ilength;
  old_length_ = transition_ref_->get_true_length();
}

void ModifyTransitionCommand::doUndo() {
  transition_ref_->set_length(old_length_);
}

void ModifyTransitionCommand::doRedo() {

  transition_ref_->set_length(new_length_);
}

DeleteTransitionCommand::DeleteTransitionCommand(TransitionPtr t) {
  transition_ref_ = t;
}

void DeleteTransitionCommand::doUndo() {
  if (opened_clip_ != nullptr) {
    opened_clip_->opening_transition = transition_ref_;
  }

  if (closed_clip_ != nullptr) {
    closed_clip_->closing_transition = transition_ref_;
  }
}

void DeleteTransitionCommand::doRedo() {
  opened_clip_ = transition_ref_->get_opened_clip();
  closed_clip_ = transition_ref_->get_closed_clip();

  if (opened_clip_ != nullptr) {
    opened_clip_->opening_transition = nullptr;
  }

  if (closed_clip_ != nullptr) {
    closed_clip_->closing_transition = nullptr;
  }
}

NewSequenceCommand::NewSequenceCommand(Media *s, Media* iparent) {
  seq = s;
  parent = iparent;
  done = false;

  if (parent == nullptr) parent = olive::project_model.get_root();
}

NewSequenceCommand::~NewSequenceCommand() {
  if (!done) delete seq;
}

void NewSequenceCommand::doUndo() {
  olive::project_model.removeChild(parent, seq);

  done = false;
}

void NewSequenceCommand::doRedo() {
  olive::project_model.appendChild(parent, seq);

  done = true;
}

AddMediaCommand::AddMediaCommand(Media* iitem, Media *iparent) {
  item = iitem;
  parent = iparent;
  done = false;
}

AddMediaCommand::~AddMediaCommand() {
  if (!done) {
    delete item;
  }
}

void AddMediaCommand::doUndo() {
  olive::project_model.removeChild(parent, item);
  done = false;

}

void AddMediaCommand::doRedo() {
  olive::project_model.appendChild(parent, item);

  done = true;
}

DeleteMediaCommand::DeleteMediaCommand(Media* i) {
  item = i;
  parent = i->parentItem();
}

DeleteMediaCommand::~DeleteMediaCommand() {
  if (done) {
    delete item;
  }
}

void DeleteMediaCommand::doUndo() {
  olive::project_model.appendChild(parent, item);


  done = false;
}

void DeleteMediaCommand::doRedo() {
  olive::project_model.removeChild(parent, item);

  done = true;
}

AddClipCommand::AddClipCommand(Sequence *s, QVector<ClipPtr>& add) {
  link_offset_ = 0;
  seq = s;
  clips = add;
}

AddClipCommand::~AddClipCommand() {}

void AddClipCommand::doUndo() {
  // clear effects panel
  panel_effect_controls->Clear(true);

  for (int i=0;i<clips.size();i++) {
    ClipPtr c = seq->clips.last();

    if (c != nullptr) {
      // un-offset all the clips
      for (int j=0;j<c->linked.size();j++) {
        c->linked[j] -= link_offset_;
      }

      // deselect the area occupied by this clip
      panel_timeline->deselect_area(c->timeline_in(), c->timeline_out(), c->track());

      // if the clip is open, close it
      if (c->IsOpen()) {
        c->Close(true);
      }
    }

    // remove it from the sequence
    seq->clips.removeLast();
  }

}

void AddClipCommand::doRedo() {
  link_offset_ = seq->clips.size();
  for (int i=0;i<clips.size();i++) {
    ClipPtr original = clips.at(i);

    if (original != nullptr) {

      // offset all links by the current clip size
      for (int j=0;j<original->linked.size();j++) {
        original->linked[j] += link_offset_;
      }

    }

    seq->clips.append(original);
  }
}

LinkCommand::LinkCommand() {
  link = true;
}

void LinkCommand::doUndo() {
  for (int i=0;i<clips.size();i++) {
    ClipPtr c = s->clips.at(clips.at(i));
    if (link) {
      c->linked.clear();
    } else {
      c->linked = old_links.at(i);
    }
  }

}

void LinkCommand::doRedo() {
  old_links.clear();
  for (int i=0;i<clips.size();i++) {
    ClipPtr c = s->clips.at(clips.at(i));
    if (link) {
      for (int j=0;j<clips.size();j++) {
        if (i != j) {
          c->linked.append(clips.at(j));
        }
      }
    } else {
      old_links.append(c->linked);
      c->linked.clear();
    }
  }
}

CheckboxCommand::CheckboxCommand(QCheckBox* b) {
  box = b;
  checked = box->isChecked();
  done = true;
}

CheckboxCommand::~CheckboxCommand() {}

void CheckboxCommand::doUndo() {
  box->setChecked(!checked);
  done = false;

}

void CheckboxCommand::doRedo() {
  if (!done) {
    box->setChecked(checked);
  }
}

ReplaceMediaCommand::ReplaceMediaCommand(Media* i, QString s) {
  item = i;
  new_filename = s;
  old_filename = item->to_footage()->url;
}

void ReplaceMediaCommand::replace(QString& filename) {
  // close any clips currently using this media
  QVector<Media*> all_sequences = panel_project->list_all_project_sequences();
  for (int i=0;i<all_sequences.size();i++) {
    Sequence* s = all_sequences.at(i)->to_sequence();
    for (int j=0;j<s->clips.size();j++) {
      ClipPtr c = s->clips.at(j);
      if (c != nullptr && c->media() == item && c->IsOpen()) {
        c->Close(true);
        c->replaced = true;
      }
    }
  }

  // replace media
  QStringList files;
  files.append(filename);
  panel_project->process_file_list(files, false, item, nullptr);
  PreviewGenerator::AnalyzeMedia(item);
}

void ReplaceMediaCommand::doUndo() {
  replace(old_filename);


}

void ReplaceMediaCommand::doRedo() {
  replace(new_filename);
}

ReplaceClipMediaCommand::ReplaceClipMediaCommand(Media *a, Media *b, bool e) {
  old_media = a;
  new_media = b;
  preserve_clip_ins = e;
}

void ReplaceClipMediaCommand::replace(bool undo) {
  if (!undo) {
    old_clip_ins.clear();
  }

  for (int i=0;i<clips.size();i++) {
    ClipPtr c = clips.at(i);
    if (c->IsOpen()) {
      c->Close(true);
    }

    if (undo) {
      if (!preserve_clip_ins) {
        c->set_clip_in(old_clip_ins.at(i));
      }

      c->set_media(old_media, c->media_stream_index());
    } else {
      if (!preserve_clip_ins) {
        old_clip_ins.append(c->clip_in());
        c->set_clip_in(0);
      }

      c->set_media(new_media, c->media_stream_index());
    }

    c->replaced = true;
    c->refresh();
  }
}

void ReplaceClipMediaCommand::doUndo() {
  replace(true);
}

void ReplaceClipMediaCommand::doRedo() {
  replace(false);

  update_ui(true);
}

EffectDeleteCommand::EffectDeleteCommand(Effect *e) :
  effect_(e)
{}

void EffectDeleteCommand::doUndo() {
  parent_clip_->effects.insert(index_, deleted_obj_);

  panel_effect_controls->Reload();
}

void EffectDeleteCommand::doRedo() {
  parent_clip_ = effect_->parent_clip;

  index_ = parent_clip_->IndexOfEffect(effect_);

  Q_ASSERT(index_ > -1);

  effect_->close();
  deleted_obj_ = parent_clip_->effects.at(index_);
  parent_clip_->effects.removeAt(index_);

  panel_effect_controls->Reload();
}

MediaMove::MediaMove() {}

void MediaMove::doUndo() {
  for (int i=0;i<items.size();i++) {
    olive::project_model.moveChild(items.at(i), froms.at(i));
  }

}

void MediaMove::doRedo() {
  if (to == nullptr) to = olive::project_model.get_root();
  froms.resize(items.size());
  for (int i=0;i<items.size();i++) {
    Media* parent = items.at(i)->parentItem();
    froms[i] = parent;
    olive::project_model.moveChild(items.at(i), to);
  }
}

MediaRename::MediaRename(Media* iitem, QString ito) {
  item = iitem;
  from = iitem->get_name();
  to = ito;
}

void MediaRename::doUndo() {
  item->set_name(from);

}

void MediaRename::doRedo() {
  item->set_name(to);
}

KeyframeDelete::KeyframeDelete(EffectField *ifield, int iindex) {
  field = ifield;
  index = iindex;
}

void KeyframeDelete::doUndo() {
  field->keyframes.insert(index, deleted_key);
}

void KeyframeDelete::doRedo() {
  deleted_key = field->keyframes.at(index);
  field->keyframes.removeAt(index);
}

/*
EffectFieldUndo::EffectFieldUndo(EffectField* f, const QVariant& old_data, const QVariant new_data) :
  field(f),
  old_val(old_data),
  new_val(new_data)
{}

void EffectFieldUndo::doUndo() {
  field->SetCurrentValue(old_val);
}

void EffectFieldUndo::doRedo() {
  field->SetCurrentValue(new_val);
}
*/

SetClipProperty::SetClipProperty(SetClipPropertyType type) : type_(type)
{}

void SetClipProperty::AddSetting(QVector<Clip *> clips, bool setting)
{
  for (int i=0;i<clips.size();i++) {
    AddSetting(clips.at(i), setting);
  }
}

void SetClipProperty::AddSetting(Clip* c, bool setting)
{
  clips_.append(c);
  setting_.append(setting);

  // store current setting for undoing
  bool old_setting = false;
  switch (type_) {
  case kSetClipPropertyAutoscale:
    old_setting = c->autoscaled();
    break;
  case kSetClipPropertyReversed:
    old_setting = c->reversed();
    break;
  case kSetClipPropertyMaintainAudioPitch:
    old_setting = c->speed().maintain_audio_pitch;
    break;
  case kSetClipPropertyEnabled:
    old_setting = c->enabled();
    break;
  }
  old_setting_.append(old_setting);
}

void SetClipProperty::MainLoop(bool undo)
{
  for (int i=0;i<clips_.size();i++) {

    bool setting = (undo) ? old_setting_.at(i) : setting_.at(i);

    switch (type_) {
    case kSetClipPropertyAutoscale:
      clips_.at(i)->set_autoscaled(setting);
      break;
    case kSetClipPropertyReversed:
      clips_.at(i)->set_reversed(setting);
      break;
    case kSetClipPropertyMaintainAudioPitch:
    {
      ClipSpeed s = clips_.at(i)->speed();
      s.maintain_audio_pitch = setting;
      clips_.at(i)->set_speed(s);
    }
      break;
    case kSetClipPropertyEnabled:
      clips_.at(i)->set_enabled(setting);
      break;
    }
  }
}

void SetClipProperty::doUndo() {
  MainLoop(true);
  panel_sequence_viewer->viewer_widget->frame_update();

}

void SetClipProperty::doRedo() {
  MainLoop(false);
  panel_sequence_viewer->viewer_widget->frame_update();
}

AddMarkerAction::AddMarkerAction(QVector<Marker>* m, long t, QString n) {
  active_array = m;
  time = t;
  name = n;
}

void AddMarkerAction::doUndo() {
  if (index == -1) {
    active_array->removeLast();
  } else {
    active_array[0][index].name = old_name;
  }
}

void AddMarkerAction::doRedo() {
  index = -1;

  for (int i=0;i<active_array->size();i++) {
    if (active_array->at(i).frame == time) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    Marker m;
    m.frame = time;
    m.name = name;
    active_array->append(m);
  } else {
    old_name = active_array->at(index).name;
    active_array[0][index].name = name;
  }
}

MoveMarkerAction::MoveMarkerAction(Marker* m, long o, long n) {
  marker = m;
  old_time = o;
  new_time = n;
}

void MoveMarkerAction::doUndo() {
  marker->frame = old_time;

}

void MoveMarkerAction::doRedo() {
  marker->frame = new_time;
}

DeleteMarkerAction::DeleteMarkerAction(QVector<Marker> *m) {
  active_array = m;
  sorted = false;
}

void DeleteMarkerAction::doUndo() {
  for (int i=markers.size()-1;i>=0;i--) {
    active_array->insert(markers.at(i), copies.at(i));
  }

}

void DeleteMarkerAction::doRedo() {
  for (int i=0;i<markers.size();i++) {
    // correct future removals
    if (!sorted) {
      copies.append(active_array->at(markers.at(i)));
      for (int j=i+1;j<markers.size();j++) {
        if (markers.at(j) > markers.at(i)) {
          markers[j]--;
        }
      }
    }
    active_array->removeAt(markers.at(i));
  }
  sorted = true;
}

SetSpeedAction::SetSpeedAction(Clip* c, double speed) {
  clip = c;
  old_speed = c->speed().value;
  new_speed = speed;
}

void SetSpeedAction::doUndo() {
  ClipSpeed cs = clip->speed();

  cs.value = old_speed;

  clip->set_speed(cs);
}

void SetSpeedAction::doRedo() {
  ClipSpeed cs = clip->speed();

  cs.value = new_speed;

  clip->set_speed(cs);
}

SetBool::SetBool(bool* b, bool setting) {
  boolean = b;
  old_setting = *b;
  new_setting = setting;
}

void SetBool::doUndo() {
  *boolean = old_setting;
}

void SetBool::doRedo() {
  *boolean = new_setting;
}

SetSelectionsCommand::SetSelectionsCommand(Sequence *s) {
  seq = s;
  done = true;
}

void SetSelectionsCommand::doUndo() {
  seq->selections = old_data;
  done = false;
}

void SetSelectionsCommand::doRedo() {
  if (!done) {
    seq->selections = new_data;
    done = true;
  }
}

EditSequenceCommand::EditSequenceCommand(Media* i, Sequence *s) {
  item = i;
  seq = s;
  old_name = s->name;
  old_width = s->width;
  old_height = s->height;
  old_frame_rate = s->frame_rate;
  old_audio_frequency = s->audio_frequency;
  old_audio_layout = s->audio_layout;
}

void EditSequenceCommand::doUndo() {
  seq->name = old_name;
  seq->width = old_width;
  seq->height = old_height;
  seq->frame_rate = old_frame_rate;
  seq->audio_frequency = old_audio_frequency;
  seq->audio_layout = old_audio_layout;
  update();
}

void EditSequenceCommand::doRedo() {
  seq->name = name;
  seq->width = width;
  seq->height = height;
  seq->frame_rate = frame_rate;
  seq->audio_frequency = audio_frequency;
  seq->audio_layout = audio_layout;
  update();
}

void EditSequenceCommand::update() {
  // update tooltip
  // TODO/FIXME: this is a lousy way to do this, Media should have a separate fuctionn for updating the tooltip
  item->update_tooltip();

  for (int i=0;i<seq->clips.size();i++) {
    if (seq->clips.at(i) != nullptr) seq->clips.at(i)->refresh();
  }

  if (olive::ActiveSequence == seq) {
    olive::Global->set_sequence(seq);
  }
}

SetInt::SetInt(int* pointer, int new_value) {
  p = pointer;
  oldval = *pointer;
  newval = new_value;
}

void SetInt::doUndo() {
  *p = oldval;

}

void SetInt::doRedo() {
  *p = newval;
}

SetString::SetString(QString* pointer, QString new_value) {
  p = pointer;
  oldval = *pointer;
  newval = new_value;
}

void SetString::doUndo() {
  *p = oldval;

}

void SetString::doRedo() {
  *p = newval;
}

void CloseAllClipsCommand::doUndo() {
  redo();
}

void CloseAllClipsCommand::doRedo() {
  close_active_clips(olive::ActiveSequence);
}

UpdateFootageTooltip::UpdateFootageTooltip(Media *i) {
  item = i;
}

void UpdateFootageTooltip::doUndo() {
  redo();
}

void UpdateFootageTooltip::doRedo() {
  item->update_tooltip();
}

MoveEffectCommand::MoveEffectCommand() {}

void MoveEffectCommand::doUndo() {
  clip->effects.move(to, from);

}

void MoveEffectCommand::doRedo() {
  clip->effects.move(from, to);
}

RemoveClipsFromClipboard::RemoveClipsFromClipboard(int index) {
  pos = index;
  done = false;
}

RemoveClipsFromClipboard::~RemoveClipsFromClipboard() {}

void RemoveClipsFromClipboard::doUndo() {
  clipboard.insert(pos, clip);
  done = false;
}

void RemoveClipsFromClipboard::doRedo() {
  clip = std::static_pointer_cast<Clip>(clipboard.at(pos));
  clipboard.removeAt(pos);
  done = true;
}

RenameClipCommand::RenameClipCommand(Clip *clip, QString new_name)
{
  clip_ = clip;
  old_name_ = clip_->name();
  new_name_ = new_name;
}

void RenameClipCommand::doUndo() {
  clip_->set_name(old_name_);
}

void RenameClipCommand::doRedo() {
  clip_->set_name(new_name_);
}

SetPointer::SetPointer(void **pointer, void *data) {
  p = pointer;
  new_data = data;
}

void SetPointer::doUndo() {
  *p = old_data;
}

void SetPointer::doRedo() {
  old_data = *p;
  *p = new_data;
}

void ReloadEffectsCommand::doUndo() {
  redo();
}

void ReloadEffectsCommand::doRedo() {
  panel_effect_controls->Reload();
}

RippleAction::RippleAction(Sequence *is, long ipoint, long ilength, const QVector<int> &iignore) {
  s = is;
  point = ipoint;
  length = ilength;
  ignore = iignore;
}

void RippleAction::doUndo() {
  ca->undo();
  delete ca;
}

void RippleAction::doRedo() {
  ca = new ComboAction();
  for (int i=0;i<s->clips.size();i++) {
    if (!ignore.contains(i)) {
      ClipPtr c = s->clips.at(i);
      if (c != nullptr) {
        if (c->timeline_in() >= point) {
          c->move(ca, length, length, 0, 0, true, true);
        }
      }
    }
  }
  ca->redo();
}

SetDouble::SetDouble(double* pointer, double old_value, double new_value) {
  p = pointer;
  oldval = old_value;
  newval = new_value;
}

void SetDouble::doUndo() {
  *p = oldval;

}

void SetDouble::doRedo() {
  *p = newval;
}

SetQVariant::SetQVariant(QVariant *itarget, const QVariant &iold, const QVariant &inew) {
  target = itarget;
  old_val = iold;
  new_val = inew;
}

void SetQVariant::doUndo() {
  *target = old_val;
}

void SetQVariant::doRedo() {
  *target = new_val;
}

SetLong::SetLong(long *pointer, long old_value, long new_value) {
  p = pointer;
  oldval = old_value;
  newval = new_value;
}

void SetLong::doUndo() {
  *p = oldval;

}

void SetLong::doRedo() {
  *p = newval;
}

KeyframeFieldSet::KeyframeFieldSet(EffectField *ifield, int ii) {
  field = ifield;
  index = ii;
  key = ifield->keyframes.at(ii);
  done = true;
}

void KeyframeFieldSet::doUndo() {
  field->keyframes.removeAt(index);

  done = false;
}

void KeyframeFieldSet::doRedo() {
  if (!done) {
    field->keyframes.insert(index, key);
  }
  done = true;
}

SetIsKeyframing::SetIsKeyframing(EffectRow *irow, bool ib) {
  row = irow;
  b = ib;
}

void SetIsKeyframing::doUndo() {
  row->SetKeyframing(!b);
}

void SetIsKeyframing::doRedo() {
  row->SetKeyframing(b);
}

RefreshClips::RefreshClips(Media *m) {
  media = m;
}

void RefreshClips::doUndo() {
  redo();
}

void RefreshClips::doRedo() {
  // close any clips currently using this media
  QVector<Media*> all_sequences = panel_project->list_all_project_sequences();
  for (int i=0;i<all_sequences.size();i++) {
    Sequence* s = all_sequences.at(i)->to_sequence();
    for (int j=0;j<s->clips.size();j++) {
      ClipPtr c = s->clips.at(j);
      if (c != nullptr && c->media() == media) {
        c->replaced = true;
        c->refresh();
      }
    }
  }
}

void UpdateViewer::doUndo() {
  redo();
}

void UpdateViewer::doRedo() {
  panel_sequence_viewer->viewer_widget->frame_update();
}

SetEffectData::SetEffectData(EffectPtr e, const QByteArray &s) {
  effect = e;
  data = s;
}

void SetEffectData::doUndo() {
  effect->load_from_string(old_data);

  old_data.clear();
}

void SetEffectData::doRedo() {
  old_data = effect->save_to_string();

  effect->load_from_string(data);
}

OliveAction::OliveAction(bool iset_window_modified) {
  set_window_modified = iset_window_modified;
}

OliveAction::~OliveAction() {}

void OliveAction::undo() {
  doUndo();

  if (set_window_modified) {
    olive::MainWindow->setWindowModified(old_window_modified);
  }
}

void OliveAction::redo() {
  doRedo();

  if (set_window_modified) {

    // store current modified state
    old_window_modified = olive::MainWindow->isWindowModified();

    // set modified to true
    olive::MainWindow->setWindowModified(true);

  }
}
