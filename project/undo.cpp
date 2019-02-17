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
#include "playback/playback.h"
#include "ui/sourcetable.h"
#include "project/effect.h"
#include "project/transition.h"
#include "project/footage.h"
#include "playback/cacher.h"
#include "ui/labelslider.h"
#include "ui/viewerwidget.h"
#include "project/marker.h"
#include "mainwindow.h"
#include "io/clipboard.h"
#include "project/media.h"
#include "debug.h"

QUndoStack olive::UndoStack;

MoveClipAction::MoveClipAction(ClipPtr c, long iin, long iout, long iclip_in, int itrack, bool irelative) {
    clip = c;

    old_in = c->timeline_in;
    old_out = c->timeline_out;
    old_clip_in = c->clip_in;
    old_track = c->track;

    new_in = iin;
    new_out = iout;
    new_clip_in = iclip_in;
    new_track = itrack;

    relative = irelative;
}

void MoveClipAction::doUndo() {
	if (relative) {
		clip->timeline_in -= new_in;
		clip->timeline_out -= new_out;
        clip->clip_in -= new_clip_in;
		clip->track -= new_track;
	} else {
		clip->timeline_in = old_in;
		clip->timeline_out = old_out;
		clip->clip_in = old_clip_in;
		clip->track = old_track;
    }
}

void MoveClipAction::doRedo() {
	if (relative) {
		clip->timeline_in += new_in;
		clip->timeline_out += new_out;
		clip->clip_in += new_clip_in;
		clip->track += new_track;
	} else {
		clip->timeline_in = new_in;
		clip->timeline_out = new_out;
		clip->clip_in = new_clip_in;
		clip->track = new_track;
    }
}

DeleteClipAction::DeleteClipAction(SequencePtr s, int clip) {
    seq = s;
    index = clip;
    opening_transition = -1;
    closing_transition = -1;
}

DeleteClipAction::~DeleteClipAction() {}

void DeleteClipAction::doUndo() {
	// restore ref to clip
	seq->clips[index] = ref;

	// restore shared transitions
	if (opening_transition > -1) {
		seq->transitions.at(opening_transition)->secondary_clip = seq->transitions.at(opening_transition)->parent_clip;
		seq->transitions.at(opening_transition)->parent_clip = ref;
		ref->opening_transition = opening_transition;
		opening_transition = -1;
	}
	if (closing_transition > -1) {
		seq->transitions.at(closing_transition)->secondary_clip = ref;
		ref->closing_transition = closing_transition;
		closing_transition = -1;
	}

	// restore links to this clip
	for (int i=linkClipIndex.size()-1;i>=0;i--) {
		seq->clips.at(linkClipIndex.at(i))->linked.insert(linkLinkIndex.at(i), index);
	}

    ref = nullptr;
}

void DeleteClipAction::doRedo() {
	// remove ref to clip
	ref = seq->clips.at(index);
	if (ref->open) {
		close_clip(ref, true);
	}
	seq->clips[index] = nullptr;

	// save shared transitions
	if (ref->opening_transition > -1 && ref->get_opening_transition()->secondary_clip != nullptr) {
		opening_transition = ref->opening_transition;
		ref->get_opening_transition()->parent_clip = ref->get_opening_transition()->secondary_clip;
		ref->get_opening_transition()->secondary_clip = nullptr;
		ref->opening_transition = -1;
	}
	if (ref->closing_transition > -1 && ref->get_closing_transition()->secondary_clip != nullptr) {
		closing_transition = ref->closing_transition;
		ref->get_closing_transition()->secondary_clip = nullptr;
		ref->closing_transition = -1;
	}

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

ChangeSequenceAction::ChangeSequenceAction(SequencePtr s) {
    new_sequence = s;
}

void ChangeSequenceAction::doUndo() {
	set_sequence(old_sequence);
}

void ChangeSequenceAction::doRedo() {
	old_sequence = olive::ActiveSequence;
	set_sequence(new_sequence);
}

SetTimelineInOutCommand::SetTimelineInOutCommand(SequencePtr s, bool enabled, long in, long out) {
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
        FootagePtr m = seq->clips.at(0)->media->to_footage();
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
        FootagePtr m = seq->clips.at(0)->media->to_footage();
		m->using_inout = new_enabled;
		m->in = new_in;
		m->out = new_out;
    }
}

AddEffectCommand::AddEffectCommand(ClipPtr c, EffectPtr e, const EffectMeta *m, int insert_pos) {
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
		ref = create_effect(clip, meta);
	}
	if (pos < 0) {
		clip->effects.append(ref);
	} else {
		clip->effects.insert(pos, ref);
	}
	done = true;
}

AddTransitionCommand::AddTransitionCommand(ClipPtr c,
                                           ClipPtr s,
                                           TransitionPtr copy,
                                           const EffectMeta *itransition,
                                           int itype,
                                           int ilength) {
    clip = c;
    secondary = s;
    transition_to_copy = copy;
    transition = itransition;
    type = itype;
    length = ilength;
}

void AddTransitionCommand::doUndo() {
	clip->sequence->hard_delete_transition(clip, type);
	if (secondary != nullptr) secondary->sequence->hard_delete_transition(secondary, (type == TA_OPENING_TRANSITION) ? TA_CLOSING_TRANSITION : TA_OPENING_TRANSITION);

	if (type == TA_OPENING_TRANSITION) {
		clip->opening_transition = old_ptransition;
		if (secondary != nullptr) secondary->closing_transition = old_stransition;
	} else {
		clip->closing_transition = old_ptransition;
		if (secondary != nullptr) secondary->opening_transition = old_stransition;
    }
}

void AddTransitionCommand::doRedo() {
	if (type == TA_OPENING_TRANSITION) {
		old_ptransition = clip->opening_transition;
		clip->opening_transition = (transition_to_copy == nullptr) ? create_transition(clip, secondary, transition) : transition_to_copy->copy(clip, nullptr);
		if (secondary != nullptr) {
			old_stransition = secondary->closing_transition;
			secondary->closing_transition = clip->opening_transition;
		}
		if (length > 0) {
			clip->get_opening_transition()->set_length(length);
		}
	} else {
		old_ptransition = clip->closing_transition;
		clip->closing_transition = (transition_to_copy == nullptr) ? create_transition(clip, secondary, transition) : transition_to_copy->copy(clip, nullptr);
		if (secondary != nullptr) {
			old_stransition = secondary->opening_transition;
			secondary->opening_transition = clip->closing_transition;
		}
		if (length > 0) {
			clip->get_closing_transition()->set_length(length);
		}
    }
}

ModifyTransitionCommand::ModifyTransitionCommand(ClipPtr c, int itype, long ilength) {
    clip = c;
    type = itype;
    new_length = ilength;
}

void ModifyTransitionCommand::doUndo() {
    TransitionPtr t = (type == TA_OPENING_TRANSITION) ? clip->get_opening_transition() : clip->get_closing_transition();
    t->set_length(old_length);
}

void ModifyTransitionCommand::doRedo() {
    TransitionPtr t = (type == TA_OPENING_TRANSITION) ? clip->get_opening_transition() : clip->get_closing_transition();
	old_length = t->get_true_length();
    t->set_length(new_length);
}

DeleteTransitionCommand::DeleteTransitionCommand(SequencePtr s, int transition_index) {
    seq = s;
    index = transition_index;
    transition = nullptr;
    otc = nullptr;
    ctc = nullptr;
}

DeleteTransitionCommand::~DeleteTransitionCommand() {}

void DeleteTransitionCommand::doUndo() {
	seq->transitions[index] = transition;

	if (otc != nullptr) otc->opening_transition = index;
	if (ctc != nullptr) ctc->closing_transition = index;

    transition = nullptr;
}

void DeleteTransitionCommand::doRedo() {
	for (int i=0;i<seq->clips.size();i++) {
        ClipPtr c = seq->clips.at(i);
		if (c != nullptr) {
			if (c->opening_transition == index) {
				otc = c;
				c->opening_transition = -1;
			}
			if (c->closing_transition == index) {
				ctc = c;
				c->closing_transition = -1;
			}
		}
	}

	transition = seq->transitions.at(index);
    seq->transitions[index] = nullptr;
}

NewSequenceCommand::NewSequenceCommand(Media *s, Media* iparent) {
    seq = s;
    parent = iparent;
    done = false;

	if (parent == nullptr) parent = project_model.get_root();
}

NewSequenceCommand::~NewSequenceCommand() {
	if (!done) delete seq;
}

void NewSequenceCommand::doUndo() {
	project_model.removeChild(parent, seq);

    done = false;
}

void NewSequenceCommand::doRedo() {
	project_model.appendChild(parent, seq);

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
	project_model.removeChild(parent, item);
	done = false;
    
}

void AddMediaCommand::doRedo() {
	project_model.appendChild(parent, item);

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
	project_model.appendChild(parent, item);

    
	done = false;
}

void DeleteMediaCommand::doRedo() {
	project_model.removeChild(parent, item);

	done = true;
}

AddClipCommand::AddClipCommand(SequencePtr s, QVector<ClipPtr>& add) {
    seq = s;
    clips = add;
}

AddClipCommand::~AddClipCommand() {}

void AddClipCommand::doUndo() {
	panel_effect_controls->clear_effects(true);
	for (int i=0;i<clips.size();i++) {
        ClipPtr c = seq->clips.last();
		panel_timeline->deselect_area(c->timeline_in, c->timeline_out, c->track);
		undone_clips.prepend(c);
		if (c->open) close_clip(c, true);
		seq->clips.removeLast();
	}
    
}

void AddClipCommand::doRedo() {
	if (undone_clips.size() > 0) {
		for (int i=0;i<undone_clips.size();i++) {
			seq->clips.append(undone_clips.at(i));
		}
		undone_clips.clear();
	} else {
		int linkOffset = seq->clips.size();
		for (int i=0;i<clips.size();i++) {
            ClipPtr original = clips.at(i);
			if (original != nullptr) {
                ClipPtr copy = original->copy(seq);
				copy->linked.resize(original->linked.size());
				for (int j=0;j<original->linked.size();j++) {
					copy->linked[j] = original->linked.at(j) + linkOffset;
				}
				if (original->opening_transition > -1) copy->opening_transition = original->get_opening_transition()->copy(copy, nullptr);
				if (original->closing_transition > -1) copy->closing_transition = original->get_closing_transition()->copy(copy, nullptr);
				seq->clips.append(copy);
			} else {
				seq->clips.append(nullptr);
			}
		}
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
		dout << clips.at(i);
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
        SequencePtr s = all_sequences.at(i)->to_sequence();
		for (int j=0;j<s->clips.size();j++) {
            ClipPtr c = s->clips.at(j);
			if (c != nullptr && c->media == item && c->open) {
				close_clip(c, true);
				c->replaced = true;
			}
		}
	}

	// replace media
	QStringList files;
	files.append(filename);
	item->to_footage()->ready_lock.lock();
	panel_project->process_file_list(files, false, item, nullptr);
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
		if (c->open) {
			close_clip(c, true);
		}

		if (undo) {
			if (!preserve_clip_ins) {
				c->clip_in = old_clip_ins.at(i);
			}

			c->media = old_media;
		} else {
			if (!preserve_clip_ins) {
				old_clip_ins.append(c->clip_in);
				c->clip_in = 0;
			}

			c->media = new_media;
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

EffectDeleteCommand::EffectDeleteCommand() {
    done = false;
}

EffectDeleteCommand::~EffectDeleteCommand() {}

void EffectDeleteCommand::doUndo() {
	for (int i=0;i<clips.size();i++) {
        ClipPtr c = clips.at(i);
		c->effects.insert(fx.at(i), deleted_objects.at(i));
	}
	panel_effect_controls->reload_clips();
	done = false;
    
}

void EffectDeleteCommand::doRedo() {
	deleted_objects.clear();
	for (int i=0;i<clips.size();i++) {
        ClipPtr c = clips.at(i);
		int fx_id = fx.at(i) - i;
        EffectPtr e = c->effects.at(fx_id);
		e->close();
		deleted_objects.append(e);
		c->effects.removeAt(fx_id);
	}
	panel_effect_controls->reload_clips();
	done = true;
}

MediaMove::MediaMove() {}

void MediaMove::doUndo() {
	for (int i=0;i<items.size();i++) {
		project_model.moveChild(items.at(i), froms.at(i));
	}
    
}

void MediaMove::doRedo() {
	if (to == nullptr) to = project_model.get_root();
	froms.resize(items.size());
	for (int i=0;i<items.size();i++) {
        Media* parent = items.at(i)->parentItem();
		froms[i] = parent;
		project_model.moveChild(items.at(i), to);
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

EffectFieldUndo::EffectFieldUndo(EffectField* f) {
    field = f;
    done = true;

	old_val = field->get_previous_data();
	new_val = field->get_current_data();
}

void EffectFieldUndo::doUndo() {
	field->set_current_data(old_val);
	done = false;
    
}

void EffectFieldUndo::doRedo() {
	if (!done) {
		field->set_current_data(new_val);
	}
}

SetAutoscaleAction::SetAutoscaleAction() {}

void SetAutoscaleAction::doUndo() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->autoscale = !clips.at(i)->autoscale;
	}
	panel_sequence_viewer->viewer_widget->frame_update();
    
}

void SetAutoscaleAction::doRedo() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->autoscale = !clips.at(i)->autoscale;
	}
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

SetSpeedAction::SetSpeedAction(ClipPtr c, double speed) {
    clip = c;
    old_speed = c->speed;
    new_speed = speed;
}

void SetSpeedAction::doUndo() {
	clip->speed = old_speed;
	clip->recalculateMaxLength();
    
}

void SetSpeedAction::doRedo() {
	clip->speed = new_speed;
	clip->recalculateMaxLength();
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

SetSelectionsCommand::SetSelectionsCommand(SequencePtr s) {
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
	// update tooltip
	item->set_sequence(seq);

	for (int i=0;i<seq->clips.size();i++) {
		if (seq->clips.at(i) != nullptr) seq->clips.at(i)->refresh();
	}

	if (olive::ActiveSequence == seq) {
		set_sequence(seq);
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
	closeActiveClips(olive::ActiveSequence);
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

RenameClipCommand::RenameClipCommand() {}

void RenameClipCommand::doUndo() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->name = old_names.at(i);
	}
}

void RenameClipCommand::doRedo() {
	old_names.resize(clips.size());
	for (int i=0;i<clips.size();i++) {
		old_names[i] = clips.at(i)->name;
		clips.at(i)->name = new_name;
	}
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
	panel_effect_controls->reload_clips();
}

RippleAction::RippleAction(SequencePtr is, long ipoint, long ilength, const QVector<int> &iignore) {
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
				if (c->timeline_in >= point) {
					move_clip(ca, c, length, length, 0, 0, true, true);
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

SetKeyframing::SetKeyframing(EffectRow *irow, bool ib) {
    row = irow;
    b = ib;
}

void SetKeyframing::doUndo() {
	row->setKeyframing(!b);
}

void SetKeyframing::doRedo() {
	row->setKeyframing(b);
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
        SequencePtr s = all_sequences.at(i)->to_sequence();
		for (int j=0;j<s->clips.size();j++) {
            ClipPtr c = s->clips.at(j);
			if (c != nullptr && c->media == media) {
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
