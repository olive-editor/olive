#include "effectrow.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>

#include "project/undo.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "panels/grapheditor.h"
#include "effect.h"
#include "ui/viewerwidget.h"
#include "ui/keyframenavigator.h"
#include "ui/clickablelabel.h"

EffectRow::EffectRow(Effect *parent, bool save, QGridLayout *uilayout, const QString &n, int row, bool keyframable) :
	parent_effect(parent),
	savable(save),
	keyframing(false),
	ui(uilayout),
	name(n),
	ui_row(row),
	just_made_unsafe_keyframe(false)
{
	label = new ClickableLabel(name + ":");

	ui->addWidget(label, row, 0);

	column_count = 1;

    if (parent_effect->meta != NULL
            && parent_effect->meta->type != EFFECT_TYPE_TRANSITION
            && keyframable) {
		connect(label, SIGNAL(clicked()), this, SLOT(focus_row()));

		keyframe_nav = new KeyframeNavigator();
		connect(keyframe_nav, SIGNAL(goto_previous_key()), this, SLOT(goto_previous_key()));
		connect(keyframe_nav, SIGNAL(toggle_key()), this, SLOT(toggle_key()));
		connect(keyframe_nav, SIGNAL(goto_next_key()), this, SLOT(goto_next_key()));
		connect(keyframe_nav, SIGNAL(keyframe_enabled_changed(bool)), this, SLOT(set_keyframe_enabled(bool)));
		connect(keyframe_nav, SIGNAL(clicked()), this, SLOT(focus_row()));
		ui->addWidget(keyframe_nav, row, 6);
	}
}

bool EffectRow::isKeyframing() {
	return keyframing;
}

void EffectRow::setKeyframing(bool b) {
	if (parent_effect->meta->type != EFFECT_TYPE_TRANSITION) {
		keyframing = b;
		keyframe_nav->enable_keyframes(b);
	}
}

void EffectRow::set_keyframe_enabled(bool enabled) {
	if (enabled) {
		ComboAction* ca = new ComboAction();
		ca->append(new SetKeyframing(this, true));
		set_keyframe_now(ca);
		undo_stack.push(ca);
	} else {
		if (QMessageBox::question(panel_effect_controls, "Disable Keyframes", "Disabling keyframes will delete all current keyframes. Are you sure you want to do this?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
			// clear
			ComboAction* ca = new ComboAction();
			for (int i=0;i<fieldCount();i++) {
				EffectField* f = field(i);
				for (int j=0;j<f->keyframes.size();j++) {
					ca->append(new KeyframeDelete(f, 0));
				}
			}
			ca->append(new SetKeyframing(this, false));
			undo_stack.push(ca);
			panel_effect_controls->update_keyframes();
		} else {
			setKeyframing(true);
		}
	}
}

void EffectRow::goto_previous_key() {
	long key = LONG_MIN;
	Clip* c = parent_effect->parent_clip;
	for (int i=0;i<fieldCount();i++) {
		EffectField* f = field(i);
		for (int j=0;j<f->keyframes.size();j++) {
			long comp = f->keyframes.at(j).time - c->clip_in + c->timeline_in;
			if (comp < sequence->playhead) {
				key = qMax(comp, key);
			}
		}
	}
	if (key != LONG_MIN) panel_sequence_viewer->seek(key);
}

void EffectRow::toggle_key() {
	QVector<EffectField*> key_fields;
	QVector<int> key_field_index;
	Clip* c = parent_effect->parent_clip;
	for (int j=0;j<fieldCount();j++) {
		EffectField* f = field(j);
		for (int i=0;i<f->keyframes.size();i++) {
			long comp = c->timeline_in - c->clip_in + f->keyframes.at(i).time;
			if (comp == sequence->playhead) {
				key_fields.append(f);
				key_field_index.append(i);
			}
		}
	}

	ComboAction* ca = new ComboAction();
	if (key_fields.size() == 0) {
		// keyframe doesn't exist, set one
		set_keyframe_now(ca);
	} else {
		for (int i=0;i<key_fields.size();i++) {
			ca->append(new KeyframeDelete(key_fields.at(i), key_field_index.at(i)));
		}
	}
	undo_stack.push(ca);
	update_ui(false);
}

void EffectRow::goto_next_key() {
	long key = LONG_MAX;
	Clip* c = parent_effect->parent_clip;
	for (int i=0;i<fieldCount();i++) {
		EffectField* f = field(i);
		for (int j=0;j<f->keyframes.size();j++) {
			long comp = f->keyframes.at(j).time - c->clip_in + c->timeline_in;
			if (comp > sequence->playhead) {
				key = qMin(comp, key);
			}
		}
	}
	if (key != LONG_MAX) panel_sequence_viewer->seek(key);
}

void EffectRow::focus_row() {
	panel_graph_editor->set_row(this);
}

EffectField* EffectRow::add_field(int type, const QString& id, int colspan) {
	EffectField* field = new EffectField(this, type, id);
	if (parent_effect->meta->type != EFFECT_TYPE_TRANSITION) connect(field, SIGNAL(clicked()), this, SLOT(focus_row()));
	fields.append(field);
	QWidget* element = field->get_ui_element();
	ui->addWidget(element, ui_row, column_count, 1, colspan);
	column_count++;
	connect(field, SIGNAL(changed()), parent_effect, SLOT(field_changed()));
	return field;
}

void EffectRow::add_widget(QWidget* w) {
	ui->addWidget(w, ui_row, column_count);
	column_count++;
}

EffectRow::~EffectRow() {
	for (int i=0;i<fields.size();i++) {
		delete fields.at(i);
	}
}

void EffectRow::set_keyframe_now(ComboAction* ca) {
	long time = sequence->playhead-parent_effect->parent_clip->timeline_in+parent_effect->parent_clip->clip_in;

	if (!just_made_unsafe_keyframe) {
		EffectKeyframe key;
		key.time = time;

		unsafe_keys.resize(fieldCount());
		unsafe_old_data.resize(fieldCount());
		key_is_new.resize(fieldCount());

		for (int i=0;i<fieldCount();i++) {
			EffectField* f = field(i);

			int exist_key = -1;
			int closest_key = 0;
			for (int j=0;j<f->keyframes.size();j++) {
				if (f->keyframes.at(j).time == time) {
					exist_key = j;
				} else if (f->keyframes.at(j).time < time
						   && f->keyframes.at(closest_key).time < f->keyframes.at(j).time) {
					closest_key = j;
				}
			}
			if (exist_key == -1) {
				key.type = (f->keyframes.size() == 0) ? KEYFRAME_TYPE_LINEAR : f->keyframes.at(closest_key).type;
				key.data = f->get_current_data();//f->keyframes.at(closest_key).data;
				unsafe_keys[i] = f->keyframes.size();
				f->keyframes.append(key);
				key_is_new[i] = true;
			} else {
				unsafe_keys[i] = exist_key;
				key_is_new[i] = false;
			}
			unsafe_old_data[i] = f->get_current_data();
		}
		just_made_unsafe_keyframe = true;
	}

	for (int i=0;i<fieldCount();i++) {
		field(i)->keyframes[unsafe_keys.at(i)].data = field(i)->get_current_data();
	}

	if (ca != NULL)	{
		for (int i=0;i<fieldCount();i++) {
			if (key_is_new.at(i)) ca->append(new KeyframeFieldSet(field(i), unsafe_keys.at(i)));
			ca->append(new SetQVariant(&field(i)->keyframes[unsafe_keys.at(i)].data, unsafe_old_data.at(i), field(i)->get_current_data()));
		}
		unsafe_keys.clear();
		unsafe_old_data.clear();
		just_made_unsafe_keyframe = false;
	}

	panel_effect_controls->update_keyframes();





	/*if (ca != NULL) {
		just_made_unsafe_keyframe = false;
	} else {
		if (!just_made_unsafe_keyframe) {
			just_made_unsafe_keyframe = true;
		}
	}*/


	/*int index = -1;
	long time = sequence->playhead-parent_effect->parent_clip->timeline_in+parent_effect->parent_clip->clip_in;
	for (int j=0;j<fieldCount();j++) {
		EffectField* f = field(j);
		for (int i=0;i<f->keyframes.size();i++) {
			if (f->keyframes.at(i).time == time) {
				index = i;
				break;
			}
		}
	}

	KeyframeSet* ks = new KeyframeSet(this, index, time, just_made_unsafe_keyframe);

	if (ca != NULL) {
		just_made_unsafe_keyframe = false;
		ca->append(ks);
	} else {
		if (index == -1) just_made_unsafe_keyframe = true;
		ks->redo();
		delete ks;
	}

	panel_effect_controls->update_keyframes();*/
}

void EffectRow::delete_keyframe_at_time(ComboAction* ca, long time) {
	for (int j=0;j<fieldCount();j++) {
		EffectField* f = field(j);
		for (int i=0;i<f->keyframes.size();i++) {
			if (f->keyframes.at(i).time == time) {
				ca->append(new KeyframeDelete(f, i));
				break;
			}
		}
	}
}

const QString &EffectRow::get_name() {
	return name;
}

EffectField* EffectRow::field(int i) {
	return fields.at(i);
}

int EffectRow::fieldCount() {
	return fields.size();
}
