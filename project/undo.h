#ifndef UNDO_H
#define UNDO_H

class QTreeWidgetItem;
class QCheckBox;
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
    void change_sequence(Sequence* s);
    void new_sequence(QTreeWidgetItem* s, QTreeWidgetItem* parent);
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
    void ripple(Sequence* s, long point, long length, QVector<int> &ignore);
    void undo();
    void redo();
private:
    bool done;

    bool change_seq;
    Sequence* old_seq;
    Sequence* new_seq;

    QVector<QTreeWidgetItem*> new_sequence_items;
    QVector<QTreeWidgetItem*> new_sequence_parents;

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

    Sequence* ripple_sequence;
    bool ripple_enabled;
    long ripple_point;
    long ripple_length;
    QVector<int> rippled_clips;
    QVector<int> ripple_ignores;

    void new_action(Sequence* s, int action, int clip, long old_val, long new_val);
    void offset_links(QVector<Clip*>& clips, int offset);
};

class LinkCommand : public QUndoCommand {
public:
    LinkCommand();
    void undo();
    void redo();
    QVector<Clip*> clips;
    bool link;
private:
    QVector< QVector<int> > old_links;
};

class CheckboxCommand : public QUndoCommand {
public:
    CheckboxCommand(QCheckBox* b);
    ~CheckboxCommand();
    void undo();
    void redo();
private:
    QCheckBox* box;
    bool checked;
    bool done;
};

#endif // UNDO_H
