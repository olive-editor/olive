#include "effectfield.h"

#include "ui/labelslider.h"
#include "ui/colorbutton.h"
#include "ui/texteditex.h"
#include "ui/checkboxex.h"
#include "ui/comboboxex.h"
#include "ui/fontcombobox.h"

#include "effectrow.h"
#include "effect.h"

#include "project/undo.h"
#include "project/clip.h"
#include "project/sequence.h"

#include "io/math.h"
#include <QtMath>

EffectField::EffectField(EffectRow *parent, int t) : parent_row(parent), type(t) {
    switch (t) {
    case EFFECT_FIELD_DOUBLE:
    {
        LabelSlider* ls = new LabelSlider();
        ui_element = ls;
        connect(ls, SIGNAL(valueChanged()), this, SLOT(uiElementChange()));
    }
        break;
    case EFFECT_FIELD_COLOR:
    {
        ColorButton* cb = new ColorButton();
        ui_element = cb;
        connect(cb, SIGNAL(color_changed()), this, SLOT(uiElementChange()));
    }
        break;
    case EFFECT_FIELD_STRING:
    {
        TextEditEx* edit = new TextEditEx();
        edit->setUndoRedoEnabled(true);
        ui_element = edit;
        connect(edit, SIGNAL(textChanged()), this, SLOT(uiElementChange()));
    }
        break;
    case EFFECT_FIELD_BOOL:
    {
        CheckboxEx* cb = new CheckboxEx();
        ui_element = cb;
        connect(cb, SIGNAL(clicked(bool)), this, SLOT(uiElementChange()));
        connect(cb, SIGNAL(toggled(bool)), this, SIGNAL(toggled(bool)));
    }
        break;
    case EFFECT_FIELD_COMBO:
    {
        ComboBoxEx* cb = new ComboBoxEx();
        ui_element = cb;
        connect(cb, SIGNAL(activated(int)), this, SLOT(uiElementChange()));
    }
        break;
    case EFFECT_FIELD_FONT:
    {
        FontCombobox* fcb = new FontCombobox();
        ui_element = fcb;
        connect(fcb, SIGNAL(activated(int)), this, SLOT(uiElementChange()));
    }
        break;
    }
}

QVariant EffectField::get_previous_data() {
    switch (type) {
    case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->getPreviousValue(); break;
    case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->getPreviousValue(); break;
    case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->getPreviousValue(); break;
    case EFFECT_FIELD_BOOL: return !static_cast<QCheckBox*>(ui_element)->isChecked(); break;
    case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->getPreviousIndex(); break;
    case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->getPreviousValue(); break;
    }
    return QVariant();
}

QVariant EffectField::get_current_data() {
    switch (type) {
    case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->value(); break;
    case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->get_color(); break;
    case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->getPlainTextEx(); break;
    case EFFECT_FIELD_BOOL: return static_cast<QCheckBox*>(ui_element)->isChecked(); break;
    case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->currentIndex(); break;
    case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->currentText(); break;
    }
    return QVariant();
}

double EffectField::frameToTimecode(long frame) {
    return ((double) frame / parent_row->parent_effect->parent_clip->sequence->frame_rate);
}

long EffectField::timecodeToFrame(double timecode) {
    return qRound(timecode * parent_row->parent_effect->parent_clip->sequence->frame_rate);
}

void EffectField::set_current_data(const QVariant& data) {
    switch (type) {
    case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->set_value(data.toDouble(), false); break;
    case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->set_color(data.value<QColor>()); break;
    case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->setPlainTextEx(data.toString()); break;
    case EFFECT_FIELD_BOOL: return static_cast<QCheckBox*>(ui_element)->setChecked(data.toBool()); break;
    case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->setCurrentIndexEx(data.toInt()); break;
    case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->setCurrentTextEx(data.toString()); break;
    }
}

void EffectField::get_keyframe_data(double timecode, int &before, int &after, double &progress) {
    int before_keyframe_index = -1;
    int after_keyframe_index = -1;
    long before_keyframe_time = LONG_MIN;
    long after_keyframe_time = LONG_MAX;
    long frame = timecodeToFrame(timecode);

    for (int i=0;i<parent_row->keyframe_times.size();i++) {
        long eval_keyframe_time = parent_row->keyframe_times.at(i);
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

        // TODO routines for bezier - currently this is purely linear
    } else if (before_keyframe_index > -1) {
        before = before_keyframe_index;
        after = before_keyframe_index;
    } else {
        before = after_keyframe_index;
        after = after_keyframe_index;
    }
}

bool EffectField::hasKeyframes() {
    return (parent_row->isKeyframing() && keyframe_data.size() > 0);
}

QVariant EffectField::validate_keyframe_data(double timecode, bool async) {
    if (hasKeyframes()) {
        int before_keyframe;
        int after_keyframe;
        double progress;
        get_keyframe_data(timecode, before_keyframe, after_keyframe, progress);

        int kf_type = (progress < 0.5) ? parent_row->keyframe_types.at(before_keyframe) : parent_row->keyframe_types.at(after_keyframe);
        if (kf_type == KEYFRAME_TYPE_SMOOTH) {
            double b = 4;
            double c = 1.313;
            double x = (b * progress) - (b*0.5);
            progress = (c*(qPow(M_E, x)/(qPow(M_E, x) + 1)))-((c-1)*0.5);
        }

        const QVariant& before_data = keyframe_data.at(before_keyframe);
        switch (type) {
        case EFFECT_FIELD_DOUBLE:
        {
            double value;
            if (before_keyframe == after_keyframe) {
                value = keyframe_data.at(before_keyframe).toDouble();
            } else {
                double before_dbl = keyframe_data.at(before_keyframe).toDouble();
                double after_dbl = keyframe_data.at(after_keyframe).toDouble();
                value = double_lerp(before_dbl, after_dbl, progress);
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
                value = keyframe_data.at(before_keyframe).value<QColor>();
            } else {
                QColor before_data = keyframe_data.at(before_keyframe).value<QColor>();
                QColor after_data = keyframe_data.at(after_keyframe).value<QColor>();
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
        }
    }
    return QVariant();
}

void EffectField::uiElementChange() {
    bool enableKeyframes = !(type == EFFECT_FIELD_DOUBLE && static_cast<LabelSlider*>(ui_element)->is_dragging());
    if (parent_row->isKeyframing()) {
        parent_row->set_keyframe_now(enableKeyframes);
    } else if (enableKeyframes) {
        // set undo
        undo_stack.push(new EffectFieldUndo(this));
    }
    emit changed();
}

QWidget* EffectField::get_ui_element() {
    return ui_element;
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

const QVariant EffectField::get_combo_data(double timecode) {
    validate_keyframe_data(timecode);
    return static_cast<ComboBoxEx*>(ui_element)->currentData();
}

const QString EffectField::get_combo_string(double timecode) {
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

const QString EffectField::get_string_value(double timecode, bool async) {
    if (async && hasKeyframes()) {
        return validate_keyframe_data(timecode, true).toString();
    }
    validate_keyframe_data(timecode);
    return static_cast<TextEditEx*>(ui_element)->getPlainTextEx();
}

void EffectField::set_string_value(const QString& s) {
    static_cast<TextEditEx*>(ui_element)->setPlainTextEx(s);
}

const QString EffectField::get_font_name(double timecode, bool async) {
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
