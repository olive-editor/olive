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

#include <QCheckBox>
#include <QUndoStack>
#include <QUndoCommand>
#include <QVector>
#include <QVariant>
#include <QModelIndex>

#include "comboaction.h"

#include "timeline/marker.h"
#include "timeline/selection.h"
#include "effects/keyframe.h"

class Clip;
using ClipPtr = std::shared_ptr<Clip>;

class Sequence;
using SequencePtr = std::shared_ptr<Sequence>;

class Media;
using MediaPtr = std::shared_ptr<Media>;

class Effect;
using EffectPtr = std::shared_ptr<Effect>;

class Transition;
using TransitionPtr = std::shared_ptr<Transition>;

struct EffectMeta;
class EffectRow;
class EffectField;

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
  RippleAction(Sequence* is, long ipoint, long ilength, const QVector<int>& iignore);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  Sequence* s;
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
  ClipPtr ref;
  int index;

  int opening_transition;
  int closing_transition;

  QVector<int> linkClipIndex;
  QVector<int> linkLinkIndex;
};

class ChangeSequenceAction : public OliveAction {
public:
  ChangeSequenceAction(SequencePtr s);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  SequencePtr old_sequence;
  SequencePtr new_sequence;
};

class AddEffectCommand : public OliveAction {
public:
  AddEffectCommand(Clip* c, EffectPtr e, const EffectMeta* m, int insert_pos = -1);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  Clip* clip;
  const EffectMeta* meta;
  EffectPtr ref;
  int pos;
  bool done;
};

class AddTransitionCommand : public OliveAction {
public:
  AddTransitionCommand(Clip* iopen, Clip* iclose, TransitionPtr copy, const EffectMeta* itransition, int ilength);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  Clip* open_;
  Clip* close_;
  TransitionPtr transition_to_copy_;
  const EffectMeta* transition_meta_;
  int length_;
  TransitionPtr old_open_transition_;
  TransitionPtr old_close_transition_;
  TransitionPtr new_transition_ref_;
};

class ModifyTransitionCommand : public OliveAction {
public:
  ModifyTransitionCommand(TransitionPtr t, long ilength);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  TransitionPtr transition_ref_;
  long new_length_;
  long old_length_;
};

class DeleteTransitionCommand : public OliveAction {
public:
  DeleteTransitionCommand(TransitionPtr t);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  TransitionPtr transition_ref_;
  Clip* opened_clip_;
  Clip* closed_clip_;
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

class AddMediaCommand : public OliveAction {
public:
  AddMediaCommand(MediaPtr iitem, Media* iparent);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  MediaPtr item;
  Media* parent;
};

class DeleteMediaCommand : public OliveAction {
public:
  DeleteMediaCommand(MediaPtr i);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  MediaPtr item;
  Media* parent;
};

class AddClipCommand : public OliveAction {
public:
  AddClipCommand(Sequence* s, QVector<ClipPtr>& add);
  virtual ~AddClipCommand() override;
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  Sequence* seq;
  QVector<ClipPtr> clips;
  int link_offset_;
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
  ReplaceMediaCommand(MediaPtr, QString);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  MediaPtr item;
  QString old_filename;
  QString new_filename;
  void replace(QString& filename);
};

class ReplaceClipMediaCommand : public OliveAction {
public:
  ReplaceClipMediaCommand(Media *, Media *, bool);
  virtual void doUndo() override;
  virtual void doRedo() override;
  QVector<ClipPtr> clips;
private:
  Media* old_media;
  Media* new_media;
  bool preserve_clip_ins;
  QVector<int> old_clip_ins;
  void replace(bool undo);
};

class EffectDeleteCommand : public OliveAction {
public:
  EffectDeleteCommand(Effect* e);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  Effect* effect_;
  EffectPtr deleted_obj_;
  Clip* parent_clip_;
  int index_;
};

class MediaMove : public OliveAction {
public:
  MediaMove();
  QVector<MediaPtr> items;
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

class KeyframeAdd : public OliveAction {
public:
  KeyframeAdd(EffectField* ifield, int ii);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  EffectField* field;
  int index;
  EffectKeyframe key;
  bool done;
};

enum SetClipPropertyType {
  kSetClipPropertyAutoscale,
  kSetClipPropertyReversed,
  kSetClipPropertyMaintainAudioPitch,
  kSetClipPropertyEnabled
};

class SetClipProperty : public OliveAction {
public:
  SetClipProperty(SetClipPropertyType type);
  virtual void doUndo() override;
  virtual void doRedo() override;
  void AddSetting(QVector<Clip *> clips, bool setting);
  void AddSetting(Clip *c, bool setting);
private:
  SetClipPropertyType type_;
  QVector<Clip*> clips_;
  QVector<bool> setting_;
  QVector<bool> old_setting_;
  void MainLoop(bool undo);
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
  SetSelectionsCommand(Sequence* s);
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
  EditSequenceCommand(Media *i, SequencePtr s);
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
  SequencePtr seq;

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
  ClipPtr clip;
  bool done;
};

class RenameClipCommand : public OliveAction {
public:
  RenameClipCommand(Clip* clip, QString new_name);
  virtual void doUndo() override;
  virtual void doRedo() override;
private:
  QString old_name_;
  QString new_name_;
  Clip* clip_;
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

class SetIsKeyframing : public OliveAction {
public:
  SetIsKeyframing(EffectRow* irow, bool ib);
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

class KeyframeDataChange : public OliveAction {
public:
  KeyframeDataChange(EffectField* field);

  void SetNewKeyframes();

  virtual void doUndo() override;
  virtual void doRedo() override;

private:
  EffectField* field_;
  QVector<EffectKeyframe> old_keys_;
  QVector<EffectKeyframe> new_keys_;
  QVariant old_persistent_data_;
  QVariant new_persistent_data_;
  bool done_;
};

#endif // UNDO_H
