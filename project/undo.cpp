#include "undo.h"

#include <QDebug>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QCheckBox>

#include "project/clip.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "panels/project.h"
#include "playback/playback.h"
#include "ui/sourcetable.h"

QUndoStack undo_stack;

#define TA_IN 0
#define TA_OUT 1
#define TA_CLIP_IN 2
#define TA_TRACK 3
#define TA_DELETE 4
#define TA_ADD_IN 5
#define TA_ADD_OUT 6
#define TA_ADD_CLIP_IN 7
#define TA_ADD_TRACK 8

TimelineAction::TimelineAction() :
    done(false),
    change_seq(false),
    ripple_enabled(false),
    change_in_out(false)
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

void TimelineAction::new_action(Sequence* s, int action, int clip, long old_val, long new_val) {
    sequences.append(s);
    actions.append(action);
    clips.append(clip);
    old_values.append(old_val);
    new_values.append(new_val);
}

void TimelineAction::set_timeline_in(Sequence* s, int clip, long value) {
    new_action(s, TA_IN, clip, s->get_clip(clip)->timeline_in, value);
}

void TimelineAction::set_timeline_out(Sequence* s, int clip, long value) {
    new_action(s, TA_OUT, clip, s->get_clip(clip)->timeline_out, value);
}

void TimelineAction::set_clip_in(Sequence* s, int clip, long value) {
    new_action(s, TA_CLIP_IN, clip, s->get_clip(clip)->clip_in, value);
}

void TimelineAction::set_track(Sequence* s, int clip, int value) {
    new_action(s, TA_TRACK, clip, s->get_clip(clip)->track, value);
}

void TimelineAction::increase_timeline_in(Sequence* s, int clip, long value) {
    new_action(s, TA_ADD_IN, clip, 0, value);
}

void TimelineAction::increase_timeline_out(Sequence* s, int clip, long value) {
    new_action(s, TA_ADD_OUT, clip, 0, value);
}

void TimelineAction::increase_clip_in(Sequence* s, int clip, long value) {
    new_action(s, TA_ADD_CLIP_IN, clip, 0, value);
}

void TimelineAction::increase_track(Sequence* s, int clip, int value) {
    new_action(s, TA_ADD_TRACK, clip, 0, value);
}

void TimelineAction::delete_clip(Sequence* s, int clip) {
    new_action(s, TA_DELETE, clip, 0, 0);
}

void TimelineAction::add_media(QTreeWidgetItem* item) {
    media_to_add.append(item);
}

void TimelineAction::delete_media(QTreeWidgetItem* item) {
    deleted_media.append(item);
}

void TimelineAction::ripple(Sequence* s, long point, long length) {
    QVector<int> i;
    ripple(s, point, length, i);
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
    if (clips_to_add.size() > 0) {
        int delete_count = 0;
        for (int i=0;i<clips_to_add.size();i++) {
            sequence_to_add_clips_to.at(i)->destroy_clip(added_indexes.at(i)-delete_count, true);
            delete_count++;
        }
    }

    // remove any added media
    if (media_to_add.size() > 0) {
        for (int i=0;i<media_to_add.size();i++) {
            panel_project->source_table->takeTopLevelItem(panel_project->source_table->indexOfTopLevelItem(media_to_add.at(i)));
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
    if (deleted_media.size() > 0) {
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
    }

    // add any new media
    if (media_to_add.size() > 0) {
        for (int i=0;i<media_to_add.size();i++) {
            panel_project->source_table->addTopLevelItem(media_to_add.at(i));
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

    done = true;
}

LinkCommand::LinkCommand() : link(true) {}

void LinkCommand::undo() {
    for (int i=0;i<clips.size();i++) {
        Clip* c = clips.at(i);
        if (link) {
            c->linked.clear();
        } else {
            c->linked = old_links.at(i);
        }
    }
}

void LinkCommand::redo() {
    old_links.clear();
    for (int i=0;i<clips.size();i++) {
        Clip* c = clips.at(i);
        if (link) {
            for (int j=0;j<clips.size();j++) {
                if (i != j) {
                    c->linked.append(j);
                }
            }
        } else {
            old_links.append(c->linked);
            c->linked.clear();
        }
    }
}

CheckboxCommand::CheckboxCommand(QCheckBox* b) : box(b), done(true), checked(box->isChecked()) {}

CheckboxCommand::~CheckboxCommand() {}

void CheckboxCommand::undo() {
    box->setChecked(!checked);
    done = false;
}

void CheckboxCommand::redo() {
    if (!done) {
        box->setChecked(checked);
    }
}
