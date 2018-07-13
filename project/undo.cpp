#include "undo.h"

#include <QDebug>

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

TimelineAction::TimelineAction() : done(false) {}

TimelineAction::~TimelineAction() {
    for (int i=0;i<clips_to_add.size();i++) {
        delete clips_to_add.at(i);
    }
    if (done) {
        for (int i=0;i<deleted_clips.size();i++) {
            delete deleted_clips.at(i);
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

void TimelineAction::add_clips(QVector<Clip*>& add) {
    offset_links(add, clips_to_add.size());
    clips_to_add.append(add);
}

void TimelineAction::new_action(int action, int clip, long old_val, long new_val) {
    actions.append(action);
    clips.append(clip);
    old_values.append(old_val);
    new_values.append(new_val);
}

void TimelineAction::set_timeline_in(int clip, long value) {
    new_action(TA_IN, clip, sequence->get_clip(clip)->timeline_in, value);
}

void TimelineAction::set_timeline_out(int clip, long value) {
    new_action(TA_OUT, clip, sequence->get_clip(clip)->timeline_out, value);
}

void TimelineAction::set_clip_in(int clip, long value) {
    new_action(TA_CLIP_IN, clip, sequence->get_clip(clip)->clip_in, value);
}

void TimelineAction::set_track(int clip, int value) {
    new_action(TA_TRACK, clip, sequence->get_clip(clip)->track, value);
}

void TimelineAction::increase_timeline_in(int clip, long value) {
    new_action(TA_ADD_IN, clip, 0, value);
}

void TimelineAction::increase_timeline_out(int clip, long value) {
    new_action(TA_ADD_OUT, clip, 0, value);
}

void TimelineAction::increase_clip_in(int clip, long value) {
    new_action(TA_ADD_CLIP_IN, clip, 0, value);
}

void TimelineAction::increase_track(int clip, int value) {
    new_action(TA_ADD_TRACK, clip, 0, value);
}

void TimelineAction::delete_clip(int clip) {
    new_action(TA_DELETE, clip, 0, 0);
}

void TimelineAction::undo() {
    for (int i=0;i<actions.size();i++) {
        switch (actions.at(i)) {
        case TA_IN:
            sequence->get_clip(clips.at(i))->timeline_in = old_values.at(i);
            break;
        case TA_OUT:
            sequence->get_clip(clips.at(i))->timeline_out = old_values.at(i);
            break;
        case TA_CLIP_IN:
            sequence->get_clip(clips.at(i))->clip_in = old_values.at(i);
            break;
        case TA_TRACK:
            sequence->get_clip(clips.at(i))->track = old_values.at(i);
            break;
        case TA_DELETE:
            sequence->replace_clip(clips.at(i), deleted_clips.at(new_values.at(i)));
            break;
        case TA_ADD_IN:
            sequence->get_clip(clips.at(i))->timeline_in -= new_values.at(i);
            break;
        case TA_ADD_OUT:
            sequence->get_clip(clips.at(i))->timeline_out -= new_values.at(i);
            break;
        case TA_ADD_CLIP_IN:
            sequence->get_clip(clips.at(i))->clip_in -= new_values.at(i);
            break;
        case TA_ADD_TRACK:
            sequence->get_clip(clips.at(i))->track -= new_values.at(i);
            break;
        }
    }



    // restore link references to deleted clips
    for (int i=0;i<removed_link_to.size();i++) {
        sequence->get_clip(removed_link_from.at(i))->linked.append(removed_link_to.at(i));
    }

    // delete added clips
    if (clips_to_add.size() > 0) {
        int delete_count = 0;
        for (int i=0;i<clips_to_add.size();i++) {
            sequence->destroy_clip(added_indexes.at(i)-delete_count, true);
            delete_count++;
        }
    }

    done = false;
}

void TimelineAction::redo() {
    removed_link_from.clear();
    removed_link_to.clear();
    deleted_clips.clear();
    deleted_clips_indices.clear();
    added_indexes.clear();

    for (int i=0;i<actions.size();i++) {
        switch (actions.at(i)) {
        case TA_IN:
        {
            sequence->get_clip(clips.at(i))->timeline_in = new_values.at(i);
        }
            break;
        case TA_OUT:
        {
            sequence->get_clip(clips.at(i))->timeline_out = new_values.at(i);
        }
            break;
        case TA_CLIP_IN:
        {
            sequence->get_clip(clips.at(i))->clip_in = new_values.at(i);
        }
            break;
        case TA_TRACK:
        {
            sequence->get_clip(clips.at(i))->track = new_values.at(i);
        }
            break;
        case TA_DELETE:
        {
            new_values[i] = deleted_clips.size();
            deleted_clips.append(sequence->get_clip(clips.at(i)));
            deleted_clips_indices.append(clips.at(i));
            sequence->replace_clip(clips.at(i), NULL);
        }
            break;
        case TA_ADD_IN:
        {
            sequence->get_clip(clips.at(i))->timeline_in += new_values.at(i);
        }
            break;
        case TA_ADD_OUT:
        {
            sequence->get_clip(clips.at(i))->timeline_out += new_values.at(i);
        }
            break;
        case TA_ADD_CLIP_IN:
        {
            sequence->get_clip(clips.at(i))->clip_in += new_values.at(i);
        }
            break;
        case TA_ADD_TRACK:
        {
            sequence->get_clip(clips.at(i))->track += new_values.at(i);
        }
            break;
        }
    }

    // remove any potential link references from deleted clips
    for (int i=0;i<deleted_clips_indices.size();i++) {
        for (int j=0;j<sequence->clip_count();j++) {
            Clip* c = sequence->get_clip(j);
            if (c != NULL) {
                for (int k=0;k<c->linked.size();k++) {
                    if (c->linked.at(k) == deleted_clips_indices.at(i)) {
                        removed_link_from.append(j);
                        removed_link_to.append(deleted_clips_indices.at(i));
                        c->linked.removeAt(k);
                        break;
                    }
                }
            }
        }
    }

    // add any new clips
    if (clips_to_add.size() > 0) {
        int link_offset = sequence->clip_count();
        for (int i=0;i<clips_to_add.size();i++) {
            Clip* original = clips_to_add.at(i);
            Clip* copy = original->copy();
            copy->linked.resize(original->linked.size());
            for (int j=0;j<original->linked.size();j++) {
                copy->linked[j] = original->linked.at(j) + link_offset;
            }
            added_indexes.append(sequence->add_clip(copy));
        }
    }

    done = true;
}
