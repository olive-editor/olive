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

#include "effectfield.h"

#include "ui/labelslider.h"
#include "ui/colorbutton.h"
#include "ui/texteditex.h"
#include "ui/checkboxex.h"
#include "ui/comboboxex.h"
#include "ui/fontcombobox.h"
#include "ui/embeddedfilechooser.h"

#include "io/config.h"

#include "effectrow.h"
#include "effect.h"

#include "project/undo.h"
#include "project/clip.h"
#include "project/sequence.h"

#include "io/math.h"

#include <QDateTime>
#include <QtMath>

#include "debug.h"

EffectField::EffectField(EffectRow *parent, int t, const QString &i) :
	parent_row(parent),
	type(t),
	id(i)
{
	switch (t) {
	case EFFECT_FIELD_DOUBLE:
	{
		LabelSlider* ls = new LabelSlider();
		ui_element = ls;
		connect(ls, SIGNAL(valueChanged()), this, SLOT(ui_element_change()));
		connect(ls, SIGNAL(clicked()), this, SIGNAL(clicked()));
	}
		break;
	case EFFECT_FIELD_COLOR:
	{
		ColorButton* cb = new ColorButton();
		ui_element = cb;
		connect(cb, SIGNAL(color_changed()), this, SLOT(ui_element_change()));
	}
		break;
	case EFFECT_FIELD_STRING:
	{
		TextEditEx* edit = new TextEditEx();

		// TODO magic number 2 - i'm not sure how to make this work otherwise though
		edit->setFixedHeight(qCeil(edit->fontMetrics().lineSpacing()*olive::CurrentConfig.effect_textbox_lines + edit->document()->documentMargin() + edit->document()->documentMargin() + 2));

		edit->setUndoRedoEnabled(true);
		ui_element = edit;
		connect(edit, SIGNAL(textChanged()), this, SLOT(ui_element_change()));
	}
		break;
	case EFFECT_FIELD_BOOL:
	{
		CheckboxEx* cb = new CheckboxEx();
		ui_element = cb;
		connect(cb, SIGNAL(clicked(bool)), this, SLOT(ui_element_change()));
		connect(cb, SIGNAL(toggled(bool)), this, SIGNAL(toggled(bool)));
	}
		break;
	case EFFECT_FIELD_COMBO:
	{
		ComboBoxEx* cb = new ComboBoxEx();
		ui_element = cb;
		connect(cb, SIGNAL(activated(int)), this, SLOT(ui_element_change()));
	}
		break;
	case EFFECT_FIELD_FONT:
	{
		FontCombobox* fcb = new FontCombobox();
		ui_element = fcb;
		connect(fcb, SIGNAL(activated(int)), this, SLOT(ui_element_change()));
	}
		break;
	case EFFECT_FIELD_FILE:
	{
		EmbeddedFileChooser* efc = new EmbeddedFileChooser();
		ui_element = efc;
		connect(efc, SIGNAL(changed()), this, SLOT(ui_element_change()));
	}
		break;
	}
}

EffectField::~EffectField() {}

double EffectField::get_validated_keyframe_handle(int key, bool post) {
	int comp_key = -1;

	// find keyframe before or after this one
	for (int i=0;i<keyframes.size();i++) {
		if (i != key
				&& ((keyframes.at(i).time > keyframes.at(key).time) == post)
				&& (comp_key == -1
					|| ((keyframes.at(i).time < keyframes.at(comp_key).time) == post))) {
			// compare with next keyframe for post or previous frame for pre
			comp_key = i;
		}
	}

	double adjusted_key = post ? keyframes.at(key).post_handle_x : keyframes.at(key).pre_handle_x;

	// if this is the earliest/latest keyframe, no validation is required
	if (comp_key == -1) {
		return adjusted_key;
	}

	double comp = keyframes.at(comp_key).time - keyframes.at(key).time;

	// if comp keyframe is bezier, validate with its accompanying handle
	if (keyframes.at(comp_key).type == EFFECT_KEYFRAME_BEZIER) {
		double relative_comp_handle = comp + (post ? keyframes.at(comp_key).pre_handle_x : keyframes.at(comp_key).post_handle_x);
		// return an average
		if ((post && keyframes.at(key).post_handle_x > relative_comp_handle)
				|| (!post && keyframes.at(key).pre_handle_x < relative_comp_handle)) {
			adjusted_key = (adjusted_key + relative_comp_handle)*0.5;
		}
	}

	// don't let handle go beyond the compare keyframe's time
	if (post == (adjusted_key > comp)) {
		return comp;
	}

	if (post == (adjusted_key < 0)) {
		return 0;
	}

	// original value is valid
	return adjusted_key;
}

QVariant EffectField::get_previous_data() {
	switch (type) {
	case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->getPreviousValue();
	case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->getPreviousValue();
	case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->getPreviousValue();
	case EFFECT_FIELD_BOOL: return !static_cast<QCheckBox*>(ui_element)->isChecked();
	case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->getPreviousIndex();
	case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->getPreviousValue();
	case EFFECT_FIELD_FILE: return static_cast<EmbeddedFileChooser*>(ui_element)->getPreviousValue();
	}
	return QVariant();
}

QVariant EffectField::get_current_data() {
	switch (type) {
	case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->value();
	case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->get_color();
	case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->getPlainTextEx();
	case EFFECT_FIELD_BOOL: return static_cast<QCheckBox*>(ui_element)->isChecked();
	case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->currentIndex();
	case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->currentText();
	case EFFECT_FIELD_FILE: return static_cast<EmbeddedFileChooser*>(ui_element)->getFilename();
	}
	return QVariant();
}

double EffectField::frameToTimecode(long frame) {
	return (double(frame) / parent_row->parent_effect->parent_clip->sequence->frame_rate);
}

long EffectField::timecodeToFrame(double timecode) {
	return qRound(timecode * parent_row->parent_effect->parent_clip->sequence->frame_rate);
}

void EffectField::set_current_data(const QVariant& data) {
	switch (type) {
	case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->set_value(data.toDouble(), false);
	case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->set_color(data.value<QColor>());
	case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->setPlainTextEx(data.toString());
	case EFFECT_FIELD_BOOL: return static_cast<QCheckBox*>(ui_element)->setChecked(data.toBool());
	case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->setCurrentIndexEx(data.toInt());
	case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->setCurrentTextEx(data.toString());
	case EFFECT_FIELD_FILE: return static_cast<EmbeddedFileChooser*>(ui_element)->setFilename(data.toString());
	}
}

void EffectField::get_keyframe_data(double timecode, int &before, int &after, double &progress) {
	int before_keyframe_index = -1;
	int after_keyframe_index = -1;
	long before_keyframe_time = LONG_MIN;
	long after_keyframe_time = LONG_MAX;
	long frame = timecodeToFrame(timecode);

	for (int i=0;i<keyframes.size();i++) {
		long eval_keyframe_time = keyframes.at(i).time;
		if (eval_keyframe_time == frame) {
			before = i;
			after = i;
			return;
		} else if (eval_keyframe_time < frame && eval_keyframe_time > before_keyframe_time) {
			before_keyframe_index = i;
			before_keyframe_time = eval_keyframe_time;
		} else if (eval_keyframe_time > frame && eval_keyframe_time < after_keyframe_time) {
			after_keyframe_index = i;
			after_keyframe_time = eval_keyframe_time;
		}
	}

	if ((type == EFFECT_FIELD_DOUBLE || type == EFFECT_FIELD_COLOR) && (before_keyframe_index > -1 && after_keyframe_index > -1)) {
		// interpolate
		before = before_keyframe_index;
		after = after_keyframe_index;
		progress = (timecode-frameToTimecode(before_keyframe_time))/(frameToTimecode(after_keyframe_time)-frameToTimecode(before_keyframe_time));
	} else if (before_keyframe_index > -1) {
		before = before_keyframe_index;
		after = before_keyframe_index;
	} else {
		before = after_keyframe_index;
		after = after_keyframe_index;
	}
}

bool EffectField::hasKeyframes() {
	return (parent_row->isKeyframing() && keyframes.size() > 0);
}

QVariant EffectField::validate_keyframe_data(double timecode, bool async) {
	if (hasKeyframes()) {
		int before_keyframe;
		int after_keyframe;
		double progress;
		get_keyframe_data(timecode, before_keyframe, after_keyframe, progress);

		const QVariant& before_data = keyframes.at(before_keyframe).data;
		switch (type) {
		case EFFECT_FIELD_DOUBLE:
		{
			double value;
			if (before_keyframe == after_keyframe) {
				value = keyframes.at(before_keyframe).data.toDouble();
			} else {
				const EffectKeyframe& before_key = keyframes.at(before_keyframe);
				const EffectKeyframe& after_key = keyframes.at(after_keyframe);

				double before_dbl = before_key.data.toDouble();
				double after_dbl = after_key.data.toDouble();

				if (before_key.type == EFFECT_KEYFRAME_HOLD) {
					// hold
					value = before_dbl;
				} else if (before_key.type == EFFECT_KEYFRAME_BEZIER || after_key.type == EFFECT_KEYFRAME_BEZIER) {
					// bezier interpolation
					if (before_key.type == EFFECT_KEYFRAME_BEZIER && after_key.type == EFFECT_KEYFRAME_BEZIER) {
						// cubic bezier
						double t = cubic_t_from_x(timecode*parent_row->parent_effect->parent_clip->sequence->frame_rate, before_key.time, before_key.time+get_validated_keyframe_handle(before_keyframe, true), after_key.time+get_validated_keyframe_handle(after_keyframe, false), after_key.time);
						value = cubic_from_t(before_dbl, before_dbl+before_key.post_handle_y, after_dbl+after_key.pre_handle_y, after_dbl, t);
					} else if (after_key.type == EFFECT_KEYFRAME_LINEAR) { // quadratic bezier
						// last keyframe is the bezier one
						double t = quad_t_from_x(timecode*parent_row->parent_effect->parent_clip->sequence->frame_rate, before_key.time, before_key.time+get_validated_keyframe_handle(before_keyframe, true), after_key.time);
						value = quad_from_t(before_dbl, before_dbl+before_key.post_handle_y, after_dbl, t);
					} else {
						// this keyframe is the bezier one
						double t = quad_t_from_x(timecode*parent_row->parent_effect->parent_clip->sequence->frame_rate, before_key.time, after_key.time+get_validated_keyframe_handle(after_keyframe, false), after_key.time);
						value = quad_from_t(before_dbl, after_dbl+after_key.pre_handle_y, after_dbl, t);
					}
				} else {
					// linear
					value = double_lerp(before_dbl, after_dbl, progress);
				}
			}
			if (async) {
				return value;
			}
			static_cast<LabelSlider*>(ui_element)->set_value(value, false);
		}
			break;
		case EFFECT_FIELD_COLOR:
		{
			QColor value;
			if (before_keyframe == after_keyframe) {
				value = keyframes.at(before_keyframe).data.value<QColor>();
			} else {
				QColor before_data = keyframes.at(before_keyframe).data.value<QColor>();
				QColor after_data = keyframes.at(after_keyframe).data.value<QColor>();
				value = QColor(lerp(before_data.red(), after_data.red(), progress), lerp(before_data.green(), after_data.green(), progress), lerp(before_data.blue(), after_data.blue(), progress));
			}
			if (async) {
				return value;
			}
			static_cast<ColorButton*>(ui_element)->set_color(value);
		}
			break;
		case EFFECT_FIELD_STRING:
			if (async) {
				return before_data;
			}
			static_cast<TextEditEx*>(ui_element)->setPlainTextEx(before_data.toString());
			break;
		case EFFECT_FIELD_BOOL:
			if (async) {
				return before_data;
			}
			static_cast<QCheckBox*>(ui_element)->setChecked(before_data.toBool());
			break;
		case EFFECT_FIELD_COMBO:
			if (async) {
				return before_data;
			}
			static_cast<ComboBoxEx*>(ui_element)->setCurrentIndexEx(before_data.toInt());
			break;
		case EFFECT_FIELD_FONT:
			if (async) {
				return before_data;
			}
			static_cast<FontCombobox*>(ui_element)->setCurrentTextEx(before_data.toString());
			break;
		case EFFECT_FIELD_FILE:
			if (async) {
				return before_data;
			}
			static_cast<EmbeddedFileChooser*>(ui_element)->setFilename(before_data.toString());
			break;
		}
	}
	return QVariant();
}

void EffectField::ui_element_change() {
	bool dragging_double = (type == EFFECT_FIELD_DOUBLE && static_cast<LabelSlider*>(ui_element)->is_dragging());
	ComboAction* ca = nullptr;
	if (!dragging_double) ca = new ComboAction();
	make_key_from_change(ca);
	if (!dragging_double) olive::UndoStack.push(ca);
	emit changed();
}

void EffectField::make_key_from_change(ComboAction* ca) {
	if (parent_row->isKeyframing()) {
		parent_row->set_keyframe_now(ca);
	} else if (ca != nullptr) {
		// set undo
		ca->append(new EffectFieldUndo(this));
	}
}

QWidget* EffectField::get_ui_element() {
	return ui_element;
}

bool EffectField::is_enabled() {
	return ui_element->isEnabled();
}

void EffectField::set_enabled(bool e) {
	ui_element->setEnabled(e);
}

double EffectField::get_double_value(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).toDouble();
	}
	validate_keyframe_data(timecode);
	return static_cast<LabelSlider*>(ui_element)->value();
}

void EffectField::set_double_value(double v) {
	static_cast<LabelSlider*>(ui_element)->set_value(v, false);
}

void EffectField::set_double_default_value(double v) {
	static_cast<LabelSlider*>(ui_element)->set_default_value(v);
}

void EffectField::set_double_minimum_value(double v) {
	static_cast<LabelSlider*>(ui_element)->set_minimum_value(v);
}

void EffectField::set_double_maximum_value(double v) {
	static_cast<LabelSlider*>(ui_element)->set_maximum_value(v);
}

void EffectField::add_combo_item(const QString& name, const QVariant& data) {
	static_cast<ComboBoxEx*>(ui_element)->addItem(name, data);
}

int EffectField::get_combo_index(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).toInt();
	}
	validate_keyframe_data(timecode);
	return static_cast<ComboBoxEx*>(ui_element)->currentIndex();
}

QVariant EffectField::get_combo_data(double timecode) {
	validate_keyframe_data(timecode);
	return static_cast<ComboBoxEx*>(ui_element)->currentData();
}

QString EffectField::get_combo_string(double timecode) {
	validate_keyframe_data(timecode);
	return static_cast<ComboBoxEx*>(ui_element)->currentText();
}

void EffectField::set_combo_index(int index) {
	static_cast<ComboBoxEx*>(ui_element)->setCurrentIndexEx(index);
}

void EffectField::set_combo_string(const QString& s) {
	static_cast<ComboBoxEx*>(ui_element)->setCurrentTextEx(s);
}

bool EffectField::get_bool_value(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).toBool();
	}
	validate_keyframe_data(timecode);
	return static_cast<QCheckBox*>(ui_element)->isChecked();
}

void EffectField::set_bool_value(bool b) {
	return static_cast<QCheckBox*>(ui_element)->setChecked(b);
}

QString EffectField::get_string_value(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).toString();
	}
	validate_keyframe_data(timecode);
	return static_cast<TextEditEx*>(ui_element)->getPlainTextEx();
}

void EffectField::set_string_value(const QString& s) {
	static_cast<TextEditEx*>(ui_element)->setPlainTextEx(s);
}

QString EffectField::get_font_name(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).toString();
	}
	validate_keyframe_data(timecode);
	return static_cast<FontCombobox*>(ui_element)->currentText();
}

void EffectField::set_font_name(const QString& s) {
	static_cast<FontCombobox*>(ui_element)->setCurrentText(s);
}

QColor EffectField::get_color_value(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).value<QColor>();
	}
	validate_keyframe_data(timecode);
	return static_cast<ColorButton*>(ui_element)->get_color();
}

void EffectField::set_color_value(QColor color) {
	static_cast<ColorButton*>(ui_element)->set_color(color);
}

QString EffectField::get_filename(double timecode, bool async) {
	if (async && hasKeyframes()) {
		return validate_keyframe_data(timecode, true).toString();
	}
	validate_keyframe_data(timecode);
	return static_cast<EmbeddedFileChooser*>(ui_element)->getFilename();
}

void EffectField::set_filename(const QString &s) {
	static_cast<EmbeddedFileChooser*>(ui_element)->setFilename(s);
}
