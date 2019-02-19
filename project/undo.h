/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

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
class Clip;
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

namespace Olive {
    extern QUndoStack UndoStack;
}

class ComboAction : public QUndoCommand {
public:
	ComboAction();
    virtual ~ComboAction() override;
    virtual void undo() override;
    virtual void redo() override;
	void append(QUndoCommand* u);
	void appendPost(QUndoCommand* u);
private:
	QVector<QUndoCommand*> commands;
	QVector<QUndoCommand*> post_commands;
};

class OliveAction : public QUndoCommand {
public:
    OliveAction(bool iset_window_modified = true);
    virtual ~OliveAction() override;

    virtual void undo() override;
    virtual void redo() override;

    virtual void doUndo() = 0;
    virtual void doRedo() = 0;
private:
    /**
     * @brief Setting whether to change the windowModified state of MainWindow
     */
    bool set_window_modified;

    /**
     * @brief Cache previous window modified value to return to if the user undoes this action
     */
    bool old_window_modified;
};

class MoveClipAction : public OliveAction {
public:
	MoveClipAction(Clip* c, long iin, long iout, long iclip_in, int itrack, bool irelative);
    virtual void doUndo() override;
    virtual void doRedo() override;
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
};

class RippleAction : public OliveAction {
public:
	RippleAction(Sequence *is, long ipoint, long ilength, const QVector<int>& iignore);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Sequence *s;
	long point;
	long length;
	QVector<int> ignore;
	ComboAction* ca;
};

class DeleteClipAction : public OliveAction {
public:
	DeleteClipAction(Sequence* s, int clip);
    virtual ~DeleteClipAction() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Sequence* seq;
	Clip* ref;
	int index;

	int opening_transition;
	int closing_transition;

	QVector<int> linkClipIndex;
    QVector<int> linkLinkIndex;
};

class ChangeSequenceAction : public OliveAction {
public:
	ChangeSequenceAction(Sequence* s);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Sequence* old_sequence;
	Sequence* new_sequence;
};

class AddEffectCommand : public OliveAction {
public:
	AddEffectCommand(Clip* c, Effect *e, const EffectMeta* m, int insert_pos = -1);
    virtual ~AddEffectCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Clip* clip;
	const EffectMeta* meta;
	Effect* ref;
	int pos;
    bool done;
};

class AddTransitionCommand : public OliveAction {
public:
	AddTransitionCommand(Clip* c, Clip* s, Transition *copy, const EffectMeta* itransition, int itype, int ilength);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Clip* clip;
	Clip* secondary;
	Transition* transition_to_copy;
	const EffectMeta* transition;
	int type;
    int length;
	int old_ptransition;
	int old_stransition;
};

class ModifyTransitionCommand : public OliveAction {
public:
	ModifyTransitionCommand(Clip* c, int itype, long ilength);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Clip* clip;
	int type;
	long new_length;
    long old_length;
};

class DeleteTransitionCommand : public OliveAction {
public:
	DeleteTransitionCommand(Sequence* s, int transition_index);
    virtual ~DeleteTransitionCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Sequence* seq;
	int index;
	Transition* transition;
	Clip* otc;
    Clip* ctc;
};

class SetTimelineInOutCommand : public OliveAction {
public:
	SetTimelineInOutCommand(Sequence* s, bool enabled, long in, long out);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Sequence* seq;

	bool old_enabled;
	long old_in;
	long old_out;

	bool new_enabled;
	long new_in;
    long new_out;
};

class NewSequenceCommand : public OliveAction {
public:
	NewSequenceCommand(Media *s, Media* iparent);
    virtual ~NewSequenceCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media* seq;
	Media* parent;
    bool done;
};

class AddMediaCommand : public OliveAction {
public:
	AddMediaCommand(Media* iitem, Media* iparent);
    virtual ~AddMediaCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media* item;
	Media* parent;
    bool done;
};

class DeleteMediaCommand : public OliveAction {
public:
	DeleteMediaCommand(Media *i);
    virtual ~DeleteMediaCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media* item;
    Media* parent;
	bool done;
};

class AddClipCommand : public OliveAction {
public:
	AddClipCommand(Sequence* s, QVector<Clip*>& add);
    virtual ~AddClipCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Sequence* seq;
	QVector<Clip*> clips;
    QVector<Clip*> undone_clips;
};

class LinkCommand : public OliveAction {
public:
	LinkCommand();
    virtual void doUndo() override;
    virtual void doRedo() override;
	Sequence* s;
	QVector<int> clips;
	bool link;
private:
    QVector< QVector<int> > old_links;
};

class CheckboxCommand : public OliveAction {
public:
	CheckboxCommand(QCheckBox* b);
    virtual ~CheckboxCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	QCheckBox* box;
	bool checked;
    bool done;
};

class ReplaceMediaCommand : public OliveAction {
public:
	ReplaceMediaCommand(Media*, QString);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media *item;
	QString old_filename;
    QString new_filename;
	void replace(QString& filename);
};

class ReplaceClipMediaCommand : public OliveAction {
public:
	ReplaceClipMediaCommand(Media *, Media *, bool);
    virtual void doUndo() override;
    virtual void doRedo() override;
	QVector<Clip*> clips;
private:
	Media* old_media;
	Media* new_media;
    bool preserve_clip_ins;
	QVector<int> old_clip_ins;
	void replace(bool undo);
};

class EffectDeleteCommand : public OliveAction {
public:
	EffectDeleteCommand();
    virtual ~EffectDeleteCommand() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
	QVector<Clip*> clips;
	QVector<int> fx;
private:
    bool done;
	QVector<Effect*> deleted_objects;
};

class MediaMove : public OliveAction {
public:
	MediaMove();
	QVector<Media*> items;
	Media* to;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
    QVector<Media*> froms;
};

class MediaRename : public OliveAction {
public:
	MediaRename(Media* iitem, QString to);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media* item;
	QString from;
	QString to;
};

class KeyframeDelete : public OliveAction {
public:
	KeyframeDelete(EffectField* ifield, int iindex);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	EffectField* field;
	int index;
	bool done;
    EffectKeyframe deleted_key;
};

// a more modern version of the above, could probably replace it
// assumes the keyframe already exists
class KeyframeFieldSet : public OliveAction {
public:
	KeyframeFieldSet(EffectField* ifield, int ii);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	EffectField* field;
	int index;
	EffectKeyframe key;
    bool done;
};

class EffectFieldUndo : public OliveAction {
public:
	EffectFieldUndo(EffectField* field);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	EffectField* field;
	QVariant old_val;
	QVariant new_val;
    bool done;
};

class SetAutoscaleAction : public OliveAction {
public:
	SetAutoscaleAction();
    virtual void doUndo() override;
    virtual void doRedo() override;
    QVector<Clip*> clips;
};

class AddMarkerAction : public OliveAction {
public:
	AddMarkerAction(QVector<Marker>* m, long t, QString n);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	QVector<Marker>* active_array;
	long time;
	QString name;
    QString old_name;
	int index;
};

class MoveMarkerAction : public OliveAction {
public:
	MoveMarkerAction(Marker* m, long o, long n);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Marker* marker;
	long old_time;
    long new_time;
};

class DeleteMarkerAction : public OliveAction {
public:
	DeleteMarkerAction(QVector<Marker>* m);
    virtual void doUndo() override;
    virtual void doRedo() override;
	QVector<int> markers;
private:
	QVector<Marker>* active_array;
	QVector<Marker> copies;
    bool sorted;
};

class SetSpeedAction : public OliveAction {
public:
	SetSpeedAction(Clip* c, double speed);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Clip* clip;
	double old_speed;
    double new_speed;
};

class SetBool : public OliveAction {
public:
	SetBool(bool* b, bool setting);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	bool* boolean;
	bool old_setting;
    bool new_setting;
};

class SetSelectionsCommand : public OliveAction {
public:
	SetSelectionsCommand(Sequence *s);
    virtual void doUndo() override;
    virtual void doRedo() override;
	QVector<Selection> old_data;
	QVector<Selection> new_data;
private:
	Sequence* seq;
    bool done;
};

class EditSequenceCommand : public OliveAction {
public:
	EditSequenceCommand(Media *i, Sequence* s);
    virtual void doUndo() override;
    virtual void doRedo() override;
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

	QString old_name;
	int old_width;
	int old_height;
	double old_frame_rate;
	int old_audio_frequency;
	int old_audio_layout;
};

class SetInt : public OliveAction {
public:
	SetInt(int* pointer, int new_value);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	int* p;
	int oldval;
    int newval;
};

class SetLong : public OliveAction {
public:
	SetLong(long* pointer, long old_value, long new_value);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	long* p;
	long oldval;
    long newval;
};

class SetDouble : public OliveAction {
public:
	SetDouble(double* pointer, double old_value, double new_value);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	double* p;
	double oldval;
    double newval;
};

class SetString : public OliveAction {
public:
	SetString(QString* pointer, QString new_value);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	QString* p;
	QString oldval;
    QString newval;
};

class CloseAllClipsCommand : public OliveAction {
public:
    virtual void doUndo() override;
    virtual void doRedo() override;
};

class UpdateFootageTooltip : public OliveAction {
public:
	UpdateFootageTooltip(Media* i);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media* item;
};

class MoveEffectCommand : public OliveAction {
public:
	MoveEffectCommand();
    virtual void doUndo() override;
    virtual void doRedo() override;
	Clip* clip;
	int from;
    int to;
};

class RemoveClipsFromClipboard : public OliveAction {
public:
	RemoveClipsFromClipboard(int index);
    virtual ~RemoveClipsFromClipboard() override;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	int pos;
    Clip* clip;
	bool done;
};

class RenameClipCommand : public OliveAction {
public:
	RenameClipCommand();
	QVector<Clip*> clips;
	QString new_name;
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
    QVector<QString> old_names;
};

class SetPointer : public OliveAction {
public:
	SetPointer(void** pointer, void* data);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	bool old_changed;
	void** p;
	void* new_data;
	void* old_data;
};

class ReloadEffectsCommand : public OliveAction {
public:
    virtual void doUndo() override;
    virtual void doRedo() override;
};

class SetQVariant : public OliveAction {
public:
	SetQVariant(QVariant* itarget, const QVariant& iold, const QVariant& inew);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	QVariant* target;
	QVariant old_val;
	QVariant new_val;
};

class SetKeyframing : public OliveAction {
public:
	SetKeyframing(EffectRow* irow, bool ib);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	EffectRow* row;
	bool b;
};

class RefreshClips : public OliveAction {
public:
	RefreshClips(Media* m);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Media* media;
};

class UpdateViewer : public OliveAction {
public:
    virtual void doUndo() override;
    virtual void doRedo() override;
};

class SetEffectData : public OliveAction {
public:
	SetEffectData(Effect* e, const QByteArray &s);
    virtual void doUndo() override;
    virtual void doRedo() override;
private:
	Effect* effect;
	QByteArray data;
	QByteArray old_data;
};

#endif // UNDO_H
