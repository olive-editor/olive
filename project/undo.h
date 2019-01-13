#ifndef UNDO_H
#define UNDO_H

class Media;
class QCheckBox;
class LabelSlider;
class Effect;
class SourceTable;
class EffectRow;
class EffectField;
class Transition;
class EffectGizmo;
struct Clip;
struct Sequence;
struct Footage;
struct EffectMeta;

#include "project/marker.h"
#include "project/selection.h"
#include "project/effectfield.h"

#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>
#include <QVariant>
#include <QModelIndex>

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
	MoveClipAction(Clip* c, long iin, long iout, long iclip_in, int itrack, bool irelative);
	void undo();
	void redo();
private:
	Clip* clip;

	long old_in;
	long old_out;
	long old_clip_in;
	int old_track;

	long new_in;
	long new_out;
	long new_clip_in;
	int new_track;

	bool relative;

	bool old_project_changed;
};

class RippleAction : public QUndoCommand {
public:
	RippleAction(Sequence *is, long ipoint, long ilength, const QVector<int>& iignore);
	void undo();
	void redo();
private:
	Sequence *s;
	long point;
	long length;
	QVector<int> ignore;
	ComboAction* ca;
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

	int opening_transition;
	int closing_transition;

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
	AddEffectCommand(Clip* c, Effect *e, const EffectMeta* m, int insert_pos = -1);
	~AddEffectCommand();
	void undo();
	void redo();
private:
	Clip* clip;
	const EffectMeta* meta;
	Effect* ref;
	int pos;
	bool done;
	bool old_project_changed;
};

class AddTransitionCommand : public QUndoCommand {
public:
	AddTransitionCommand(Clip* c, Clip* s, Transition *copy, const EffectMeta* itransition, int itype, int ilength);
	void undo();
	void redo();
private:
	Clip* clip;
	Clip* secondary;
	Transition* transition_to_copy;
	const EffectMeta* transition;
	int type;
	int length;
	bool old_project_changed;
	int old_ptransition;
	int old_stransition;
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
	DeleteTransitionCommand(Sequence* s, int transition_index);
	~DeleteTransitionCommand();
	void undo();
	void redo();
private:
	Sequence* seq;
	int index;
	Transition* transition;
	Clip* otc;
	Clip* ctc;
	bool old_project_changed;
};

class SetTimelineInOutCommand : public QUndoCommand {
public:
	SetTimelineInOutCommand(Sequence* s, bool enabled, long in, long out);
	void undo();
	void redo();
private:
	Sequence* seq;

    bool old_workarea_enabled;

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
	NewSequenceCommand(Media *s, Media* iparent);
	~NewSequenceCommand();
	void undo();
	void redo();
private:
	Media* seq;
	Media* parent;
	bool done;
	bool old_project_changed;
};

class AddMediaCommand : public QUndoCommand {
public:
	AddMediaCommand(Media* iitem, Media* iparent);
	~AddMediaCommand();
	void undo();
	void redo();
private:
	Media* item;
	Media* parent;
	bool done;
	bool old_project_changed;
};

class DeleteMediaCommand : public QUndoCommand {
public:
	DeleteMediaCommand(Media *i);
	~DeleteMediaCommand();
	void undo();
	void redo();
private:
	Media* item;
	Media* parent;
	bool old_project_changed;
	bool done;
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
	Sequence* s;
	QVector<int> clips;
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
	ReplaceMediaCommand(Media*, QString);
	void undo();
	void redo();
private:
	Media *item;
	QString old_filename;
	QString new_filename;
	bool old_project_changed;
	void replace(QString& filename);
};

class ReplaceClipMediaCommand : public QUndoCommand {
public:
	ReplaceClipMediaCommand(Media *, Media *, bool);
	void undo();
	void redo();
	QVector<Clip*> clips;
private:
	Media* old_media;
	Media* new_media;
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
	MediaMove();
	QVector<Media*> items;
	Media* to;
	void undo();
	void redo();
private:
	QVector<Media*> froms;
	bool old_project_changed;
};

class MediaRename : public QUndoCommand {
public:
	MediaRename(Media* iitem, QString to);
	void undo();
	void redo();
private:
	bool old_project_changed;
	Media* item;
	QString from;
	QString to;
};

class KeyframeDelete : public QUndoCommand {
public:
	KeyframeDelete(EffectField* ifield, int iindex);
	void undo();
	void redo();
private:
	EffectField* field;
	int index;
	bool done;
	EffectKeyframe deleted_key;
	bool old_project_changed;
};

// a more modern version of the above, could probably replace it
// assumes the keyframe already exists
class KeyframeFieldSet : public QUndoCommand {
public:
	KeyframeFieldSet(EffectField* ifield, int ii);
	void undo();
	void redo();
private:
	EffectField* field;
	int index;
	EffectKeyframe key;
	bool done;
	bool old_project_changed;
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
	EditSequenceCommand(Media *i, Sequence* s);
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
	Media* item;
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

class SetLong : public QUndoCommand {
public:
	SetLong(long* pointer, long old_value, long new_value);
	void undo();
	void redo();
private:
	long* p;
	long oldval;
	long newval;
	bool old_project_changed;
};

class SetDouble : public QUndoCommand {
public:
	SetDouble(double* pointer, double old_value, double new_value);
	void undo();
	void redo();
private:
	double* p;
	double oldval;
	double newval;
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
	UpdateFootageTooltip(Media* i);
	void undo();
	void redo();
private:
	Media* item;
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

class SetPointer : public QUndoCommand {
public:
	SetPointer(void** pointer, void* data);
	void undo();
	void redo();
private:
	bool old_changed;
	void** p;
	void* new_data;
	void* old_data;
};

class ReloadEffectsCommand : public QUndoCommand {
public:
	void undo();
	void redo();
};

class SetQVariant : public QUndoCommand {
public:
	SetQVariant(QVariant* itarget, const QVariant& iold, const QVariant& inew);
	void undo();
	void redo();
private:
	QVariant* target;
	QVariant old_val;
	QVariant new_val;
};

class SetKeyframing : public QUndoCommand {
public:
	SetKeyframing(EffectRow* irow, bool ib);
	void undo();
	void redo();
private:
	EffectRow* row;
	bool b;
};

class RefreshClips : public QUndoCommand {
public:
	RefreshClips(Media* m);
	void undo();
	void redo();
private:
	Media* media;
};

#endif // UNDO_H
