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

#include "global/global.h"
#include "panels/panels.h"
#include "panels/project.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "ui/sourcetable.h"
#include "project/footage.h"
#include "rendering/renderfunctions.h"
#include "rendering/cacher.h"
#include "ui/labelslider.h"
#include "ui/viewerwidget.h"
#include "project/media.h"
#include "global/clipboard.h"
#include "project/previewgenerator.h"
#include "ui/mainwindow.h"

MoveClipAction::MoveClipAction(ClipPtr c, long iin, long iout, long iclip_in, Track* itrack, bool irelative) :
  clip(c),
  old_in(c->timeline_in()),
  old_out(c->timeline_out()),
  old_clip_in(c->clip_in()),
  old_track(c->track()),
  new_in(iin),
  new_out(iout),
  new_clip_in(iclip_in),
  new_track(itrack),
  relative(irelative),
  done(false)
{
  doRedo();
}

void MoveClipAction::doUndo() {
  if (relative) {
    clip->set_timeline_in (clip->timeline_in() - new_in);
    clip->set_timeline_out (clip->timeline_out() - new_out);
    clip->set_clip_in (clip->clip_in() - new_clip_in);
  } else {
    clip->set_timeline_in(old_in);
    clip->set_timeline_out(old_out);
    clip->set_clip_in(old_clip_in);
  }

  // Move clip to the new track ONLY IF the old track currently contains this clip - a workaround to ensure this
  // action doesn't accidentaly add a clip that it's not supposed to
  if (new_track->ContainsClip(clip.get())) {
    old_track->AddClip(clip);
  }

  done = false;
}

void MoveClipAction::doRedo() {
  if (!done) {
    if (relative) {
      clip->set_timeline_in(clip->timeline_in() + new_in);
      clip->set_timeline_out(clip->timeline_out() + new_out);
      clip->set_clip_in(clip->clip_in() + new_clip_in);
    } else {
      clip->set_timeline_in(new_in);
      clip->set_timeline_out(new_out);
      clip->set_clip_in(new_clip_in);
    }

    // Move clip to the new track ONLY IF the old track currently contains this clip - a workaround to ensure this
    // action doesn't accidentaly add a clip that it's not supposed to
    if (old_track->ContainsClip(clip.get())) {
      new_track->AddClip(clip);
    }

    done = true;
  }
}

DeleteClipAction::DeleteClipAction(Clip *clip) :
  done_(false)
{
  // Get shared_ptr object to take ownership of this Clip

  clip_ = clip->track()->GetClipObjectFromRawPtr(clip);
  doRedo();
}

void DeleteClipAction::doUndo() {
  // restore clip to this track
  clip_->track()->AddClip(clip_);

  // restore links to this clip
  for (int i=0;i<clips_linked_to_this_one_.size();i++) {
    clips_linked_to_this_one_.at(i)->linked.append(clip_.get());
  }

  done_ = false;
}

void DeleteClipAction::doRedo() {
  if (!done_) {

    // remove ref to clip
    if (clip_->IsOpen()) {
      clip_->Close(true);
    }

    clip_->track()->RemoveClip(clip_.get());

    // delete link to this clip
    QVector<Clip*> clips = clip_->track()->sequence()->GetAllClips();
    for (int i=0;i<clips.size();i++) {
      Clip* c = clips.at(i);

      for (int j=0;j<c->linked.size();j++) {
        if (c->linked.at(j) == clip_.get()) {
          c->linked.removeAt(j);
          clips_linked_to_this_one_.append(c);
          break;
        }
      }
    }

    done_ = true;

  }
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
    Footage* m = seq->GetAllClips().first()->media()->to_footage();
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
    Footage* m = seq->GetAllClips().first()->media()->to_footage();
    m->using_inout = new_enabled;
    m->in = new_in;
    m->out = new_out;
  }
}

AddEffectCommand::AddEffectCommand(Clip* c, OldEffectNodePtr e, NodeType m, int insert_pos) {
  clip = c;
  ref = e;
  meta = m;
  pos = insert_pos;
}

void AddEffectCommand::doUndo() {
  clip->effects.last()->close();
  if (pos < 0) {
    clip->effects.removeLast();
  } else {
    clip->effects.removeAt(pos);
  }
}

void AddEffectCommand::doRedo() {
  if (ref == nullptr) {
    ref = olive::node_library[meta]->Create(clip);
  }
  if (pos < 0) {
    clip->effects.append(ref);
  } else {
    clip->effects.insert(pos, ref);
  }
}

AddTransitionCommand::AddTransitionCommand(Clip* iopen,
                                           Clip* iclose,
                                           TransitionPtr copy,
                                           NodeType itransition,
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
      new_transition_ref_ = std::static_pointer_cast<Transition>(olive::node_library[transition_meta_]->Create(primary));
      new_transition_ref_->secondary_clip = secondary;
    } else {
      new_transition_ref_ = std::static_pointer_cast<Transition>(transition_to_copy_->copy(primary));
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

DeleteTransitionCommand::DeleteTransitionCommand(TransitionPtr t) :
  transition_ref_(t)
{
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

AddMediaCommand::AddMediaCommand(MediaPtr iitem, Media *iparent) :
  item(iitem),
  parent(iparent),
  done_(false)
{
  doRedo();
}

void AddMediaCommand::doUndo() {
  olive::project_model.removeChild(parent, item.get());
  done_ = false;
}

void AddMediaCommand::doRedo() {
  if (!done_) {
    olive::project_model.appendChild(parent, item);
    done_ = true;
  }
}

DeleteMediaCommand::DeleteMediaCommand(MediaPtr i) :
  item(i),
  parent(i->parentItem())
{
}

void DeleteMediaCommand::doUndo() {
  olive::project_model.appendChild(parent, item);
}

void DeleteMediaCommand::doRedo() {
  olive::project_model.removeChild(parent, item.get());
}

AddClipCommand::AddClipCommand(const QVector<ClipPtr> &add) :
  clips_(add),
  done_(false)
{
  doRedo();
}

void AddClipCommand::doUndo() {
  // clear effects panel
  panel_graph_editor->set_row(nullptr);
  panel_effect_controls->Clear(true);

  for (int i=0;i<clips_.size();i++) {
    ClipPtr c = clips_.at(i);

    if (c != nullptr) {

      c->track()->RemoveClip(c.get());

      // deselect the area occupied by this clip
      c->track()->DeselectArea(c->timeline_in(), c->timeline_out());

      // if the clip is open, close it
      if (c->IsOpen()) {
        c->Close(true);
      }
    }
  }

  done_ = false;
}

void AddClipCommand::doRedo() {
  if (!done_) {
    for (int i=0;i<clips_.size();i++) {
      ClipPtr original = clips_.at(i);

      if (original != nullptr) {
        original->track()->AddClip(original);
      }      
    }
    done_ = true;
  }
}

LinkCommand::LinkCommand(const QVector<Clip*>& clips, bool link) :
  clips_(clips),
  link_(link)
{
}

void LinkCommand::doUndo() {
  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);
    if (link_) {
      c->linked.clear();
    } else {
      c->linked = old_links_.at(i);
    }
  }

}

void LinkCommand::doRedo() {
  old_links_.clear();
  for (int i=0;i<clips_.size();i++) {
    Clip* c = clips_.at(i);
    if (link_) {

      for (int j=0;j<clips_.size();j++) {
        if (i != j) {
          c->linked.append(clips_.at(j));
        }
      }

    } else {

      old_links_.append(c->linked);
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

ReplaceMediaCommand::ReplaceMediaCommand(MediaPtr i, QString s) {
  item = i;
  new_filename = s;
  old_filename = item->to_footage()->url;
}

void ReplaceMediaCommand::replace(QString& filename) {
  // close any clips currently using this media
  QVector<Media*> all_sequences = olive::project_model.GetAllSequences();
  for (int i=0;i<all_sequences.size();i++) {

    QVector<Clip*> sequence_clips = all_sequences.at(i)->to_sequence()->GetAllClips();

    for (int j=0;j<sequence_clips.size();j++) {
      Clip* c = sequence_clips.at(j);
      if (c->media() == item.get() && c->IsOpen()) {
        c->Close(true);
        c->replaced = true;
      }
    }
  }

  // replace media
  QStringList files;
  files.append(filename);
  olive::project_model.process_file_list(files, false, item, nullptr);
  PreviewGenerator::AnalyzeMedia(item.get());
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
    Clip* c = clips.at(i);
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

EffectDeleteCommand::EffectDeleteCommand(OldEffectNode *e) :
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

KeyframeDelete::KeyframeDelete(EffectField *ifield, int iindex) :
  field(ifield),
  index(iindex)
{
}

void KeyframeDelete::doUndo() {
  field->keyframes.insert(index, deleted_key);
}

void KeyframeDelete::doRedo() {
  deleted_key = field->keyframes.at(index);
  field->keyframes.removeAt(index);
}

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
  panel_sequence_viewer->viewer_widget()->frame_update();

}

void SetClipProperty::doRedo() {
  MainLoop(false);
  panel_sequence_viewer->viewer_widget()->frame_update();
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

SetSelectionsCommand::SetSelectionsCommand(Sequence *s,
                                           const QVector<Selection> &old_data,
                                           const QVector<Selection> &new_data) :
  seq_(s),
  old_data_(old_data),
  new_data_(new_data)
{

}

void SetSelectionsCommand::doUndo() {
  seq_->SetSelections(old_data_);
}

void SetSelectionsCommand::doRedo() {
  seq_->SetSelections(new_data_);
}

EditSequenceCommand::EditSequenceCommand(Media* i, SequencePtr s) {
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
  // Update sequence's tooltip
  item->update_tooltip();

  QVector<Clip*> all_clips = seq->GetAllClips();
  for (int i=0;i<all_clips.size();i++) {
    if (all_clips.at(i) != nullptr) {
      all_clips.at(i)->refresh();
    }
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
  QVector<Media*> sequences = olive::project_model.GetAllSequences();

  for (int i=0;i<sequences.size();i++) {
    sequences.at(i)->to_sequence()->Close();
  }
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
  olive::clipboard.Insert(pos, clip);
  done = false;
}

void RemoveClipsFromClipboard::doRedo() {
  clip = std::static_pointer_cast<Clip>(olive::clipboard.Get(pos));
  olive::clipboard.RemoveAt(pos);
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

RippleAction::RippleAction(Sequence *is, long ipoint, long ilength, const QVector<Clip*> &iignore) :
  s(is),
  point(ipoint),
  length(ilength),
  ignore(iignore)
{
}

void RippleAction::doUndo() {
  ca->undo();
  delete ca;
}

void RippleAction::doRedo() {
  ca = new ComboAction();

  QVector<Clip*> all_clips = s->GetAllClips();

  for (int i=0;i<all_clips.size();i++) {
    Clip* c = all_clips.at(i);
    if (!ignore.contains(c)) {
      if (c->timeline_in() >= point) {
        s->MoveClip(c,
                    ca,
                    length,
                    length,
                    0,
                    c->track(),
                    true,
                    true);
      }
    }
  }
  ca->redo();
}

SetDouble::SetDouble(double* pointer, double old_value, double new_value) :
  p(pointer),
  oldval(old_value),
  newval(new_value)
{
}

void SetDouble::doUndo() {
  *p = oldval;

}

void SetDouble::doRedo() {
  *p = newval;
}

SetQVariant::SetQVariant(QVariant *itarget, const QVariant &iold, const QVariant &inew) :
  target(itarget),
  old_val(iold),
  new_val(inew)
{
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

KeyframeAdd::KeyframeAdd(EffectField *ifield, int ii) :
  field(ifield),
  index(ii),
  key(ifield->keyframes.at(ii)),
  done(true)
{
}

void KeyframeAdd::doUndo() {
  field->keyframes.removeAt(index);
  done = false;
}

void KeyframeAdd::doRedo() {
  if (!done) {
    field->keyframes.insert(index, key);
    done = true;
  }
}

SetIsKeyframing::SetIsKeyframing(NodeIO *irow, bool ib) {
  row = irow;
  b = ib;
}

void SetIsKeyframing::doUndo() {
  row->SetKeyframingInternal(!b);
}

void SetIsKeyframing::doRedo() {
  row->SetKeyframingInternal(b);
}

RefreshClips::RefreshClips(Media *m) :
  media(m)
{
}

void RefreshClips::doUndo() {
  redo();
}

void RefreshClips::doRedo() {
  // close any clips currently using this media
  QVector<Media*> all_sequences = olive::project_model.GetAllSequences();
  for (int i=0;i<all_sequences.size();i++) {

    QVector<Clip*> sequence_clips = all_sequences.at(i)->to_sequence().get()->GetAllClips();

    for (int j=0;j<sequence_clips.size();j++) {
      Clip* c = sequence_clips.at(j);
      if (c->media() == media || media == nullptr) {
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
  panel_sequence_viewer->viewer_widget()->frame_update();
}

SetEffectData::SetEffectData(OldEffectNode *e, const QByteArray &s) {
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
    olive::Global->set_modified(old_window_modified);
  }
}

void OliveAction::redo() {
  doRedo();

  if (set_window_modified) {

    // store current modified state
    old_window_modified = olive::Global->is_modified();

    // set modified to true
    olive::Global->set_modified(true);

  }
}

KeyframeDataChange::KeyframeDataChange(EffectField *field) :
  field_(field),
  done_(true)
{
  old_keys_ = field_->keyframes;
  old_persistent_data_ = field_->persistent_data_;
}

void KeyframeDataChange::SetNewKeyframes()
{
  new_keys_ = field_->keyframes;
  new_persistent_data_ = field_->persistent_data_;
}

void KeyframeDataChange::doUndo()
{
  field_->keyframes = old_keys_;
  field_->persistent_data_ = old_persistent_data_;
  done_ = false;
}

void KeyframeDataChange::doRedo()
{
  if (!done_) {
    field_->keyframes = new_keys_;
    field_->persistent_data_ = new_persistent_data_;
    done_ = true;
  }
}
