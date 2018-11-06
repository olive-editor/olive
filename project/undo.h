#ifndef UNDO_H
#define UNDO_H

class QTreeWidgetItem;
class QCheckBox;
class LabelSlider;
class Effect;
class SourceTable;
class EffectRow;
class EffectField;
class Transition;
struct Clip;
struct Sequence;
struct Media;
struct EffectMeta;

#include "project/marker.h"
#include "project/selection.h"

#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>
#include <QVariant>

#define TA_NO_TRANSITION 0
#define TA_OPENING_TRANSITION 1
#define TA_CLOSING_TRANSITION 2

extern QUndoStack undo_stack;

class ComboAction : public QUndoCommand {
public:
    ComboAction();
    ~ComboAction();
    void undo();
    void redo();
    void append(QUndoCommand* u);
	void appendPost(QUndoCommand* u);
private:
    QVector<QUndoCommand*> commands;
	QVector<QUndoCommand*> post_commands;
};

class MoveClipAction : public QUndoCommand {
public:
    MoveClipAction(Clip* c, long iin, long iout, long iclip_in, int itrack);
    void undo();
    void redo();
private:
    Clip* clip;

    long new_in;
    long new_out;
    long new_clip_in;
    int new_track;

    long old_in;
    long old_out;
    long old_clip_in;
    int old_track;

	bool old_project_changed;
};

class DeleteClipAction : public QUndoCommand {
public:
    DeleteClipAction(Sequence* s, int clip);
    ~DeleteClipAction();
    void undo();
    void redo();
private:
    Sequence* seq;
    Clip* ref;
    int index;

	QVector<int> linkClipIndex;
	QVector<int> linkLinkIndex;

	bool old_project_changed;
};

class ChangeSequenceAction : public QUndoCommand {
public:
    ChangeSequenceAction(Sequence* s);
    void undo();
    void redo();
private:
    Sequence* old_sequence;
    Sequence* new_sequence;
};

class AddEffectCommand : public QUndoCommand {
public:
	AddEffectCommand(Clip* c, const EffectMeta* e);
    ~AddEffectCommand();
    void undo();
    void redo();
private:
    Clip* clip;
	const EffectMeta* meta;
    Effect* ref;
    bool done;
	bool old_project_changed;
};

class AddTransitionCommand : public QUndoCommand {
public:
	AddTransitionCommand(Clip* c, const EffectMeta* itransition, int itype, int ilength);
    void undo();
    void redo();
private:
    Clip* clip;
	const EffectMeta* transition;
    int type;
    int length;
	bool old_project_changed;
};

class ModifyTransitionCommand : public QUndoCommand {
public:
    ModifyTransitionCommand(Clip* c, int itype, long ilength);
    void undo();
    void redo();
private:
    Clip* clip;
    int type;
    long new_length;
    long old_length;
	bool old_project_changed;
};

class DeleteTransitionCommand : public QUndoCommand {
public:
    DeleteTransitionCommand(Clip* c, int itype);
    ~DeleteTransitionCommand();
    void undo();
    void redo();
private:
    Clip* clip;
    int type;
	Effect* transition;
	bool old_project_changed;
};

class SetTimelineInOutCommand : public QUndoCommand {
public:
    SetTimelineInOutCommand(Sequence* s, bool enabled, long in, long out);
    void undo();
    void redo();
private:
    Sequence* seq;

    bool old_enabled;
    long old_in;
    long old_out;

    bool new_enabled;
    long new_in;
    long new_out;

	bool old_project_changed;
};

class NewSequenceCommand : public QUndoCommand {
public:
    NewSequenceCommand(QTreeWidgetItem *s, QTreeWidgetItem* iparent);
    ~NewSequenceCommand();
    void undo();
    void redo();
private:
    QTreeWidgetItem* seq;
    QTreeWidgetItem* parent;
    bool done;
	bool old_project_changed;
};

class AddMediaCommand : public QUndoCommand {
public:
    AddMediaCommand(QTreeWidgetItem* iitem, QTreeWidgetItem* iparent);
    ~AddMediaCommand();
    void undo();
    void redo();
private:
    QTreeWidgetItem* item;
    QTreeWidgetItem* parent;
    bool done;
	bool old_project_changed;
};

class DeleteMediaCommand : public QUndoCommand {
public:
    DeleteMediaCommand(QTreeWidgetItem* i);
    ~DeleteMediaCommand();
    void undo();
    void redo();
private:
    QTreeWidgetItem* item;
    QTreeWidgetItem* parent;
	bool old_project_changed;
	bool done;
};

class RippleCommand : public QUndoCommand {
public:
    RippleCommand(Sequence* s, long ipoint, long ilength);
    QVector<Clip*> ignore;
    void undo();
    void redo();
private:
    Sequence* seq;
    long point;
    long length;
    QVector<Clip*> rippled;
	bool old_project_changed;
};

class AddClipCommand : public QUndoCommand {
public:
    AddClipCommand(Sequence* s, QVector<Clip*>& add);
    ~AddClipCommand();
    void undo();
    void redo();
private:
    Sequence* seq;
    QVector<Clip*> clips;
	QVector<Clip*> undone_clips;
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

class EffectFieldUndo : public QUndoCommand {
public:
	EffectFieldUndo(EffectField* field);
	void undo();
	void redo();
private:
	EffectField* field;
	QVariant old_val;
	QVariant new_val;
	bool done;
	bool old_project_changed;
};

class SetAutoscaleAction : public QUndoCommand {
public:
	SetAutoscaleAction();
    void undo();
    void redo();
    QVector<Clip*> clips;
private:
	bool old_project_changed;
};

class AddMarkerAction : public QUndoCommand {
public:
	AddMarkerAction(Sequence* s, long t, QString n);
	void undo();
	void redo();
private:
	Sequence* seq;
	long time;
	QString name;
	QString old_name;
	bool old_project_changed;
	int index;
};

class MoveMarkerAction : public QUndoCommand {
public:
	MoveMarkerAction(Marker* m, long o, long n);
	void undo();
	void redo();
private:
	Marker* marker;
	long old_time;
	long new_time;
	bool old_project_changed;
};

class DeleteMarkerAction : public QUndoCommand {
public:
	DeleteMarkerAction(Sequence* s);
	void undo();
	void redo();
	QVector<int> markers;
private:
	Sequence* seq;
	QVector<Marker> copies;
	bool sorted;
	bool old_project_changed;
};

class SetSpeedAction : public QUndoCommand {
public:
	SetSpeedAction(Clip* c, double speed);
	void undo();
	void redo();
private:
	Clip* clip;
	double old_speed;
	double new_speed;
	bool old_project_changed;
};

class SetBool : public QUndoCommand {
public:
	SetBool(bool* b, bool setting);
	void undo();
	void redo();
private:
	bool* boolean;
	bool old_setting;
	bool new_setting;
	bool old_project_changed;
};

class SetSelectionsCommand : public QUndoCommand {
public:
    SetSelectionsCommand(Sequence *s);
    void undo();
    void redo();
    QVector<Selection> old_data;
    QVector<Selection> new_data;
private:
    Sequence* seq;
    bool done;
	bool old_project_changed;
};

class SetEnableCommand : public QUndoCommand {
public:
	SetEnableCommand(Clip* c, bool enable);
	void undo();
	void redo();
private:
	Clip* clip;
	bool old_val;
	bool new_val;
	bool old_project_changed;
};

class EditSequenceCommand : public QUndoCommand {
public:
	EditSequenceCommand(QTreeWidgetItem *i, Sequence* s);
	void undo();
	void redo();
	void update();

	QString name;
	int width;
	int height;
	double frame_rate;
	int audio_frequency;
	int audio_layout;
private:
	QTreeWidgetItem* item;
	Sequence* seq;
	bool old_project_changed;

	QString old_name;
	int old_width;
	int old_height;
	double old_frame_rate;
	int old_audio_frequency;
	int old_audio_layout;
};

class SetInt : public QUndoCommand {
public:
    SetInt(int* pointer, int new_value);
    void undo();
    void redo();
private:
    int* p;
    int oldval;
    int newval;
	bool old_project_changed;
};

class SetString : public QUndoCommand {
public:
    SetString(QString* pointer, QString new_value);
    void undo();
    void redo();
private:
    QString* p;
    QString oldval;
    QString newval;
	bool old_project_changed;
};

class CloseAllClipsCommand : public QUndoCommand {
public:
	void undo();
	void redo();
};

class UpdateFootageTooltip : public QUndoCommand {
public:
	UpdateFootageTooltip(QTreeWidgetItem* i, Media* m);
	void undo();
	void redo();
private:
	QTreeWidgetItem* item;
	Media* media;
};

class MoveEffectCommand : public QUndoCommand {
public:
	MoveEffectCommand();
	void undo();
	void redo();
	Clip* clip;
	int from;
	int to;
private:
	bool old_project_changed;
};

class RemoveClipsFromClipboard : public QUndoCommand {
public:
	RemoveClipsFromClipboard(int index);
	~RemoveClipsFromClipboard();
	void undo();
	void redo();
private:
	int pos;
	Clip* clip;
	bool old_project_changed;
	bool done;
};

class RenameClipCommand : public QUndoCommand {
public:
	RenameClipCommand();
	QVector<Clip*> clips;
	QString new_name;
	void undo();
	void redo();
private:
	QVector<QString> old_names;
	bool old_project_changed;
};

#endif // UNDO_H
