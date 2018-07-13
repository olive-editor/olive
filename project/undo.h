#ifndef UNDO_H
#define UNDO_H

#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>

#include "project/clip.h"
#include "project/sequence.h"

extern QUndoStack undo_stack;

class TimelineAction : public QUndoCommand {
public:
    TimelineAction();
    ~TimelineAction();
    void add_clips(QVector<Clip*>& add);
    void set_timeline_in(int clip, long value);
    void increase_timeline_in(int clip, long value);
    void set_timeline_out(int clip, long value);
    void increase_timeline_out(int clip, long value);
    void set_clip_in(int clip, long value);
    void increase_clip_in(int clip, long value);
    void set_track(int clip, int value);
    void increase_track(int clip, int value);
    void delete_clip(int clip);
    void undo();
    void redo();
private:
    QVector<int> actions;
    QVector<int> clips;
    QVector<long> old_values;
    QVector<long> new_values;

    QVector<Clip*> deleted_clips;
    QVector<int> deleted_clips_indices;

    QVector<Clip*> clips_to_add;
    QVector<int> added_indexes;

    void new_action(int action, int clip, long old_val, long new_val);

    void offset_links(QVector<Clip*>& clips, int offset);

    QVector<int> removed_link_from;
    QVector<int> removed_link_to;

    bool done;
};

#endif // UNDO_H
