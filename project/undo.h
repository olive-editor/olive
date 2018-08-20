#ifndef UNDO_H
#define UNDO_H

class QTreeWidgetItem;
class QCheckBox;
class LabelSlider;
class Effect;
class SourceTable;
class EffectRow;
struct Clip;
struct Sequence;
struct Media;

#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>

#define TA_NO_TRANSITION 0
#define TA_OPENING_TRANSITION 1
#define TA_CLOSING_TRANSITION 2

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
	void add_media(QTreeWidgetItem* item, QTreeWidgetItem* parent);
    void delete_media(QTreeWidgetItem* item);
    void ripple(Sequence* s, long point, long length);
    void ripple(Sequence* s, long point, long length, QVector<int> &ignore);
    void set_in_out(Sequence* s, bool enabled, long in, long out);
	void add_effect(Sequence* s, int clip, int effect);
	void add_transition(Sequence* s, int clip, int transition, int type);
	void modify_transition(Sequence* s, int clip, int type, long length);
	void delete_transition(Sequence* s, int clip, int type);
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
	QVector<int> transition_types;
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
	QVector<QTreeWidgetItem*> media_to_add_parents;

    Sequence* ripple_sequence;
    bool ripple_enabled;
    long ripple_point;
    long ripple_length;
    QVector<int> rippled_clips;
    QVector<int> ripple_ignores;

    bool change_in_out;
    Sequence* change_in_out_sequence;
    bool old_in_out_enabled;
    bool new_in_out_enabled;
    long old_sequence_in;
    long new_sequence_in;
    long old_sequence_out;
    long new_sequence_out;

	void new_action(Sequence* s, int action, int clip, int transition_type, long old_val, long new_val);
    void offset_links(QVector<Clip*>& clips, int offset);

	bool old_project_changed;
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
	bool old_project_changed;
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
	bool old_project_changed;
};

class ReplaceMediaCommand : public QUndoCommand {
public:
	ReplaceMediaCommand(QTreeWidgetItem*, QString);
	void undo();
	void redo();
private:
	QTreeWidgetItem *item;
	QString old_filename;
	QString new_filename;
	bool old_project_changed;
	Media* media;
	void replace(QString& filename);
};

class ReplaceClipMediaCommand : public QUndoCommand {
public:
	ReplaceClipMediaCommand(void*, void*, int, int, bool);
	void undo();
	void redo();
	QVector<Clip*> clips;
private:
	void* old_media;
	void* new_media;
	int old_type;
	int new_type;
	bool preserve_clip_ins;
	bool old_project_changed;
	QVector<int> old_clip_ins;
	void replace(bool undo);
};

class EffectDeleteCommand : public QUndoCommand {
public:
	EffectDeleteCommand();
	~EffectDeleteCommand();
	void undo();
	void redo();
	QVector<Clip*> clips;
	QVector<int> fx;
private:
	bool done;
	bool old_project_changed;
	QVector<Effect*> deleted_objects;
};

class MediaMove : public QUndoCommand {
public:
	MediaMove(SourceTable* s);
	QVector<QTreeWidgetItem*> items;
	QTreeWidgetItem* to;
	void undo();
	void redo();
private:
	QVector<QTreeWidgetItem*> froms;
	SourceTable* table;
	bool old_project_changed;
};

class MediaRename : public QUndoCommand {
public:
	MediaRename();
	QTreeWidgetItem* item;
	QString from;
	QString to;
	void undo();
	void redo();
private:
	bool done;
	bool old_project_changed;
};

class KeyframeMove : public QUndoCommand {
public:
	KeyframeMove();
	QVector<EffectRow*> rows;
	QVector<int> keyframes;
	long movement;
	void undo();
	void redo();
private:
	bool old_project_changed;
};

class KeyframeDelete : public QUndoCommand {
public:
	KeyframeDelete();
	QVector<EffectRow*> rows;
	EffectRow* disable_keyframes_on_row;
	QVector<int> keyframes;
	void undo();
	void redo();
private:
	bool old_project_changed;
	QVector<long> deleted_keyframe_times;
	QVector<int> deleted_keyframe_types;
	QVector<QVariant> deleted_keyframe_data;
	bool sorted;
};


class KeyframeSet : public QUndoCommand {
public:
	KeyframeSet(EffectRow* r, int i, long t, bool justMadeKeyframe);
	void undo();
	void redo();
	QVector<QVariant> old_values;
	QVector<QVariant> new_values;
private:
	bool old_project_changed;
	EffectRow* row;
	int index;
	long time;
	bool enable_keyframes;
	bool just_made_keyframe;
	bool done;
};

#endif // UNDO_H
