#ifndef UNDO_H
#define UNDO_H

class QTreeWidgetItem;
struct Clip;
struct Sequence;

#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>

extern QUndoStack undo_stack;

class TimelineAction : public QUndoCommand {
public:
    TimelineAction();
    ~TimelineAction();
    void add_clips(Sequence* s, QVector<Clip*>& add);
    void set_timeline_in(Sequence* s, int clip, long value);
    void increase_timeline_in(Sequence* s, int clip, long value);
    void set_timeline_out(Sequence* s, int clip, long value);
    void increase_timeline_out(Sequence* s, int clip, long value);
    void set_clip_in(Sequence* s, int clip, long value);
    void increase_clip_in(Sequence* s, int clip, long value);
    void set_track(Sequence* s, int clip, int value);
    void increase_track(Sequence* s, int clip, int value);
    void delete_clip(Sequence* s, int clip);
    void add_media(QTreeWidgetItem* item);
    void delete_media(QTreeWidgetItem* item);
    void undo();
    void redo();
private:
    bool done;

    QVector<Sequence*> sequences;
    QVector<int> actions;
    QVector<int> clips;
    QVector<long> old_values;
    QVector<long> new_values;

    QVector<Clip*> deleted_clips;
    QVector<int> deleted_clips_indices;
    QVector<Sequence*> deleted_clip_sequences;

    QVector<Sequence*> sequence_to_add_clips_to;
    QVector<Clip*> clips_to_add;
    QVector<int> added_indexes;

    QVector<Sequence*> removed_link_from_sequence;
    QVector<int> removed_link_from;
    QVector<int> removed_link_to;

    QVector<QTreeWidgetItem*> deleted_media;
    QVector<QTreeWidgetItem*> deleted_media_parents;

    QVector<QTreeWidgetItem*> media_to_add;

    void new_action(Sequence* s, int action, int clip, long old_val, long new_val);
    void offset_links(QVector<Clip*>& clips, int offset);
};

#endif // UNDO_H
