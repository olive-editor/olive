#include "undo.h"

#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QCheckBox>

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

QUndoStack undo_stack;

ComboAction::ComboAction() {}

ComboAction::~ComboAction() {
	for (int i=0;i<commands.size();i++) {
		delete commands.at(i);
	}
}

void ComboAction::undo() {
	for (int i=commands.size()-1;i>=0;i--) {
		commands.at(i)->undo();
	}
	for (int i=0;i<post_commands.size();i++) {
		post_commands.at(i)->undo();
	}
}

void ComboAction::redo() {
	for (int i=0;i<commands.size();i++) {
		commands.at(i)->redo();
	}
	for (int i=0;i<post_commands.size();i++) {
		post_commands.at(i)->redo();
	}
}

void ComboAction::append(QUndoCommand* u) {
	commands.append(u);
}

void ComboAction::appendPost(QUndoCommand* u) {
	post_commands.append(u);
}

MoveClipAction::MoveClipAction(Clip *c, long iin, long iout, long iclip_in, int itrack, bool irelative) :
	clip(c),
	old_in(c->timeline_in),
	old_out(c->timeline_out),
	old_clip_in(c->clip_in),
	old_track(c->track),
	new_in(iin),
	new_out(iout),
	new_clip_in(iclip_in),
	new_track(itrack),
	relative(irelative),
	old_project_changed(mainWindow->isWindowModified())
{}

void MoveClipAction::undo() {
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

	mainWindow->setWindowModified(old_project_changed);
}

void MoveClipAction::redo() {
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

	mainWindow->setWindowModified(true);
}

DeleteClipAction::DeleteClipAction(Sequence* s, int clip) :
	seq(s),
	index(clip),
	opening_transition(-1),
	closing_transition(-1),
	old_project_changed(mainWindow->isWindowModified())
{}

DeleteClipAction::~DeleteClipAction() {
	if (ref != NULL) delete ref;
}

void DeleteClipAction::undo() {
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

	ref = NULL;

	mainWindow->setWindowModified(old_project_changed);
}

void DeleteClipAction::redo() {
	// remove ref to clip
	ref = seq->clips.at(index);
	if (ref->open) {
		close_clip(ref, true);
	}
	seq->clips[index] = NULL;

	// save shared transitions
	if (ref->opening_transition > -1 && ref->get_opening_transition()->secondary_clip != NULL) {
		opening_transition = ref->opening_transition;
		ref->get_opening_transition()->parent_clip = ref->get_opening_transition()->secondary_clip;
		ref->get_opening_transition()->secondary_clip = NULL;
		ref->opening_transition = -1;
	}
	if (ref->closing_transition > -1 && ref->get_closing_transition()->secondary_clip != NULL) {
		closing_transition = ref->closing_transition;
		ref->get_closing_transition()->secondary_clip = NULL;
		ref->closing_transition = -1;
	}

	// delete link to this clip
	linkClipIndex.clear();
	linkLinkIndex.clear();
	for (int i=0;i<seq->clips.size();i++) {
		Clip* c = seq->clips.at(i);
		if (c != NULL) {
			for (int j=0;j<c->linked.size();j++) {
				if (c->linked.at(j) == index) {
					linkClipIndex.append(i);
					linkLinkIndex.append(j);
					c->linked.removeAt(j);
				}
			}
		}
	}

	mainWindow->setWindowModified(true);
}

ChangeSequenceAction::ChangeSequenceAction(Sequence* s) :
	new_sequence(s)
{}

void ChangeSequenceAction::undo() {
	set_sequence(old_sequence);
}

void ChangeSequenceAction::redo() {
	old_sequence = sequence;
	set_sequence(new_sequence);
}

SetTimelineInOutCommand::SetTimelineInOutCommand(Sequence *s, bool enabled, long in, long out) :
	seq(s),
	new_enabled(enabled),
	new_in(in),
	new_out(out),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetTimelineInOutCommand::undo() {
	seq->using_workarea = old_enabled;
    seq->enable_workarea = old_workarea_enabled;
	seq->workarea_in = old_in;
	seq->workarea_out = old_out;

	// footage viewer functions
	if (seq->wrapper_sequence) {
		Footage* m = seq->clips.at(0)->media->to_footage();
		m->using_inout = old_enabled;
		m->in = old_in;
		m->out = old_out;
	}

	mainWindow->setWindowModified(old_project_changed);
}

void SetTimelineInOutCommand::redo() {
	old_enabled = seq->using_workarea;
    old_workarea_enabled = seq->enable_workarea;
	old_in = seq->workarea_in;
	old_out = seq->workarea_out;

    if (!seq->using_workarea) seq->enable_workarea = true;
	seq->using_workarea = new_enabled;
	seq->workarea_in = new_in;
	seq->workarea_out = new_out;

	// footage viewer functions
	if (seq->wrapper_sequence) {
		Footage* m = seq->clips.at(0)->media->to_footage();
		m->using_inout = new_enabled;
		m->in = new_in;
		m->out = new_out;
	}

	mainWindow->setWindowModified(true);
}

AddEffectCommand::AddEffectCommand(Clip* c, Effect* e, const EffectMeta *m, int insert_pos) :
	clip(c),
	meta(m),
	ref(e),
	pos(insert_pos),
	done(false),
	old_project_changed(mainWindow->isWindowModified())
{}

AddEffectCommand::~AddEffectCommand() {
	if (!done && ref != NULL) delete ref;
}

void AddEffectCommand::undo() {
	clip->effects.last()->close();
	if (pos < 0) {
		clip->effects.removeLast();
	} else {
		clip->effects.removeAt(pos);
	}
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void AddEffectCommand::redo() {
	if (ref == NULL) {
		ref = create_effect(clip, meta);
	}
	if (pos < 0) {
		clip->effects.append(ref);
	} else {
		clip->effects.insert(pos, ref);
	}
	done = true;
	mainWindow->setWindowModified(true);
}

AddTransitionCommand::AddTransitionCommand(Clip* c, Clip *s, Transition* copy, const EffectMeta *itransition, int itype, int ilength) :
	clip(c),
	secondary(s),
	transition_to_copy(copy),
	transition(itransition),
	type(itype),
	length(ilength),
	old_project_changed(mainWindow->isWindowModified())
{}

void AddTransitionCommand::undo() {
	clip->sequence->hard_delete_transition(clip, type);
	if (secondary != NULL) secondary->sequence->hard_delete_transition(secondary, (type == TA_OPENING_TRANSITION) ? TA_CLOSING_TRANSITION : TA_OPENING_TRANSITION);

	if (type == TA_OPENING_TRANSITION) {
		clip->opening_transition = old_ptransition;
		if (secondary != NULL) secondary->closing_transition = old_stransition;
	} else {
		clip->closing_transition = old_ptransition;
		if (secondary != NULL) secondary->opening_transition = old_stransition;
	}

	mainWindow->setWindowModified(old_project_changed);
}

void AddTransitionCommand::redo() {
	if (type == TA_OPENING_TRANSITION) {
		old_ptransition = clip->opening_transition;
		clip->opening_transition = (transition_to_copy == NULL) ? create_transition(clip, secondary, transition) : transition_to_copy->copy(clip, NULL);
		if (secondary != NULL) {
			old_stransition = secondary->closing_transition;
			secondary->closing_transition = clip->opening_transition;
		}
		if (length > 0) {
			clip->get_opening_transition()->set_length(length);
		}
	} else {
		old_ptransition = clip->closing_transition;
		clip->closing_transition = (transition_to_copy == NULL) ? create_transition(clip, secondary, transition) : transition_to_copy->copy(clip, NULL);
		if (secondary != NULL) {
			old_stransition = secondary->opening_transition;
			secondary->opening_transition = clip->closing_transition;
		}
		if (length > 0) {
			clip->get_closing_transition()->set_length(length);
		}
	}
	mainWindow->setWindowModified(true);
}

ModifyTransitionCommand::ModifyTransitionCommand(Clip* c, int itype, long ilength) :
	clip(c),
	type(itype),
	new_length(ilength),
	old_project_changed(mainWindow->isWindowModified())
{}

void ModifyTransitionCommand::undo() {
	Transition* t = (type == TA_OPENING_TRANSITION) ? clip->get_opening_transition() : clip->get_closing_transition();
	t->set_length(old_length);
	mainWindow->setWindowModified(old_project_changed);
}

void ModifyTransitionCommand::redo() {
	Transition* t = (type == TA_OPENING_TRANSITION) ? clip->get_opening_transition() : clip->get_closing_transition();
	old_length = t->get_true_length();
	t->set_length(new_length);
	mainWindow->setWindowModified(true);
}

DeleteTransitionCommand::DeleteTransitionCommand(Sequence* s, int transition_index) :
	seq(s),
	index(transition_index),
	transition(NULL),
	otc(NULL),
	ctc(NULL),
	old_project_changed(mainWindow->isWindowModified())
{}

DeleteTransitionCommand::~DeleteTransitionCommand() {
	if (transition != NULL) delete transition;
}

void DeleteTransitionCommand::undo() {
	seq->transitions[index] = transition;

	if (otc != NULL) otc->opening_transition = index;
	if (ctc != NULL) ctc->closing_transition = index;

	transition = NULL;
	mainWindow->setWindowModified(old_project_changed);
}

void DeleteTransitionCommand::redo() {
	for (int i=0;i<seq->clips.size();i++) {
		Clip* c = seq->clips.at(i);
		if (c != NULL) {
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
	seq->transitions[index] = NULL;

	mainWindow->setWindowModified(true);
}

NewSequenceCommand::NewSequenceCommand(Media *s, Media* iparent) :
	seq(s),
	parent(iparent),
	done(false),
	old_project_changed(mainWindow->isWindowModified())
{
	if (parent == NULL) parent = project_model.get_root();
}

NewSequenceCommand::~NewSequenceCommand() {
	if (!done) delete seq;
}

void NewSequenceCommand::undo() {
	project_model.removeChild(parent, seq);

	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void NewSequenceCommand::redo() {
	project_model.appendChild(parent, seq);

	done = true;
	mainWindow->setWindowModified(true);
}

AddMediaCommand::AddMediaCommand(Media* iitem, Media *iparent) :
	item(iitem),
	parent(iparent),
	done(false),
	old_project_changed(mainWindow->isWindowModified())
{}

AddMediaCommand::~AddMediaCommand() {
	if (!done) {
		delete item;
	}
}

void AddMediaCommand::undo() {
	project_model.removeChild(parent, item);
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void AddMediaCommand::redo() {
	project_model.appendChild(parent, item);

	done = true;
	mainWindow->setWindowModified(true);
}

DeleteMediaCommand::DeleteMediaCommand(Media* i) :
	item(i),
	parent(i->parentItem()),
	old_project_changed(mainWindow->isWindowModified())
{}

DeleteMediaCommand::~DeleteMediaCommand() {
	if (done) {
		delete item;
	}
}

void DeleteMediaCommand::undo() {
	project_model.appendChild(parent, item);

	mainWindow->setWindowModified(old_project_changed);
	done = false;
}

void DeleteMediaCommand::redo() {
	project_model.removeChild(parent, item);

	mainWindow->setWindowModified(true);
	done = true;
}

AddClipCommand::AddClipCommand(Sequence* s, QVector<Clip*>& add) :
	seq(s),
	clips(add),
	old_project_changed(mainWindow->isWindowModified())
{}

AddClipCommand::~AddClipCommand() {
	for (int i=0;i<clips.size();i++) {
		delete clips.at(i);
	}
	for (int i=0;i<undone_clips.size();i++) {
		delete undone_clips.at(i);
	}
}

void AddClipCommand::undo() {
	panel_effect_controls->clear_effects(true);
	for (int i=0;i<clips.size();i++) {
		Clip* c = seq->clips.last();
		panel_timeline->deselect_area(c->timeline_in, c->timeline_out, c->track);
		undone_clips.prepend(c);
		if (c->open) close_clip(c, true);
		seq->clips.removeLast();
	}
	mainWindow->setWindowModified(old_project_changed);
}

void AddClipCommand::redo() {
	if (undone_clips.size() > 0) {
		for (int i=0;i<undone_clips.size();i++) {
			seq->clips.append(undone_clips.at(i));
		}
		undone_clips.clear();
	} else {
		int linkOffset = seq->clips.size();
		for (int i=0;i<clips.size();i++) {
			Clip* original = clips.at(i);
			Clip* copy = original->copy(seq);
			copy->linked.resize(original->linked.size());
			for (int j=0;j<original->linked.size();j++) {
				copy->linked[j] = original->linked.at(j) + linkOffset;
			}
			if (original->opening_transition > -1) copy->opening_transition = original->get_opening_transition()->copy(copy, NULL);
			if (original->closing_transition > -1) copy->closing_transition = original->get_closing_transition()->copy(copy, NULL);
			seq->clips.append(copy);
		}
	}
	mainWindow->setWindowModified(true);
}

LinkCommand::LinkCommand() : link(true), old_project_changed(mainWindow->isWindowModified()) {}

void LinkCommand::undo() {
	for (int i=0;i<clips.size();i++) {
		Clip* c = s->clips.at(clips.at(i));
		if (link) {
			c->linked.clear();
		} else {
			c->linked = old_links.at(i);
		}
	}
	mainWindow->setWindowModified(old_project_changed);
}

void LinkCommand::redo() {
	old_links.clear();
	for (int i=0;i<clips.size();i++) {
		dout << clips.at(i);
		Clip* c = s->clips.at(clips.at(i));
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
	mainWindow->setWindowModified(true);
}

CheckboxCommand::CheckboxCommand(QCheckBox* b) : box(b), checked(box->isChecked()), done(true), old_project_changed(mainWindow->isWindowModified()) {}

CheckboxCommand::~CheckboxCommand() {}

void CheckboxCommand::undo() {
	box->setChecked(!checked);
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void CheckboxCommand::redo() {
	if (!done) {
		box->setChecked(checked);
	}
	mainWindow->setWindowModified(true);
}

ReplaceMediaCommand::ReplaceMediaCommand(Media* i, QString s) :
	item(i),
	new_filename(s),
	old_project_changed(mainWindow->isWindowModified())
{
	old_filename = item->to_footage()->url;
}

void ReplaceMediaCommand::replace(QString& filename) {
	// close any clips currently using this media
	QVector<Media*> all_sequences = panel_project->list_all_project_sequences();
	for (int i=0;i<all_sequences.size();i++) {
		Sequence* s = all_sequences.at(i)->to_sequence();
		for (int j=0;j<s->clips.size();j++) {
			Clip* c = s->clips.at(j);
			if (c != NULL && c->media == item && c->open) {
				close_clip(c, true);
				c->replaced = true;
			}
		}
	}

	// replace media
	QStringList files;
	files.append(filename);
	item->to_footage()->ready_lock.lock();
	panel_project->process_file_list(files, false, item, NULL);
}

void ReplaceMediaCommand::undo() {
	replace(old_filename);

	mainWindow->setWindowModified(old_project_changed);
}

void ReplaceMediaCommand::redo() {
	replace(new_filename);

	mainWindow->setWindowModified(true);
}

ReplaceClipMediaCommand::ReplaceClipMediaCommand(Media *a, Media *b, bool e) :
	old_media(a),
	new_media(b),
	preserve_clip_ins(e),
	old_project_changed(mainWindow->isWindowModified())
{}

void ReplaceClipMediaCommand::replace(bool undo) {
	if (!undo) {
		old_clip_ins.clear();
	}

	for (int i=0;i<clips.size();i++) {
		Clip* c = clips.at(i);
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

void ReplaceClipMediaCommand::undo() {
	replace(true);

	mainWindow->setWindowModified(old_project_changed);
}

void ReplaceClipMediaCommand::redo() {
	replace(false);

	update_ui(true);
	mainWindow->setWindowModified(true);
}

EffectDeleteCommand::EffectDeleteCommand() : done(false), old_project_changed(mainWindow->isWindowModified()) {}

EffectDeleteCommand::~EffectDeleteCommand() {
	if (done) {
		for (int i=0;i<deleted_objects.size();i++) {
			delete deleted_objects.at(i);
		}
	}
}

void EffectDeleteCommand::undo() {
	for (int i=0;i<clips.size();i++) {
		Clip* c = clips.at(i);
		c->effects.insert(fx.at(i), deleted_objects.at(i));
	}
	panel_effect_controls->reload_clips();
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void EffectDeleteCommand::redo() {
	deleted_objects.clear();
	for (int i=0;i<clips.size();i++) {
		Clip* c = clips.at(i);
		int fx_id = fx.at(i) - i;
		Effect* e = c->effects.at(fx_id);
		e->close();
		deleted_objects.append(e);
		c->effects.removeAt(fx_id);
	}
	panel_effect_controls->reload_clips();
	done = true;
	mainWindow->setWindowModified(true);
}

MediaMove::MediaMove() : old_project_changed(mainWindow->isWindowModified()) {}

void MediaMove::undo() {
	for (int i=0;i<items.size();i++) {
		project_model.moveChild(items.at(i), froms.at(i));
	}
	mainWindow->setWindowModified(old_project_changed);
}

void MediaMove::redo() {
	if (to == NULL) to = project_model.get_root();
	froms.resize(items.size());
	for (int i=0;i<items.size();i++) {
		Media* parent = items.at(i)->parentItem();
		froms[i] = parent;
		project_model.moveChild(items.at(i), to);
	}
	mainWindow->setWindowModified(true);
}

MediaRename::MediaRename(Media* iitem, QString ito) :
	item(iitem),
	from(iitem->get_name()),
	to(ito),
	old_project_changed(mainWindow->isWindowModified())
{}

void MediaRename::undo() {
	item->set_name(from);
	mainWindow->setWindowModified(old_project_changed);
}

void MediaRename::redo() {
	item->set_name(to);
	mainWindow->setWindowModified(true);
}

KeyframeDelete::KeyframeDelete(EffectField *ifield, int iindex) :
	field(ifield),
	index(iindex),
	old_project_changed(mainWindow->isWindowModified())
{}

void KeyframeDelete::undo() {
	field->keyframes.insert(index, deleted_key);
	mainWindow->setWindowModified(old_project_changed);
}

void KeyframeDelete::redo() {
	deleted_key = field->keyframes.at(index);
	field->keyframes.removeAt(index);
	mainWindow->setWindowModified(true);
}

EffectFieldUndo::EffectFieldUndo(EffectField* f) :
	field(f),
	done(true),
	old_project_changed(mainWindow->isWindowModified())
{
	old_val = field->get_previous_data();
	new_val = field->get_current_data();
}

void EffectFieldUndo::undo() {
	field->set_current_data(old_val);
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void EffectFieldUndo::redo() {
	if (!done) {
		field->set_current_data(new_val);
	}
	mainWindow->setWindowModified(true);
}

SetAutoscaleAction::SetAutoscaleAction() :
	old_project_changed(mainWindow->isWindowModified())
{}

void SetAutoscaleAction::undo() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->autoscale = !clips.at(i)->autoscale;
	}
	panel_sequence_viewer->viewer_widget->update();
	mainWindow->setWindowModified(old_project_changed);
}

void SetAutoscaleAction::redo() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->autoscale = !clips.at(i)->autoscale;
	}
	panel_sequence_viewer->viewer_widget->update();
	mainWindow->setWindowModified(true);
}

AddMarkerAction::AddMarkerAction(Sequence* s, long t, QString n) :
	seq(s),
	time(t),
	name(n),
	old_project_changed(mainWindow->isWindowModified())
{}

void AddMarkerAction::undo() {
	if (index == -1) {
		seq->markers.removeLast();
	} else {
		seq->markers[index].name = old_name;
	}

	mainWindow->setWindowModified(old_project_changed);
}

void AddMarkerAction::redo() {
	index = -1;
	for (int i=0;i<seq->markers.size();i++) {
		if (seq->markers.at(i).frame == time) {
			index = i;
			break;
		}
	}

	if (index == -1) {
		Marker m;
		m.frame = time;
		seq->markers.append(m);
	} else {
		old_name = seq->markers.at(index).name;
		seq->markers[index].name = name;
	}

	mainWindow->setWindowModified(true);
}

MoveMarkerAction::MoveMarkerAction(Marker* m, long o, long n) :
	marker(m),
	old_time(o),
	new_time(n),
	old_project_changed(mainWindow->isWindowModified())
{}

void MoveMarkerAction::undo() {
	marker->frame = old_time;
	mainWindow->setWindowModified(old_project_changed);
}

void MoveMarkerAction::redo() {
	marker->frame = new_time;
	mainWindow->setWindowModified(true);
}

DeleteMarkerAction::DeleteMarkerAction(Sequence* s) :
	seq(s),
	sorted(false),
	old_project_changed(mainWindow->isWindowModified())
{}

void DeleteMarkerAction::undo() {
	for (int i=markers.size()-1;i>=0;i--) {
		seq->markers.insert(markers.at(i), copies.at(i));
	}
	mainWindow->setWindowModified(old_project_changed);
}

void DeleteMarkerAction::redo() {
	for (int i=0;i<markers.size();i++) {
		// correct future removals
		if (!sorted) {
			copies.append(seq->markers.at(markers.at(i)));
			for (int j=i+1;j<markers.size();j++) {
				if (markers.at(j) > markers.at(i)) {
					markers[j]--;
				}
			}
		}
		seq->markers.removeAt(markers.at(i));
	}
	sorted = true;
	mainWindow->setWindowModified(true);
}

SetSpeedAction::SetSpeedAction(Clip* c, double speed) :
	clip(c),
	old_speed(c->speed),
	new_speed(speed),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetSpeedAction::undo() {
	clip->speed = old_speed;
	clip->recalculateMaxLength();
	mainWindow->setWindowModified(old_project_changed);
}

void SetSpeedAction::redo() {
	clip->speed = new_speed;
	clip->recalculateMaxLength();
	mainWindow->setWindowModified(true);
}

SetBool::SetBool(bool* b, bool setting) :
	boolean(b),
	old_setting(*b),
	new_setting(setting),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetBool::undo() {
	*boolean = old_setting;
	mainWindow->setWindowModified(old_project_changed);
}

void SetBool::redo() {
	*boolean = new_setting;
	mainWindow->setWindowModified(true);
}

SetSelectionsCommand::SetSelectionsCommand(Sequence* s) :
	seq(s),
	done(true),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetSelectionsCommand::undo() {
	seq->selections = old_data;
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void SetSelectionsCommand::redo() {
	if (!done) {
		seq->selections = new_data;
		done = true;
	}
	mainWindow->setWindowModified(true);
}

SetEnableCommand::SetEnableCommand(Clip* c, bool enable) :
	clip(c),
	old_val(c->enabled),
	new_val(enable),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetEnableCommand::undo() {
	clip->enabled = old_val;
	mainWindow->setWindowModified(old_project_changed);
}

void SetEnableCommand::redo() {
	clip->enabled = new_val;
	mainWindow->setWindowModified(true);
}

EditSequenceCommand::EditSequenceCommand(Media* i, Sequence *s) :
	item(i),
	seq(s),
	old_project_changed(mainWindow->isWindowModified()),
	old_name(s->name),
	old_width(s->width),
	old_height(s->height),
	old_frame_rate(s->frame_rate),
	old_audio_frequency(s->audio_frequency),
	old_audio_layout(s->audio_layout)
{}

void EditSequenceCommand::undo() {
	seq->name = old_name;
	seq->width = old_width;
	seq->height = old_height;
	seq->frame_rate = old_frame_rate;
	seq->audio_frequency = old_audio_frequency;
	seq->audio_layout = old_audio_layout;
	update();

	mainWindow->setWindowModified(old_project_changed);
}

void EditSequenceCommand::redo() {
	seq->name = name;
	seq->width = width;
	seq->height = height;
	seq->frame_rate = frame_rate;
	seq->audio_frequency = audio_frequency;
	seq->audio_layout = audio_layout;
	update();

	mainWindow->setWindowModified(true);
}

void EditSequenceCommand::update() {
	// update tooltip
	item->set_sequence(seq);

	for (int i=0;i<seq->clips.size();i++) {
		if (seq->clips.at(i) != NULL) seq->clips.at(i)->refresh();
	}

	if (sequence == seq) {
		set_sequence(seq);
	}
}

SetInt::SetInt(int* pointer, int new_value) :
	p(pointer),
	oldval(*pointer),
	newval(new_value),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetInt::undo() {
	*p = oldval;
	mainWindow->setWindowModified(old_project_changed);
}

void SetInt::redo() {
	*p = newval;
	mainWindow->setWindowModified(true);
}

SetString::SetString(QString* pointer, QString new_value) :
	p(pointer),
	oldval(*pointer),
	newval(new_value),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetString::undo() {
	*p = oldval;
	mainWindow->setWindowModified(old_project_changed);
}

void SetString::redo() {
	*p = newval;
	mainWindow->setWindowModified(true);
}

void CloseAllClipsCommand::undo() {
	redo();
}

void CloseAllClipsCommand::redo() {
	closeActiveClips(sequence);
}

UpdateFootageTooltip::UpdateFootageTooltip(Media *i) :
	item(i)
{}

void UpdateFootageTooltip::undo() {
	redo();
}

void UpdateFootageTooltip::redo() {
	item->update_tooltip();
}

MoveEffectCommand::MoveEffectCommand() :
	old_project_changed(mainWindow->isWindowModified())
{}

void MoveEffectCommand::undo() {
	clip->effects.move(to, from);
	mainWindow->setWindowModified(old_project_changed);
}

void MoveEffectCommand::redo() {
	clip->effects.move(from, to);
	mainWindow->setWindowModified(true);
}

RemoveClipsFromClipboard::RemoveClipsFromClipboard(int index) :
	pos(index),
	old_project_changed(mainWindow->isWindowModified()),
	done(false)
{}

RemoveClipsFromClipboard::~RemoveClipsFromClipboard() {
	if (done) {
		delete clip;
	}
}

void RemoveClipsFromClipboard::undo() {
	clipboard.insert(pos, clip);
	done = false;
}

void RemoveClipsFromClipboard::redo() {
	clip = static_cast<Clip*>(clipboard.at(pos));
	clipboard.removeAt(pos);
	done = true;
}

RenameClipCommand::RenameClipCommand() :
	old_project_changed(mainWindow->isWindowModified())
{}

void RenameClipCommand::undo() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->name = old_names.at(i);
	}
}

void RenameClipCommand::redo() {
	old_names.resize(clips.size());
	for (int i=0;i<clips.size();i++) {
		old_names[i] = clips.at(i)->name;
		clips.at(i)->name = new_name;
	}
}

SetPointer::SetPointer(void **pointer, void *data) :
	p(pointer),
	new_data(data),
	old_changed(mainWindow->isWindowModified())
{}

void SetPointer::undo() {
	*p = old_data;
	mainWindow->setWindowModified(old_changed);
}

void SetPointer::redo() {
	old_data = *p;
	*p = new_data;
	mainWindow->setWindowModified(true);
}

void ReloadEffectsCommand::undo() {
	redo();
}

void ReloadEffectsCommand::redo() {
	panel_effect_controls->reload_clips();
}

RippleAction::RippleAction(Sequence *is, long ipoint, long ilength, const QVector<int> &iignore) :
	s(is),
	point(ipoint),
	length(ilength),
	ignore(iignore)
{}

void RippleAction::undo() {
	ca->undo();
	delete ca;
}

void RippleAction::redo() {
	ca = new ComboAction();
	for (int i=0;i<s->clips.size();i++) {
		if (!ignore.contains(i)) {
			Clip* c = s->clips.at(i);
			if (c != NULL) {
				if (c->timeline_in >= point) {
					move_clip(ca, c, length, length, 0, 0, true, true);
				}
			}
		}
	}
	ca->redo();
}

SetDouble::SetDouble(double* pointer, double old_value, double new_value) :
	p(pointer),
	oldval(old_value),
	newval(new_value),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetDouble::undo() {
	*p = oldval;
	mainWindow->setWindowModified(old_project_changed);
}

void SetDouble::redo() {
	*p = newval;
	mainWindow->setWindowModified(true);
}

SetQVariant::SetQVariant(QVariant *itarget, const QVariant &iold, const QVariant &inew) :
	target(itarget),
	old_val(iold),
	new_val(inew)
{}

void SetQVariant::undo() {
	*target = old_val;
}

void SetQVariant::redo() {
	*target = new_val;
}

SetLong::SetLong(long *pointer, long old_value, long new_value) :
	p(pointer),
	oldval(old_value),
	newval(new_value),
	old_project_changed(mainWindow->isWindowModified())
{}

void SetLong::undo() {
	*p = oldval;
	mainWindow->setWindowModified(old_project_changed);
}

void SetLong::redo() {
	*p = newval;
	mainWindow->setWindowModified(true);
}

KeyframeFieldSet::KeyframeFieldSet(EffectField *ifield, int ii) :
	field(ifield),
	index(ii),
	key(ifield->keyframes.at(ii)),
	done(true),
	old_project_changed(mainWindow->isWindowModified())
{}

void KeyframeFieldSet::undo() {
	field->keyframes.removeAt(index);
	mainWindow->setWindowModified(old_project_changed);
	done = false;
}

void KeyframeFieldSet::redo() {
	if (!done) {
		field->keyframes.insert(index, key);
		mainWindow->setWindowModified(true);
	}
	done = true;
}

SetKeyframing::SetKeyframing(EffectRow *irow, bool ib) :
	row(irow),
	b(ib)
{}

void SetKeyframing::undo() {
	row->setKeyframing(!b);
}

void SetKeyframing::redo() {
	row->setKeyframing(b);
}

RefreshClips::RefreshClips(Media *m) :
	media(m)
{}

void RefreshClips::undo() {
	redo();
}

void RefreshClips::redo() {
	// close any clips currently using this media
	QVector<Media*> all_sequences = panel_project->list_all_project_sequences();
	for (int i=0;i<all_sequences.size();i++) {
		Sequence* s = all_sequences.at(i)->to_sequence();
		for (int j=0;j<s->clips.size();j++) {
			Clip* c = s->clips.at(j);
			if (c != NULL && c->media == media) {
				c->replaced = true;
				c->refresh();
			}
		}
	}
}
