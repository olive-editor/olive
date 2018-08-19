#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "panels/project.h"
#include "project/undo.h"
#include "ui/labelslider.h"
#include "ui/colorbutton.h"
#include "ui/comboboxex.h"
#include "ui/fontcombobox.h"
#include "ui/checkboxex.h"
#include "project/clip.h"
#include "panels/timeline.h"
#include "panels/effectcontrols.h"

#include "effects/video/transformeffect.h"
#include "effects/video/inverteffect.h"
#include "effects/video/shakeeffect.h"
#include "effects/video/solideffect.h"
#include "effects/video/texteffect.h"

#include "effects/audio/paneffect.h"
#include "effects/audio/volumeeffect.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QTextEdit>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QVector<QString> video_effect_names;
QVector<QString> audio_effect_names;

void init_effects() {
	video_effect_names.resize(VIDEO_EFFECT_COUNT);
	audio_effect_names.resize(AUDIO_EFFECT_COUNT);

	video_effect_names[VIDEO_TRANSFORM_EFFECT] = "Transform";
	video_effect_names[VIDEO_SHAKE_EFFECT] = "Shake";
	video_effect_names[VIDEO_TEXT_EFFECT] = "Text";
	video_effect_names[VIDEO_SOLID_EFFECT] = "Solid";
	video_effect_names[VIDEO_INVERT_EFFECT] = "Invert";

	audio_effect_names[AUDIO_VOLUME_EFFECT] = "Volume";
	audio_effect_names[AUDIO_PAN_EFFECT] = "Pan";
}

Effect* create_effect(int effect_id, Clip* c) {
	if (c->track < 0) {
		switch (effect_id) {
		case VIDEO_TRANSFORM_EFFECT: return new TransformEffect(c); break;
		case VIDEO_SHAKE_EFFECT: return new ShakeEffect(c); break;
		case VIDEO_TEXT_EFFECT: return new TextEffect(c); break;
		case VIDEO_SOLID_EFFECT: return new SolidEffect(c); break;
		case VIDEO_INVERT_EFFECT: return new InvertEffect(c); break;
		}
	} else {
		switch (effect_id) {
		case AUDIO_VOLUME_EFFECT: return new VolumeEffect(c); break;
		case AUDIO_PAN_EFFECT: return new PanEffect(c); break;
		}
	}
	qDebug() << "[ERROR] Invalid effect ID";
	return NULL;
}

double double_lerp(double a, double b, double t) {
    return ((1.0 - t) * a) + (t * b);
}

Effect::Effect(Clip* c, int t, int i) :
	parent_clip(c),
	type(t),
	id(i),
	enable_image(false),
	enable_opengl(false)

{
    container = new CollapsibleWidget();
    if (type == EFFECT_TYPE_VIDEO) {
        container->setText(video_effect_names[i]);
    } else if (type == EFFECT_TYPE_AUDIO) {
        container->setText(audio_effect_names[i]);
	}
    connect(container->enabled_check, SIGNAL(clicked(bool)), this, SLOT(field_changed()));
    ui = new QWidget();

    ui_layout = new QGridLayout();
    ui->setLayout(ui_layout);
	container->setContents(ui);
}

Effect::~Effect() {
	for (int i=0;i<rows.size();i++) {
		delete rows.at(i);
	}
}

void Effect::copy_field_keyframes(Effect* e) {
	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
        e->rows.at(i)->keyframe_times = rows.at(i)->keyframe_times;
        e->rows.at(i)->keyframe_types = rows.at(i)->keyframe_types;
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
            e->rows.at(i)->field(j)->keyframe_data = field->keyframe_data;
		}
	}
}

EffectRow* Effect::add_row(const QString& name) {
	EffectRow* row = new EffectRow(this, ui_layout, name, rows.size());
	rows.append(row);
	return row;
}

EffectRow* Effect::row(int i) {
	return rows.at(i);
}

int Effect::row_count() {
	return rows.size();
}

void Effect::refresh() {}

void Effect::field_changed() {
	panel_viewer->viewer_widget->update();
}

bool Effect::is_enabled() {
    return container->enabled_check->isChecked();
}

void Effect::load(QXmlStreamReader* stream) {
    stream->readNext();
    /*for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		while (!stream->atEnd() && !(stream->name() == "effect" && stream->isEndElement())) {
			stream->readNext();
			if (stream->name() == "row" && stream->isStartElement()) {
				for (int j=0;j<row->fieldCount();j++) {
					EffectField* field = row->field(j);
					while (!stream->atEnd() && !(stream->name() == "effect" && stream->isEndElement())) {
						stream->readNext();
						if (stream->name() == "field" && stream->isStartElement()) {
                            // read all keyframes
                            QVector<EffectKeyframe> keys;
                            while (!stream->atEnd() && (stream->name() == "field" && stream->isEndElement())) {
                                stream->readNext();
                                if (stream->name() == "key" && stream->isStartElement()) {
                                    EffectKeyframe kf;
                                    for (int k=0;k<stream->attributes().size();k++) {
                                        const QXmlStreamAttribute& attr = stream->attributes().at(k);
                                        if (attr.name() == "frame") {
                                            kf.frame = attr.value().toLong();
                                        } else if (attr.name() == "type") {
                                            kf.type = attr.value().toInt();
                                        } else if (attr.name() == "value") {
                                            switch (field->type) {
                                            case EFFECT_FIELD_DOUBLE:
                                                kf.data = attr.value().toDouble();
                                                break;
                                            case EFFECT_FIELD_COLOR:
                                            {
                                                kf.data = QColor(attr.value().toString());
                                            }
                                                break;
                                            case EFFECT_FIELD_STRING:
                                                kf.data = attr.value().toString();
                                                break;
                                            case EFFECT_FIELD_BOOL:
                                                kf.data = (stream->text() == "1");
                                                break;
                                            case EFFECT_FIELD_COMBO:
                                                kf.data = (stream->text().toInt());
                                                break;
                                            case EFFECT_FIELD_FONT:
                                                kf.data = (stream->text().toString());
                                                break;
                                            }
                                        }
                                    }
                                    keys.append(kf);
                                }
                            }
                            field->keyframes = keys;
							break;
						}
					}
				}
				break;
			}
		}
    }*/
}

void Effect::save(QXmlStreamWriter* stream) {
	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
        stream->writeStartElement("row");
        for (int j=0;j<row->keyframe_times.size();j++) {
            stream->writeStartElement("key");
            stream->writeAttribute("frame", QString::number(row->keyframe_times.at(j)));
            stream->writeAttribute("type", QString::number(row->keyframe_types.at(j)));
            stream->writeEndElement(); // key
        }
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			stream->writeStartElement("field");
            for (int k=0;k<field->keyframe_data.size();k++) {
                const QVariant& data = field->keyframe_data.at(k);
                stream->writeStartElement("key");
				switch (field->type) {
                case EFFECT_FIELD_DOUBLE: stream->writeAttribute("value", QString::number(data.toDouble())); break;
                case EFFECT_FIELD_COLOR: stream->writeAttribute("value", data.value<QColor>().name()); break;
                case EFFECT_FIELD_STRING: stream->writeAttribute("value", data.toString()); break;
                case EFFECT_FIELD_BOOL: stream->writeAttribute("value", QString::number(data.toBool())); break;
                case EFFECT_FIELD_COMBO: stream->writeAttribute("value", QString::number(data.toInt())); break;
                case EFFECT_FIELD_FONT: stream->writeAttribute("value", data.toString()); break;
				}
				stream->writeEndElement();
			}
			stream->writeEndElement(); // field
		}
		stream->writeEndElement(); // row
	}
}

Effect* Effect::copy(Clip*) {return NULL;}
void Effect::process_image(long, QImage&) {}
void Effect::process_gl(long, QOpenGLShaderProgram&, int*, int*) {}
void Effect::process_audio(uint8_t*, int) {}

/* Effect Row Definitions */

EffectRow::EffectRow(Effect *parent, QGridLayout *uilayout, const QString &n, int row) :
    parent_effect(parent),
    keyframing(false),
    ui(uilayout),
    name(n),
    ui_row(row)
{
    label = new QLabel(name);
	ui->addWidget(label, row, 0);

    // DEBUG STARTS
    QPushButton* nkf = new QPushButton();
    connect(nkf, SIGNAL(clicked(bool)), this, SLOT(set_keyframe_now()));
    ui->addWidget(nkf, row, 5);
    // DEBUG ENDS
	/*keyframe_enable = new CheckboxEx();
	keyframe_enable->setToolTip("Enable Keyframes");
	ui->addWidget(keyframe_enable, row, 5);*/
}

EffectField* EffectRow::add_field(int type, int colspan) {
    EffectField* field = new EffectField(this, type);
	fields.append(field);
	QWidget* element = field->get_ui_element();
	ui->addWidget(element, ui_row, fields.size(), 1, colspan);
	return field;
}

EffectRow::~EffectRow() {
	for (int i=0;i<fields.size();i++) {
		delete fields.at(i);
	}
}

void EffectRow::set_keyframe_now() {
    set_keyframe(panel_timeline->playhead-parent_effect->parent_clip->timeline_in+parent_effect->parent_clip->clip_in);
}

void EffectRow::set_keyframe(long time) {
    keyframing = true;
    int index = -1;
    for (int i=0;i<keyframe_times.size();i++) {
        if (keyframe_times.at(i) == time) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        keyframe_times.append(time);
        keyframe_types.append((keyframe_types.size() > 0) ? keyframe_types.last() : EFFECT_KEYFRAME_LINEAR);
    }
    for (int i=0;i<fields.size();i++) {
        fields.at(i)->set_keyframe_data(index);
    }
    panel_effect_controls->update_keyframes();
}

EffectField* EffectRow::field(int i) {
	return fields.at(i);
}

int EffectRow::fieldCount() {
	return fields.size();
}

/* Effect Field Definitions */

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
		QTextEdit* edit = new QTextEdit();
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
        connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(uiElementChange()));
	}
		break;
	case EFFECT_FIELD_FONT:
	{
		FontCombobox* fcb = new FontCombobox();
		ui_element = fcb;
        connect(fcb, SIGNAL(currentIndexChanged(int)), this, SLOT(uiElementChange()));
	}
		break;
	}
}

QVariant EffectField::get_current_data() {
    switch (type) {
    case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->value(); break;
    case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->get_color(); break;
    case EFFECT_FIELD_STRING: return static_cast<QTextEdit*>(ui_element)->toPlainText(); break;
    case EFFECT_FIELD_BOOL: return static_cast<QCheckBox*>(ui_element)->isChecked(); break;
    case EFFECT_FIELD_COMBO: return static_cast<ComboBoxEx*>(ui_element)->currentIndex(); break;
    case EFFECT_FIELD_FONT: return static_cast<FontCombobox*>(ui_element)->currentText(); break;
    }
    return QVariant();
}

void EffectField::set_keyframe_data(int i) {
    if (i == -1) {
        keyframe_data.append(get_current_data());
    } else {
        keyframe_data[i] = get_current_data();
    }
}

void EffectField::get_keyframe_data(long frame, int* before, int* after, double* progress) {
    int before_keyframe_index = -1;
    int after_keyframe_index = -1;
    long before_keyframe_time = LONG_MIN;
    long after_keyframe_time = LONG_MAX;

    for (int i=0;i<parent_row->keyframe_times.size();i++) {
        long eval_keyframe_time = parent_row->keyframe_times.at(i);
        if (eval_keyframe_time == frame) {
            *before = i;
            *after = i;
            return;
        } else if (eval_keyframe_time < frame && eval_keyframe_time > before_keyframe_time) {
            before_keyframe_index = i;
            before_keyframe_time = eval_keyframe_time;
        } else if (eval_keyframe_time > frame && eval_keyframe_time < after_keyframe_time) {
            after_keyframe_index = i;
            after_keyframe_time = eval_keyframe_time;
        }
    }

    if (type == EFFECT_FIELD_DOUBLE || type == EFFECT_FIELD_COLOR) {
        if (before_keyframe_index > -1 && after_keyframe_index > -1) {
            // interpolate
            *before = before_keyframe_index;
            *after = after_keyframe_index;
            *progress = (double)(frame-before_keyframe_time)/(double)(after_keyframe_time-before_keyframe_time);

            // TODO routines for bezier - currently this is purely linear
        } else if (before_keyframe_index > -1) {
            *before = before_keyframe_index;
            *after = before_keyframe_index;
        } else if (after_keyframe_index > -1) {
            *before = after_keyframe_index;
            *after = after_keyframe_index;
        }
    } else {
        *before = before_keyframe_index;
        *after = before_keyframe_index;
    }
}

void EffectField::uiElementChange() {
    if (parent_row->keyframing) {
        parent_row->set_keyframe_now();
    }
    emit changed();
}

QWidget* EffectField::get_ui_element() {
	return ui_element;
}

void EffectField::set_enabled(bool e) {
	ui_element->setEnabled(e);
}

double EffectField::get_double_value(long p) {
    if (parent_row->keyframing) {
        int before_keyframe;
        int after_keyframe;
        double progress;
        get_keyframe_data(p, &before_keyframe, &after_keyframe, &progress);
        double value;
        if (before_keyframe == after_keyframe) {
            value = keyframe_data.at(before_keyframe).toDouble();
        } else {
            double before_data = keyframe_data.at(before_keyframe).toDouble();
            double after_data = keyframe_data.at(after_keyframe).toDouble();
            value = double_lerp(before_data, after_data, progress);
        }
        static_cast<LabelSlider*>(ui_element)->set_value(value, false);
        return value;
    } else {
        return static_cast<LabelSlider*>(ui_element)->value();
    }
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

int EffectField::get_combo_index(long p) {
    return static_cast<ComboBoxEx*>(ui_element)->currentIndex();
}

const QVariant EffectField::get_combo_data(long p) {
    return static_cast<ComboBoxEx*>(ui_element)->currentData();
}

const QString EffectField::get_combo_string(long p) {
    return static_cast<ComboBoxEx*>(ui_element)->currentText();
}

void EffectField::set_combo_index(int index) {
	static_cast<ComboBoxEx*>(ui_element)->setCurrentIndexEx(index);
}

void EffectField::set_combo_string(const QString& s) {
	static_cast<ComboBoxEx*>(ui_element)->setCurrentTextEx(s);
}

bool EffectField::get_bool_value(long p) {
	return static_cast<QCheckBox*>(ui_element)->isChecked();
}

void EffectField::set_bool_value(bool b) {
	return static_cast<QCheckBox*>(ui_element)->setChecked(b);
}

const QString EffectField::get_string_value(long p) {
	return static_cast<QTextEdit*>(ui_element)->toPlainText();
}

void EffectField::set_string_value(const QString& s) {
	static_cast<QTextEdit*>(ui_element)->setText(s);
}

const QString EffectField::get_font_name(long p) {
	return static_cast<FontCombobox*>(ui_element)->currentText();
}

void EffectField::set_font_name(const QString& s) {
	static_cast<FontCombobox*>(ui_element)->setCurrentText(s);
}

QColor EffectField::get_color_value(long p) {
    if (parent_row->keyframing) {
        int before_keyframe;
        int after_keyframe;
        double progress;
        get_keyframe_data(p, &before_keyframe, &after_keyframe, &progress);
        if (before_keyframe == after_keyframe) {
            return keyframe_data.at(before_keyframe).value<QColor>();
        } else {
            QColor before_data = keyframe_data.at(before_keyframe).value<QColor>();
            QColor after_data = keyframe_data.at(after_keyframe).value<QColor>();
            return QColor(lerp(before_data.red(), after_data.red(), progress), lerp(before_data.green(), after_data.green(), progress), lerp(before_data.blue(), after_data.blue(), progress));
        }
    }

	return static_cast<ColorButton*>(ui_element)->get_color();
}

void EffectField::set_color_value(QColor color) {
	static_cast<ColorButton*>(ui_element)->set_color(color);
}
