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
#include "io/media.h"
#include "playback/cacher.h"
#include "ui/labelslider.h"
#include "ui/viewerwidget.h"
#include "project/marker.h"
#include "mainwindow.h"
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

MoveClipAction::MoveClipAction(Clip *c, long iin, long iout, long iclip_in, int itrack) :
    clip(c),
	old_in(c->timeline_in),
	old_out(c->timeline_out),
	old_clip_in(c->clip_in),
	old_track(c->track),
    new_in(iin),
    new_out(iout),
    new_clip_in(iclip_in),
	new_track(itrack),
	old_project_changed(mainWindow->isWindowModified())
{}

void MoveClipAction::undo() {
    clip->timeline_in = old_in;
    clip->timeline_out = old_out;
    clip->clip_in = old_clip_in;
    clip->track = old_track;

	mainWindow->setWindowModified(old_project_changed);
}

void MoveClipAction::redo() {
    clip->timeline_in = new_in;
    clip->timeline_out = new_out;
    clip->clip_in = new_clip_in;
    clip->track = new_track;

	mainWindow->setWindowModified(true);
}

DeleteClipAction::DeleteClipAction(Sequence* s, int clip) :
    seq(s),
	index(clip),
	old_project_changed(mainWindow->isWindowModified())
{}

DeleteClipAction::~DeleteClipAction() {
    if (ref != NULL) delete ref;
}

void DeleteClipAction::undo() {
	// restore ref to clip
    seq->clips[index] = ref;
    ref = NULL;

	// restore links to this clip
	for (int i=linkClipIndex.size()-1;i>=0;i--) {
		seq->clips.at(linkClipIndex.at(i))->linked.insert(linkLinkIndex.at(i), index);
	}

	mainWindow->setWindowModified(old_project_changed);
}

void DeleteClipAction::redo() {
	// remove ref to clip
    ref = seq->clips.at(index);
    seq->clips[index] = NULL;

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
    seq->workarea_in = old_in;
    seq->workarea_out = old_out;

    // footage viewer functions
    if (seq->wrapper_sequence) {
        Media* m = static_cast<Media*>(seq->clips.at(0)->media);
        m->using_inout = old_enabled;
        m->in = old_in;
        m->out = old_out;
    }

	mainWindow->setWindowModified(old_project_changed);
}

void SetTimelineInOutCommand::redo() {
    old_enabled = seq->using_workarea;
    old_in = seq->workarea_in;
    old_out = seq->workarea_out;

    seq->using_workarea = new_enabled;
    seq->workarea_in = new_in;
    seq->workarea_out = new_out;

    // footage viewer functions
    if (seq->wrapper_sequence) {
        Media* m = static_cast<Media*>(seq->clips.at(0)->media);
        m->using_inout = new_enabled;
        m->in = new_in;
        m->out = new_out;
    }

	mainWindow->setWindowModified(true);
}

AddEffectCommand::AddEffectCommand(Clip* c, const EffectMeta *e) :
    clip(c),
    meta(e),
    ref(NULL),
	done(false),
	old_project_changed(mainWindow->isWindowModified())
{}

AddEffectCommand::~AddEffectCommand() {
    if (!done && ref != NULL) delete ref;
}

void AddEffectCommand::undo() {
    clip->effects.removeLast();
    done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void AddEffectCommand::redo() {
    if (ref == NULL) {
		ref = create_effect(clip, meta);
    }
    clip->effects.append(ref);
    done = true;
	mainWindow->setWindowModified(true);
}

AddTransitionCommand::AddTransitionCommand(Clip* c, const EffectMeta *itransition, int itype, int ilength) :
    clip(c),
    transition(itransition),
	type(itype),
    length(ilength),
	old_project_changed(mainWindow->isWindowModified())
{}

void AddTransitionCommand::undo() {
    if (type == TA_OPENING_TRANSITION) {
        delete clip->opening_transition;
        clip->opening_transition = NULL;
    } else {
        delete clip->closing_transition;
        clip->closing_transition = NULL;
    }
	mainWindow->setWindowModified(old_project_changed);
}

void AddTransitionCommand::redo() {
    if (type == TA_OPENING_TRANSITION) {
		clip->opening_transition = create_effect(clip, transition);
        if (length > 0) clip->opening_transition->length = length;
    } else {
		clip->closing_transition = create_effect(clip, transition);
        if (length > 0) clip->closing_transition->length = length;
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
    if (type == TA_OPENING_TRANSITION) {
        clip->opening_transition->length = old_length;
    } else {
        clip->closing_transition->length = old_length;
    }
	mainWindow->setWindowModified(old_project_changed);
}

void ModifyTransitionCommand::redo() {
    if (type == TA_OPENING_TRANSITION) {
        old_length = clip->opening_transition->length;
        clip->opening_transition->length = new_length;
    } else {
        old_length = clip->closing_transition->length;
        clip->closing_transition->length = new_length;
    }
	mainWindow->setWindowModified(true);
}

DeleteTransitionCommand::DeleteTransitionCommand(Clip* c, int itype) :
    clip(c),
    type(itype),
	transition(NULL),
	old_project_changed(mainWindow->isWindowModified())
{}

DeleteTransitionCommand::~DeleteTransitionCommand() {
    if (transition != NULL) delete transition;
}

void DeleteTransitionCommand::undo() {
    if (type == TA_OPENING_TRANSITION) {
        clip->opening_transition = transition;
    } else {
        clip->closing_transition = transition;
    }
    transition = NULL;
	mainWindow->setWindowModified(old_project_changed);
}

void DeleteTransitionCommand::redo() {
    if (type == TA_OPENING_TRANSITION) {
        transition = clip->opening_transition;
        clip->opening_transition = NULL;
    } else {
        transition = clip->closing_transition;
        clip->closing_transition = NULL;
    }
	mainWindow->setWindowModified(true);
}

NewSequenceCommand::NewSequenceCommand(QTreeWidgetItem *s, QTreeWidgetItem* iparent) :
    seq(s),
    parent(iparent),
	done(false),
	old_project_changed(mainWindow->isWindowModified())
{}

NewSequenceCommand::~NewSequenceCommand() {
    if (!done) delete seq;
}

void NewSequenceCommand::undo() {
    if (parent == NULL) {
        panel_project->source_table->takeTopLevelItem(panel_project->source_table->indexOfTopLevelItem(seq));
    } else {
        parent->removeChild(seq);
    }
    done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void NewSequenceCommand::redo() {
    if (parent == NULL) {
        panel_project->source_table->addTopLevelItem(seq);
    } else {
        parent->addChild(seq);
    }
    done = true;
	mainWindow->setWindowModified(true);
}

AddMediaCommand::AddMediaCommand(QTreeWidgetItem* iitem, QTreeWidgetItem* iparent) :
    item(iitem),
    parent(iparent),
	done(false),
	old_project_changed(mainWindow->isWindowModified())
{}

AddMediaCommand::~AddMediaCommand() {
    if (!done) {
        panel_project->delete_media(item);
		if (item->data(0, Qt::UserRole + 5) != 0) delete reinterpret_cast<MediaThrobber*>(item->data(0, Qt::UserRole + 5).value<quintptr>());
        delete item;
    }
}

void AddMediaCommand::undo() {
	if (parent == NULL) {
		bool found = false;
		for (int i=0;i<panel_project->source_table->topLevelItemCount();i++) {
			if (panel_project->source_table->topLevelItem(i) == item) {
				panel_project->source_table->takeTopLevelItem(i);
				found = true;
				break;
			}
		}
    } else {
        parent->removeChild(item);
    }
    done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void AddMediaCommand::redo() {
    if (parent == NULL) {
        panel_project->source_table->addTopLevelItem(item);
    } else {
        parent->addChild(item);
	}

	/* Here we force the source_table to sort itself.
	 *
	 * For some reason, sometimes when you add items to the QTreeWidget,
	 * (usually upon first import) they appear at the bottom, regardless
	 * of where they should be placed alphabetically. Then when this
	 * function is "undone", and it tries to remove this item from the
	 * QTreeWidget, it immediately sorts and then removes THE WRONG ONE.
	 * If this happens to be a sequence, the sequence data doesn't save
	 * and is then lost forever (outside of autorecoveries).
	 *
	 * The following 2 lines seem to force the source_table to re-sort
	 * correctly and therefore works around this problem. But holy shit.
	 */
	panel_project->source_table->setSortingEnabled(false);
	panel_project->source_table->setSortingEnabled(true);

	done = true;
	mainWindow->setWindowModified(true);
}

DeleteMediaCommand::DeleteMediaCommand(QTreeWidgetItem* i) :
	item(i),
	old_project_changed(mainWindow->isWindowModified())
{}

DeleteMediaCommand::~DeleteMediaCommand() {
	if (done) {
		panel_project->delete_media(item);
		if (item->data(0, Qt::UserRole + 5) != 0) delete reinterpret_cast<MediaThrobber*>(item->data(0, Qt::UserRole + 5).value<quintptr>());
		delete item;
	}
}

void DeleteMediaCommand::undo() {
    if (parent == NULL) {
        panel_project->source_table->addTopLevelItem(item);
    } else {
        parent->addChild(item);
    }

	mainWindow->setWindowModified(old_project_changed);
	done = false;
}

void DeleteMediaCommand::redo() {
    parent = item->parent();

    if (parent == NULL) {
        panel_project->source_table->takeTopLevelItem(panel_project->source_table->indexOfTopLevelItem(item));
    } else {
        parent->removeChild(item);
    }	

	mainWindow->setWindowModified(true);
	done = true;
}

RippleCommand::RippleCommand(Sequence *s, long ipoint, long ilength) :
    seq(s),
    point(ipoint),
	length(ilength),
	old_project_changed(mainWindow->isWindowModified())
{}

void RippleCommand::undo() {
    for (int i=0;i<rippled.size();i++) {
        Clip* c = rippled.at(i);
        c->timeline_in -= length;
        c->timeline_out -= length;
    }
	mainWindow->setWindowModified(old_project_changed);
}

void RippleCommand::redo() {
    rippled.clear();
    for (int j=0;j<seq->clips.size();j++) {
        Clip* c = seq->clips.at(j);
        bool found = false;
        for (int i=0;i<ignore.size();i++) {
            if (ignore.at(i) == c) {
                found = true;
                break;
            }
        }

        if (!found) {
            if (c != NULL && c->timeline_in >= point) {
                c->timeline_in += length;
                c->timeline_out += length;
                rippled.append(c);
            }
        }
    }
	mainWindow->setWindowModified(true);
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
    for (int i=0;i<clips.size();i++) {
		Clip* c = seq->clips.last();
		panel_timeline->deselect_area(c->timeline_in, c->timeline_out, c->track);
		undone_clips.prepend(c);
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
			seq->clips.append(copy);
		}
	}
	mainWindow->setWindowModified(true);
}

/*
#define TA_ADD_EFFECT 9
#define TA_ADD_TRANSITION 10
#define TA_MODIFY_TRANSITION 11
#define TA_DELETE_TRANSITION 12

TimelineAction::TimelineAction() :
    done(false),
    change_seq(false),
    ripple_enabled(false),
	change_in_out(false),
	old_project_changed(mainWindow->isWindowModified())
{}

TimelineAction::~TimelineAction() {
    for (int i=0;i<clips_to_add.size();i++) {
        delete clips_to_add.at(i);
    }
    if (done) {
        for (int i=0;i<deleted_clips.size();i++) {
            delete deleted_clips.at(i);
        }
        for (int i=0;i<deleted_media.size();i++) {
            panel_project->delete_media(deleted_media.at(i));
            delete deleted_media.at(i);
        }
    } else {
        for (int i=0;i<new_sequence_items.size();i++) {
            delete get_sequence_from_tree(new_sequence_items.at(i));
            delete new_sequence_items.at(i);
        }
        for (int i=0;i<media_to_add.size();i++) {
            panel_project->delete_media(media_to_add.at(i));
            delete media_to_add.at(i);
        }
    }
}

void TimelineAction::offset_links(QVector<Clip*>& clips, int offset) {
    for (int i=0;i<clips.size();i++) {
        Clip* c = clips.at(i);
        for (int j=0;j<c->linked.size();j++) {
            c->linked[j] += offset;
        }
    }
}

void TimelineAction::change_sequence(Sequence* s) {
    new_seq = s;
    change_seq = true;
}

void TimelineAction::new_sequence(QTreeWidgetItem *s, QTreeWidgetItem* parent) {
    new_sequence_items.append(s);
    new_sequence_parents.append(parent);
}

void TimelineAction::add_clips(Sequence* s, QVector<Clip*>& add) {
    offset_links(add, clips_to_add.size());
    clips_to_add.append(add);
    for (int i=0;i<clips_to_add.size();i++) {
        sequence_to_add_clips_to.append(s);
    }
}

void TimelineAction::new_action(Sequence* s, int action, int clip, int transition_type, long old_val, long new_val) {
    sequences.append(s);
    actions.append(action);
    clips.append(clip);
	transition_types.append(transition_type);
    old_values.append(old_val);
    new_values.append(new_val);
}

void TimelineAction::set_timeline_in(Sequence* s, int clip, long value) {
	new_action(s, TA_IN, clip, TA_NO_TRANSITION, s->get_clip(clip)->timeline_in, value);
}

void TimelineAction::set_timeline_out(Sequence* s, int clip, long value) {
	new_action(s, TA_OUT, clip, TA_NO_TRANSITION, s->get_clip(clip)->timeline_out, value);
}

void TimelineAction::set_clip_in(Sequence* s, int clip, long value) {
	new_action(s, TA_CLIP_IN, clip, TA_NO_TRANSITION, s->get_clip(clip)->clip_in, value);
}

void TimelineAction::set_track(Sequence* s, int clip, int value) {
	new_action(s, TA_TRACK, clip, TA_NO_TRANSITION, s->get_clip(clip)->track, value);
}

void TimelineAction::increase_timeline_in(Sequence* s, int clip, long value) {
	new_action(s, TA_ADD_IN, clip, TA_NO_TRANSITION, 0, value);
}

void TimelineAction::increase_timeline_out(Sequence* s, int clip, long value) {
	new_action(s, TA_ADD_OUT, clip, TA_NO_TRANSITION, 0, value);
}

void TimelineAction::increase_clip_in(Sequence* s, int clip, long value) {
	new_action(s, TA_ADD_CLIP_IN, clip, TA_NO_TRANSITION, 0, value);
}

void TimelineAction::increase_track(Sequence* s, int clip, int value) {
	new_action(s, TA_ADD_TRACK, clip, TA_NO_TRANSITION, 0, value);
}

void TimelineAction::delete_clip(Sequence* s, int clip) {
	new_action(s, TA_DELETE, clip, TA_NO_TRANSITION, 0, 0);
}

void TimelineAction::add_media(QTreeWidgetItem* item, QTreeWidgetItem *parent) {
    media_to_add.append(item);
	media_to_add_parents.append(parent);
}

void TimelineAction::delete_media(QTreeWidgetItem* item) {
    deleted_media.append(item);
}

void TimelineAction::ripple(Sequence* s, long point, long length) {
    QVector<int> i;
    ripple(s, point, length, i);
}

void TimelineAction::add_effect(Sequence* s, int clip, int effect) {
	new_action(s, TA_ADD_EFFECT, clip, TA_NO_TRANSITION, 0, effect);
}

void TimelineAction::add_transition(Sequence* s, int clip, int transition, int type) {
	new_action(s, TA_ADD_TRANSITION, clip, type, 0, transition);
}

void TimelineAction::modify_transition(Sequence* s, int clip, int type, long length) {
	new_action(s, TA_MODIFY_TRANSITION, clip, type, 0, length);
}

void TimelineAction::delete_transition(Sequence* s, int clip, int type) {
	new_action(s, TA_DELETE_TRANSITION, clip, type, 0, 0);
}

void TimelineAction::set_in_out(Sequence* s, bool enabled, long in, long out) {
    change_in_out = true;
    change_in_out_sequence = s;

    old_in_out_enabled = s->using_workarea;
    old_sequence_in = s->workarea_in;
    old_sequence_out = s->workarea_out;

    new_in_out_enabled = enabled;
    new_sequence_in = in;
    new_sequence_out = out;

}

void TimelineAction::ripple(Sequence* s, long point, long length, QVector<int>& ignore) {
    ripple_enabled = true;
    ripple_point = point;
    ripple_length = length;
    ripple_sequence = s;
    ripple_ignores = ignore;
}

void TimelineAction::undo() {
    // re-add deleted media
    for (int i=0;i<deleted_media.size();i++) {
        QTreeWidgetItem* item = deleted_media.at(i);
        QTreeWidgetItem* parent = deleted_media_parents.at(i);
        if (parent == NULL) {
            panel_project->source_table->addTopLevelItem(item);
        } else {
            parent->addChild(item);
        }
    }

    // de-ripple clips
    if (ripple_enabled) {
        for (int i=0;i<rippled_clips.size();i++) {
            Clip* c = ripple_sequence->get_clip(rippled_clips.at(i));
            c->timeline_in -= ripple_length;
            c->timeline_out -= ripple_length;
        }
    }

    for (int i=actions.size()-1;i>=0;i--) {
        switch (actions.at(i)) {
        case TA_IN:
            sequences.at(i)->get_clip(clips.at(i))->timeline_in = old_values.at(i);
            break;
        case TA_OUT:
            sequences.at(i)->get_clip(clips.at(i))->timeline_out = old_values.at(i);
            break;
        case TA_CLIP_IN:
            sequences.at(i)->get_clip(clips.at(i))->clip_in = old_values.at(i);
            break;
        case TA_TRACK:
            sequences.at(i)->get_clip(clips.at(i))->track = old_values.at(i);
            break;
        case TA_DELETE:
            sequences.at(i)->replace_clip(clips.at(i), deleted_clips.at(new_values.at(i)));
            break;
        case TA_ADD_IN:
            sequences.at(i)->get_clip(clips.at(i))->timeline_in -= new_values.at(i);
            break;
        case TA_ADD_OUT:
            sequences.at(i)->get_clip(clips.at(i))->timeline_out -= new_values.at(i);
            break;
        case TA_ADD_CLIP_IN:
            sequences.at(i)->get_clip(clips.at(i))->clip_in -= new_values.at(i);
            break;
        case TA_ADD_TRACK:
            sequences.at(i)->get_clip(clips.at(i))->track -= new_values.at(i);
            break;
        case TA_ADD_EFFECT:
		{
            Clip* c = sequences.at(i)->get_clip(clips.at(i));
            Effect* effect = c->effects.at(old_values.at(i));
            c->effects.removeAt(old_values.at(i));
            delete effect;
		}
            break;
		case TA_ADD_TRANSITION:
		{
			Clip* c = sequences.at(i)->get_clip(clips.at(i));
			if (transition_types.at(i) == TA_OPENING_TRANSITION) {
				delete c->opening_transition;
				c->opening_transition = NULL;
			} else {
				delete c->closing_transition;
				c->closing_transition = NULL;
			}
		}
			break;
		case TA_MODIFY_TRANSITION:
		{
			Clip* c = sequences.at(i)->get_clip(clips.at(i));
			if (transition_types.at(i) == TA_OPENING_TRANSITION) {
				c->opening_transition->length = old_values.at(i);
			} else {
				c->closing_transition->length = old_values.at(i);
			}
		}
			break;
		case TA_DELETE_TRANSITION:
		{
			Clip* c = sequences.at(i)->get_clip(clips.at(i));
			if (transition_types.at(i) == TA_OPENING_TRANSITION) {
				c->opening_transition = create_transition(new_values.at(i), c);
				c->opening_transition->length = old_values.at(i);
			} else {
				c->closing_transition = create_transition(new_values.at(i), c);
				c->closing_transition->length = old_values.at(i);
			}
		}
			break;
        }
    }

    // remove added sequences
    for (int i=0;i<new_sequence_items.size();i++) {
        if (new_sequence_parents.at(i) == NULL) {
            panel_project->source_table->takeTopLevelItem(panel_project->source_table->indexOfTopLevelItem(new_sequence_items.at(i)));
        } else {
            new_sequence_parents.at(i)->removeChild(new_sequence_items.at(i));
        }
    }

    // restore link references to deleted clips
    for (int i=0;i<removed_link_to.size();i++) {
        removed_link_from_sequence.at(i)->get_clip(removed_link_from.at(i))->linked.append(removed_link_to.at(i));
    }

    // delete added clips
	int delete_count = 0;
	for (int i=0;i<clips_to_add.size();i++) {
		sequence_to_add_clips_to.at(i)->destroy_clip(added_indexes.at(i)-delete_count, true);
		delete_count++;
	}

    // remove any added media
	for (int i=0;i<media_to_add.size();i++) {
		if (media_to_add_parents.at(i) == NULL) {
			panel_project->source_table->takeTopLevelItem(panel_project->source_table->indexOfTopLevelItem(media_to_add.at(i)));
		} else {
			media_to_add_parents.at(i)->removeChild(media_to_add.at(i));
		}
	}

    // un-change workarea
    if (change_in_out) {
        change_in_out_sequence->using_workarea = old_in_out_enabled;
        change_in_out_sequence->workarea_in = old_sequence_in;
        change_in_out_sequence->workarea_out = old_sequence_out;
    }

    // un-change current active sequence
    if (change_seq) {
        set_sequence(old_seq);
    }

	mainWindow->setWindowModified(old_project_changed);

    done = false;
}

void TimelineAction::redo() {
    removed_link_from.clear();
    removed_link_to.clear();
    deleted_clips.clear();
    deleted_clips_indices.clear();
    deleted_clip_sequences.clear();
    added_indexes.clear();
    deleted_media_parents.clear();
    removed_link_from_sequence.clear();
    rippled_clips.clear();

    for (int i=0;i<new_sequence_items.size();i++) {
        if (new_sequence_parents.at(i) == NULL) {
            panel_project->source_table->addTopLevelItem(new_sequence_items.at(i));
        } else {
            new_sequence_parents.at(i)->addChild(new_sequence_items.at(i));
        }
    }

    // add any new clips
    if (clips_to_add.size() > 0) {
        QVector<Clip*> copies;
        for (int i=0;i<clips_to_add.size();i++) {
            Clip* original = clips_to_add.at(i);
            Clip* copy = original->copy(sequence_to_add_clips_to.at(i));
            copy->linked.resize(original->linked.size());
            for (int j=0;j<original->linked.size();j++) {
                copy->linked[j] = original->linked.at(j) + sequence_to_add_clips_to.at(i)->clip_count();
            }
            copies.append(copy);
        }
        for (int i=0;i<clips_to_add.size();i++) {
            added_indexes.append(sequence_to_add_clips_to.at(i)->add_clip(copies.at(i)));
        }
    }

    for (int i=0;i<actions.size();i++) {
        switch (actions.at(i)) {
        case TA_IN:
        {
            sequences.at(i)->get_clip(clips.at(i))->timeline_in = new_values.at(i);
        }
            break;
        case TA_OUT:
        {
            sequences.at(i)->get_clip(clips.at(i))->timeline_out = new_values.at(i);
        }
            break;
        case TA_CLIP_IN:
        {
            sequences.at(i)->get_clip(clips.at(i))->clip_in = new_values.at(i);
        }
            break;
        case TA_TRACK:
        {
            sequences.at(i)->get_clip(clips.at(i))->track = new_values.at(i);
        }
            break;
        case TA_DELETE:
        {
            new_values[i] = deleted_clips.size();
            deleted_clips.append(sequences.at(i)->get_clip(clips.at(i)));
            deleted_clips_indices.append(clips.at(i));
            deleted_clip_sequences.append(sequences.at(i));
            sequences.at(i)->replace_clip(clips.at(i), NULL);
        }
            break;
        case TA_ADD_IN:
        {
            sequences.at(i)->get_clip(clips.at(i))->timeline_in += new_values.at(i);
        }
            break;
        case TA_ADD_OUT:
        {
            sequences.at(i)->get_clip(clips.at(i))->timeline_out += new_values.at(i);
        }
            break;
        case TA_ADD_CLIP_IN:
        {
            sequences.at(i)->get_clip(clips.at(i))->clip_in += new_values.at(i);
        }
            break;
        case TA_ADD_TRACK:
        {
            sequences.at(i)->get_clip(clips.at(i))->track += new_values.at(i);
        }
            break;
        case TA_ADD_EFFECT:
        {
            Clip* c = sequences.at(i)->get_clip(clips.at(i));
            old_values[i] = c->effects.size();
			Effect* e = create_effect(new_values.at(i), c);
			c->effects.append(e);
        }
            break;
		case TA_ADD_TRANSITION:
		{
			Clip* c = sequences.at(i)->get_clip(clips.at(i));
			if (transition_types.at(i) == TA_OPENING_TRANSITION) {
				c->opening_transition = create_transition(new_values.at(i), c);
			} else {
				c->closing_transition = create_transition(new_values.at(i), c);
			}
		}
			break;
		case TA_MODIFY_TRANSITION:
		{
            Clip* c = sequences.at(i)->get_clip(clips.at(i));
			if (transition_types.at(i) == TA_OPENING_TRANSITION) {
				old_values[i] = c->opening_transition->length;
				c->opening_transition->length = new_values.at(i);
			} else {
				old_values[i] = c->closing_transition->length;
				c->closing_transition->length = new_values.at(i);
			}
		}
			break;
		case TA_DELETE_TRANSITION:
		{
			Clip* c = sequences.at(i)->get_clip(clips.at(i));
			if (transition_types.at(i) == TA_OPENING_TRANSITION) {
				new_values[i] = c->opening_transition->id;
				old_values[i] = c->opening_transition->length;
				delete c->opening_transition;
				c->opening_transition = NULL;
			} else {
				new_values[i] = c->closing_transition->id;
				old_values[i] = c->closing_transition->length;
				delete c->closing_transition;
				c->closing_transition = NULL;
			}
		}
			break;
        }
    }

    // remove any potential link references from deleted clips
    for (int i=0;i<deleted_clips_indices.size();i++) {
        Sequence* s = deleted_clip_sequences.at(i);
        for (int j=0;j<s->clip_count();j++) {
            Clip* c = s->get_clip(j);
            if (c != NULL) {
                for (int k=0;k<c->linked.size();k++) {
                    if (c->linked.at(k) == deleted_clips_indices.at(i)) {
                        removed_link_from_sequence.append(s);
                        removed_link_from.append(j);
                        removed_link_to.append(deleted_clips_indices.at(i));
                        c->linked.removeAt(k);
                        break;
                    }
                }
            }
        }
    }

    // ripple clips - moves all clips after a certain ripple_point by ripple_length
    if (ripple_enabled) {
        for (int j=0;j<ripple_sequence->clip_count();j++) {
            bool found = false;
            for (int i=0;i<ripple_ignores.size();i++) {
                if (ripple_ignores.at(i) == j) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                Clip* c = ripple_sequence->get_clip(j);
                if (c != NULL && c->timeline_in >= ripple_point) {
                    c->timeline_in += ripple_length;
                    c->timeline_out += ripple_length;
                    rippled_clips.append(j);
                }
            }
        }
    }

    // delete media
	for (int i=0;i<deleted_media.size();i++) {
		QTreeWidgetItem* item = deleted_media.at(i);
		deleted_media_parents.append(item->parent());

		// if we're deleting the open sequence, close it
		if (get_type_from_tree(item) == MEDIA_TYPE_SEQUENCE &&
				get_sequence_from_tree(item) == sequence &&
				!change_seq) {
			old_seq = sequence;
			new_seq = NULL;
			change_seq = true;
		}

		if (item->parent() == NULL) {
			panel_project->source_table->takeTopLevelItem(panel_project->source_table->indexOfTopLevelItem(item));
		} else {
			item->parent()->removeChild(item);
		}
	}

    // add any new media
	for (int i=0;i<media_to_add.size();i++) {
		if (media_to_add_parents.at(i) == NULL) {
			panel_project->source_table->addTopLevelItem(media_to_add.at(i));
		} else {
			media_to_add_parents.at(i)->addChild(media_to_add.at(i));
		}
	}

    // change sequence workarea
    if (change_in_out) {
        change_in_out_sequence->using_workarea = new_in_out_enabled;
        change_in_out_sequence->workarea_in = new_sequence_in;
        change_in_out_sequence->workarea_out = new_sequence_out;
    }

    // change current sequence
    if (change_seq) {
        old_seq = sequence;
        set_sequence(new_seq);
    }

	mainWindow->setWindowModified(true);

    done = true;
}*/

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

ReplaceMediaCommand::ReplaceMediaCommand(QTreeWidgetItem* i, QString s) :
	item(i),
	new_filename(s),
	old_project_changed(mainWindow->isWindowModified())
{
	media = get_footage_from_tree(item);
	old_filename = media->url;
}

void ReplaceMediaCommand::replace(QString& filename) {
	// close any clips currently using this media
	QVector<Sequence*> all_sequences = panel_project->list_all_project_sequences();
	for (int i=0;i<all_sequences.size();i++) {
		Sequence* s = all_sequences.at(i);
        for (int j=0;j<s->clips.size();j++) {
            Clip* c = s->clips.at(j);
			if (c != NULL && c->media == media && c->open) {
				close_clip(c);
				if (c->media_type == MEDIA_TYPE_FOOTAGE) c->cacher->wait();
				c->replaced = true;
			}
		}
	}

	// replace media
	QStringList files;
	files.append(filename);
	panel_project->process_file_list(false, files, NULL, item);
}

void ReplaceMediaCommand::undo() {
	replace(old_filename);

	mainWindow->setWindowModified(old_project_changed);
}

void ReplaceMediaCommand::redo() {
	replace(new_filename);

	mainWindow->setWindowModified(true);
}

ReplaceClipMediaCommand::ReplaceClipMediaCommand(void *a, void *b, int c, int d, bool e) :
	old_media(a),
	new_media(b),
	old_type(c),
	new_type(d),
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
			close_clip(c);
			if (c->media_type == MEDIA_TYPE_FOOTAGE) c->cacher->wait();
		}

		if (undo) {
			if (!preserve_clip_ins) {
				c->clip_in = old_clip_ins.at(i);
			}

			c->media = old_media;
			c->media_type = old_type;
		} else {
			if (!preserve_clip_ins) {
				old_clip_ins.append(c->clip_in);
				c->clip_in = 0;
			}

			c->media = new_media;
			c->media_type = new_type;
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
		deleted_objects.append(c->effects.at(fx_id));
		c->effects.removeAt(fx_id);
	}
	panel_effect_controls->reload_clips();
	done = true;
	mainWindow->setWindowModified(true);
}

MediaMove::MediaMove(SourceTable *s) : table(s), old_project_changed(mainWindow->isWindowModified()) {}

void MediaMove::undo() {
	for (int i=0;i<items.size();i++) {
		if (to == NULL) {
			table->takeTopLevelItem(table->indexOfTopLevelItem(items.at(i)));
		} else {
			to->removeChild(items.at(i));
		}
	}
	for (int i=0;i<items.size();i++) {
		if (froms.at(i) == NULL) {
			table->addTopLevelItem(items.at(i));
		} else {
			froms.at(i)->addChild(items.at(i));
		}
	}
	mainWindow->setWindowModified(old_project_changed);
}

void MediaMove::redo() {
	froms.resize(items.size());
	for (int i=0;i<items.size();i++) {
		QTreeWidgetItem* parent = items.at(i)->parent();
		froms[i] = parent;
		if (parent == NULL) {
			table->takeTopLevelItem(table->indexOfTopLevelItem(items.at(i)));
		} else {
			parent->removeChild(items.at(i));
		}
		if (to == NULL) {
			table->addTopLevelItem(items.at(i));
		} else {
			to->addChild(items.at(i));
		}
	}	
	for (int i=0;i<items.size();i++) {
		if (to == NULL) {
			table->addTopLevelItem(items.at(i));
		} else {
			to->addChild(items.at(i));
		}
	}
	mainWindow->setWindowModified(true);
}

MediaRename::MediaRename() : done(true), old_project_changed(mainWindow->isWindowModified()) {}

void MediaRename::undo() {
	item->setText(0, from);
	done = false;
	mainWindow->setWindowModified(old_project_changed);
}

void MediaRename::redo() {
	if (!done) {
		item->setText(0, to);
	}
	mainWindow->setWindowModified(true);
}

KeyframeMove::KeyframeMove() : old_project_changed(mainWindow->isWindowModified()) {}

void KeyframeMove::undo() {
	for (int i=0;i<rows.size();i++) {
		rows.at(i)->keyframe_times[keyframes.at(i)] -= movement;
	}
	mainWindow->setWindowModified(old_project_changed);
}

void KeyframeMove::redo() {
	for (int i=0;i<rows.size();i++) {
		rows.at(i)->keyframe_times[keyframes.at(i)] += movement;
	}
	mainWindow->setWindowModified(true);
}

KeyframeDelete::KeyframeDelete() :
    disable_keyframes_on_row(NULL),
    old_project_changed(mainWindow->isWindowModified()),
    sorted(false)
{}

void KeyframeDelete::undo() {
	if (disable_keyframes_on_row != NULL) disable_keyframes_on_row->setKeyframing(true);

	int data_index = deleted_keyframe_data.size()-1;
	for (int i=rows.size()-1;i>=0;i--) {
		EffectRow* row = rows.at(i);
		int keyframe_index = keyframes.at(i);

		row->keyframe_times.insert(keyframe_index, deleted_keyframe_times.at(i));
		row->keyframe_types.insert(keyframe_index, deleted_keyframe_types.at(i));

		for (int j=row->fieldCount()-1;j>=0;j--) {
			row->field(j)->keyframe_data.insert(keyframe_index, deleted_keyframe_data.at(data_index));
			data_index--;
		}
	}

	mainWindow->setWindowModified(old_project_changed);
}

void KeyframeDelete::redo() {
	if (!sorted) {
		deleted_keyframe_times.resize(rows.size());
		deleted_keyframe_types.resize(rows.size());
		deleted_keyframe_data.clear();
	}

	for (int i=0;i<keyframes.size();i++) {
		EffectRow* row = rows.at(i);
		int keyframe_index = keyframes.at(i);

		if (!sorted) {
			deleted_keyframe_times[i] = (row->keyframe_times.at(keyframe_index));
			deleted_keyframe_types[i] = (row->keyframe_types.at(keyframe_index));
		}

		row->keyframe_times.removeAt(keyframe_index);
		row->keyframe_types.removeAt(keyframe_index);

		for (int j=0;j<row->fieldCount();j++) {
			if (!sorted) deleted_keyframe_data.append(row->field(j)->keyframe_data.at(keyframe_index));
			row->field(j)->keyframe_data.removeAt(keyframe_index);
		}

		// correct following indices
		if (!sorted) {
			for (int j=i+1;j<keyframes.size();j++) {
				if (rows.at(j) == row && keyframes.at(j) > keyframe_index) {
					keyframes[j]--;
				}
			}
		}
	}

	if (disable_keyframes_on_row != NULL) disable_keyframes_on_row->setKeyframing(false);
	mainWindow->setWindowModified(true);
	sorted = true;
}

KeyframeSet::KeyframeSet(EffectRow* r, int i, long t, bool justMadeKeyframe) :
	old_project_changed(mainWindow->isWindowModified()),
	row(r),
	index(i),
	time(t),
	just_made_keyframe(justMadeKeyframe),
	done(true)
{
	enable_keyframes = !row->isKeyframing();
	if (index != -1) old_values.resize(row->fieldCount());
	new_values.resize(row->fieldCount());
	for (int i=0;i<row->fieldCount();i++) {
		EffectField* field = row->field(i);
		if (index != -1) {
			if (field->type == EFFECT_FIELD_DOUBLE) {
				old_values[i] = static_cast<LabelSlider*>(field->ui_element)->get_drag_start_value();
			} else {
				old_values[i] = field->keyframe_data.at(index);
			}
		}
		new_values[i] = field->get_current_data();
	}
}

void KeyframeSet::undo() {
	if (enable_keyframes) row->setKeyframing(false);

	bool append = (index == -1 || just_made_keyframe);
	if (append) {
		row->keyframe_times.removeLast();
		row->keyframe_types.removeLast();
	}
	for (int i=0;i<row->fieldCount();i++) {
		if (append) {
			row->field(i)->keyframe_data.removeLast();
		} else {
			row->field(i)->keyframe_data[index] = old_values.at(i);
		}
	}

	mainWindow->setWindowModified(old_project_changed);
	done = false;
}

void KeyframeSet::redo() {
	bool append = (index == -1 || (just_made_keyframe && !done));
	if (append) {
		row->keyframe_times.append(time);
		row->keyframe_types.append((row->keyframe_types.size() > 0) ? row->keyframe_types.last() : EFFECT_KEYFRAME_LINEAR);
	}
	for (int i=0;i<row->fieldCount();i++) {
		if (append) {
			row->field(i)->keyframe_data.append(new_values.at(i));
		} else {
			row->field(i)->keyframe_data[index] = new_values.at(i);
		}
	}
	row->setKeyframing(true);

	mainWindow->setWindowModified(true);
	done = true;
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

EditSequenceCommand::EditSequenceCommand(QTreeWidgetItem* i, Sequence *s) :
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
	// update name
	item->setText(0, seq->name);

	// update tooltip
	set_sequence_of_tree(item, seq);

	for (int i=0;i<seq->clips.size();i++) {
		// TODO shift in/out/clipin points to match new frame rate
		//		BUT ALSO copy/paste must need a similar routine, no?
		//		See if one exists or if you have to make one, make it
		//		re-usable

		seq->clips.at(i)->refresh();
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
	closeActiveClips(sequence, true);
}

UpdateFootageTooltip::UpdateFootageTooltip(QTreeWidgetItem *i, Media *m) :
	item(i),
	media(m)
{}

void UpdateFootageTooltip::undo() {
	redo();
}

void UpdateFootageTooltip::redo() {
	update_footage_tooltip(item, media);
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
	panel_timeline->clip_clipboard.insert(pos, clip);
	done = false;
}

void RemoveClipsFromClipboard::redo() {
	clip = panel_timeline->clip_clipboard.at(pos);
	panel_timeline->clip_clipboard.removeAt(pos);
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
