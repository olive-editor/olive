#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "panels/project.h"
#include "project/undo.h"
#include "project/sequence.h"
#include "ui/labelslider.h"
#include "ui/colorbutton.h"
#include "ui/comboboxex.h"
#include "ui/fontcombobox.h"
#include "ui/checkboxex.h"
#include "ui/texteditex.h"
#include "project/clip.h"
#include "panels/timeline.h"
#include "panels/effectcontrols.h"

#include "effects/video/transformeffect.h"
#include "effects/video/inverteffect.h"
#include "effects/video/shakeeffect.h"
#include "effects/video/solideffect.h"
#include "effects/video/texteffect.h"
#include "effects/video/chromakeyeffect.h"
#include "effects/video/gaussianblureffect.h"
#include "effects/video/cropeffect.h"
#include "effects/video/flipeffect.h"
#include "effects/video/boxblureffect.h"

#include "effects/audio/paneffect.h"
#include "effects/audio/volumeeffect.h"
#include "effects/audio/audionoiseeffect.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMessageBox>
#include <QOpenGLContext>

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
	video_effect_names[VIDEO_CHROMAKEY_EFFECT] = "Chroma Key";
	video_effect_names[VIDEO_GAUSSIANBLUR_EFFECT] = "Gaussian Blur";
	video_effect_names[VIDEO_CROP_EFFECT] = "Crop";
	video_effect_names[VIDEO_FLIP_EFFECT] = "Flip";
    video_effect_names[VIDEO_BOXBLUR_EFFECT] = "Box Blur";

	audio_effect_names[AUDIO_VOLUME_EFFECT] = "Volume";
	audio_effect_names[AUDIO_PAN_EFFECT] = "Pan";
	audio_effect_names[AUDIO_NOISE_EFFECT] = "Noise";
}

Effect* create_effect(int effect_id, Clip* c) {
	if (c->track < 0) {
		switch (effect_id) {
		case VIDEO_TRANSFORM_EFFECT: return new TransformEffect(c); break;
		case VIDEO_SHAKE_EFFECT: return new ShakeEffect(c); break;
		case VIDEO_TEXT_EFFECT: return new TextEffect(c); break;
		case VIDEO_SOLID_EFFECT: return new SolidEffect(c); break;
		case VIDEO_INVERT_EFFECT: return new InvertEffect(c); break;
		case VIDEO_CHROMAKEY_EFFECT: return new ChromaKeyEffect(c); break;
		case VIDEO_GAUSSIANBLUR_EFFECT: return new GaussianBlurEffect(c); break;
		case VIDEO_CROP_EFFECT: return new CropEffect(c);
		case VIDEO_FLIP_EFFECT: return new FlipEffect(c);
        case VIDEO_BOXBLUR_EFFECT: return new BoxBlurEffect(c);
		}
	} else {
		switch (effect_id) {
		case AUDIO_VOLUME_EFFECT: return new VolumeEffect(c); break;
		case AUDIO_PAN_EFFECT: return new PanEffect(c); break;
		case AUDIO_NOISE_EFFECT: return new AudioNoiseEffect(c); break;
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
	enable_shader(false),
	enable_coords(false),
	enable_superimpose(false),
	iterations(1),
	isOpen(false),
	glslProgram(NULL),
	bound(false)
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
	if (isOpen) {
		close();
	}

	for (int i=0;i<rows.size();i++) {
		delete rows.at(i);
	}
}

void Effect::copy_field_keyframes(Effect* e) {
	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		e->rows.at(i)->setKeyframing(rows.at(i)->isKeyframing());
        e->rows.at(i)->keyframe_times = rows.at(i)->keyframe_times;
        e->rows.at(i)->keyframe_types = rows.at(i)->keyframe_types;
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
            e->rows.at(i)->field(j)->set_current_data(field->get_current_data());
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

void Effect::set_enabled(bool b) {
	container->enabled_check->setChecked(b);
}

QVariant load_data_from_string(int type, const QString& string) {
	switch (type) {
	case EFFECT_FIELD_DOUBLE: return string.toDouble(); break;
	case EFFECT_FIELD_COLOR: return QColor(string); break;
	case EFFECT_FIELD_STRING: return string; break;
	case EFFECT_FIELD_BOOL: return (string == "1"); break;
	case EFFECT_FIELD_COMBO: return string.toInt(); break;
	case EFFECT_FIELD_FONT: return string; break;
	}
	return QVariant();
}

QString save_data_to_string(int type, const QVariant& data) {
	switch (type) {
	case EFFECT_FIELD_DOUBLE: return QString::number(data.toDouble()); break;
	case EFFECT_FIELD_COLOR: return data.value<QColor>().name(); break;
	case EFFECT_FIELD_STRING: return data.toString(); break;
	case EFFECT_FIELD_BOOL: return QString::number(data.toBool()); break;
	case EFFECT_FIELD_COMBO: return QString::number(data.toInt()); break;
	case EFFECT_FIELD_FONT: return data.toString(); break;
	}
	return QString();
}

void Effect::load(QXmlStreamReader& stream) {
	int row_count = 0;

	while (!stream.atEnd() && !(stream.name() == "effect" && stream.isEndElement())) {
		stream.readNext();
		if (stream.name() == "row" && stream.isStartElement()) {
			if (row_count < rows.size()) {
				EffectRow* row = rows.at(row_count);
				int field_count = 0;

				while (!stream.atEnd() && !(stream.name() == "row" && stream.isEndElement())) {
					stream.readNext();

					// read keyframes
					if (stream.name() == "keyframes" && stream.isStartElement()) {
						for (int k=0;k<stream.attributes().size();k++) {
							const QXmlStreamAttribute& attr = stream.attributes().at(k);
							if (attr.name() == "enabled") {
								row->setKeyframing(attr.value() == "1");
								break;
							}
						}
						if (row->isKeyframing()) {
							stream.readNext();
							while (!stream.atEnd() && !(stream.name() == "keyframes" && stream.isEndElement())) {
								if (stream.name() == "key" && stream.isStartElement()) {
									long keyframe_frame;
									int keyframe_type;
									for (int k=0;k<stream.attributes().size();k++) {
										const QXmlStreamAttribute& attr = stream.attributes().at(k);
										if (attr.name() == "frame") {
											keyframe_frame = attr.value().toLong();
										} else if (attr.name() == "type") {
											keyframe_type = attr.value().toInt();
										}
									}
									row->keyframe_times.append(keyframe_frame);
									row->keyframe_types.append(keyframe_type);
								}
								stream.readNext();
							}
						}
						stream.readNext();
					}

					// read field
					if (stream.name() == "field" && stream.isStartElement()) {
						if (field_count < row->fieldCount()) {
							EffectField* field = row->field(field_count);

							for (int k=0;k<stream.attributes().size();k++) {
								const QXmlStreamAttribute& attr = stream.attributes().at(k);
								if (attr.name() == "value") {
									field->set_current_data(load_data_from_string(field->type, attr.value().toString()));
									break;
								}
							}

							while (!stream.atEnd() && !(stream.name() == "field" && stream.isEndElement())) {
								stream.readNext();

								// read all keyframes
								if (stream.name() == "key" && stream.isStartElement()) {
									stream.readNext();
									field->keyframe_data.append(load_data_from_string(field->type, stream.text().toString()));
								}
							}
						} else {
							qDebug() << "[ERROR] Too many fields for effect" << id << "row" << row_count << ". Project might be corrupt. (Got" << field_count << ", expected <" << row->fieldCount()-1 << ")";
						}
						field_count++;
					}
				}

			} else {
				qDebug() << "[ERROR] Too many rows for effect" << id << ". Project might be corrupt. (Got" << row_count << ", expected <" << rows.size()-1 << ")";
			}
			row_count++;
		}
	}
}

void Effect::save(QXmlStreamWriter& stream) {
	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
        stream.writeStartElement("row"); // row
        stream.writeStartElement("keyframes"); // keyframes
		stream.writeAttribute("enabled", QString::number(row->isKeyframing()));
        for (int j=0;j<row->keyframe_times.size();j++) {
            stream.writeStartElement("key"); // key
            stream.writeAttribute("frame", QString::number(row->keyframe_times.at(j)));
            stream.writeAttribute("type", QString::number(row->keyframe_types.at(j)));
            stream.writeEndElement(); // key
        }
        stream.writeEndElement(); // keyframes
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
            stream.writeStartElement("field"); // field
			stream.writeAttribute("value", save_data_to_string(field->type, field->get_current_data()));
            for (int k=0;k<field->keyframe_data.size();k++) {
				stream.writeTextElement("key", save_data_to_string(field->type, field->keyframe_data.at(k)));
			}
            stream.writeEndElement(); // field
		}
        stream.writeEndElement(); // row
	}
}

void Effect::open() {
	if (isOpen) {
		qDebug() << "[WARNING] Tried to open an effect that was already open";
		close();
	}
	if (QOpenGLContext::currentContext() == NULL) {
		qDebug() << "[WARNING] No current context to create a shader program for - will retry next repaint";
	} else {
		glslProgram = new QOpenGLShaderProgram();
		if (!vertPath.isEmpty()) glslProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, vertPath);
		if (!fragPath.isEmpty()) glslProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, fragPath);
		glslProgram->link();
		isOpen = true;
	}
}

void Effect::close() {
	if (!isOpen) {
		qDebug() << "[WARNING] Tried to close an effect that was already closed";
	} else {
		delete glslProgram;
	}
	glslProgram = NULL;
	isOpen = false;
}

void Effect::startEffect() {
	if (!isOpen) {
		open();
		qDebug() << "[WARNING] Tried to start a closed effect - opening";
	}
	bound = glslProgram->bind();
}

void Effect::endEffect() {
	if (bound) glslProgram->release();
	bound = false;
}

int Effect::getIterations() {
	return iterations;
}

void Effect::setIterations(int i) {
	iterations = qMax(i, 1);
}

Effect* Effect::copy(Clip* c) {
    Effect* copy = create_effect(id, c);
	copy->set_enabled(is_enabled());
    copy_field_keyframes(copy);
    return copy;
}

void Effect::process_shader(double) {}
void Effect::process_coords(double, GLTextureCoords&) {}
GLuint Effect::process_superimpose(double) {return 0;}
void Effect::process_audio(double, double, quint8*, int, int) {}

SuperimposeEffect::SuperimposeEffect(Clip* c, int t, int i) : Effect(c, t, i), texture(NULL) {
	enable_superimpose = true;
}

void SuperimposeEffect::open() {
	Effect::open();
	texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
}

void SuperimposeEffect::close() {
	Effect::close();
	deleteTexture();
}

GLuint SuperimposeEffect::process_superimpose(double timecode) {
	bool recreate_texture = false;
	int width = parent_clip->getWidth();
	int height = parent_clip->getHeight();

	if (width != img.width() || height != img.height()) {
		img = QImage(width, height, QImage::Format_RGBA8888);
		recreate_texture = true;
	}

	if (valueHasChanged(timecode) || recreate_texture) {
		redraw(timecode);
	}

	if (texture != NULL) {
		if (recreate_texture || texture->width() != img.width() || texture->height() != img.height()) {
			deleteTexture();
			texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
			texture->setData(img);
		} else {
			texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, img.constBits());
		}
		return texture->textureId();
	}
	return 0;

}

void SuperimposeEffect::redraw(double) {}

void SuperimposeEffect::deleteTexture() {
	delete texture;
	texture = NULL;
}

bool SuperimposeEffect::valueHasChanged(double timecode) {
	if (cachedValues.size() == 0) {
		for (int i=0;i<row_count();i++) {
			EffectRow* crow = row(i);
			for (int j=0;j<crow->fieldCount();j++) {
				cachedValues.append(crow->field(j)->get_current_data());
			}
		}
		return true;
	} else {
		bool changed = false;
		int index = 0;
		for (int i=0;i<row_count();i++) {
			EffectRow* crow = row(i);
			for (int j=0;j<crow->fieldCount();j++) {
				EffectField* field = crow->field(j);
				field->validate_keyframe_data(timecode);
				if (cachedValues.at(index) != field->get_current_data()) {
					changed = true;
				}
				cachedValues[index] = field->get_current_data();
				index++;
			}
		}
		return changed;
	}
}

/* Effect Row Definitions */

EffectRow::EffectRow(Effect *parent, QGridLayout *uilayout, const QString &n, int row) :
    parent_effect(parent),
    keyframing(false),
    ui(uilayout),
    name(n),
	ui_row(row),
	just_made_unsafe_keyframe(false)
{
    label = new QLabel(name);
	ui->addWidget(label, row, 0);

	keyframe_enable = new CheckboxEx();
	keyframe_enable->setToolTip("Enable Keyframes");
	connect(keyframe_enable, SIGNAL(clicked(bool)), this, SLOT(set_keyframe_enabled(bool)));
	ui->addWidget(keyframe_enable, row, 6);
}

bool EffectRow::isKeyframing() {
	return keyframing;
}

void EffectRow::setKeyframing(bool b) {
	keyframing = b;
	keyframe_enable->setChecked(b);
}

void EffectRow::set_keyframe_enabled(bool enabled) {
	if (enabled) {
		set_keyframe_now(true);
	} else {
		if (QMessageBox::question(panel_effect_controls, "Disable Keyframes", "Disabling keyframes will delete all current keyframes. Are you sure you want to do this?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
			// clear
			KeyframeDelete* kd = new KeyframeDelete();
			for (int i=keyframe_times.size()-1;i>=0;i--) {
				delete_keyframe(kd, i);
			}
			kd->disable_keyframes_on_row = this;
			undo_stack.push(kd);
			panel_effect_controls->update_keyframes();
		} else {
			setKeyframing(true);
		}
	}
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

void EffectRow::set_keyframe_now(bool undoable) {
	int index = -1;
	long time = panel_timeline->playhead-parent_effect->parent_clip->timeline_in+parent_effect->parent_clip->clip_in;
	for (int i=0;i<keyframe_times.size();i++) {
		if (keyframe_times.at(i) == time) {
			index = i;
			break;
		}
	}

	KeyframeSet* ks = new KeyframeSet(this, index, time, just_made_unsafe_keyframe);

	if (undoable) {
		just_made_unsafe_keyframe = false;
		undo_stack.push(ks);
	} else {
		if (index == -1) just_made_unsafe_keyframe = true;
		ks->redo();
		delete ks;
	}

	panel_effect_controls->update_keyframes();
}

void EffectRow::delete_keyframe_at_time(KeyframeDelete* kd, long time) {
	for (int i=0;i<keyframe_times.size();i++) {
		if (keyframe_times.at(i) == time) {
			delete_keyframe(kd, i);
			break;
		}
	}
}

void EffectRow::delete_keyframe(KeyframeDelete* kd, int index) {
	kd->rows.append(this);
	kd->keyframes.append(index);
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

QVariant EffectField::get_current_data() {
    switch (type) {
    case EFFECT_FIELD_DOUBLE: return static_cast<LabelSlider*>(ui_element)->value(); break;
    case EFFECT_FIELD_COLOR: return static_cast<ColorButton*>(ui_element)->get_color(); break;
	case EFFECT_FIELD_STRING: return static_cast<TextEditEx*>(ui_element)->toPlainText(); break;
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

void EffectField::validate_keyframe_data(double timecode) {
	if (parent_row->isKeyframing() && keyframe_data.size() > 0) {
        int before_keyframe;
        int after_keyframe;
        double progress;
		get_keyframe_data(timecode, before_keyframe, after_keyframe, progress);

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
            return static_cast<ColorButton*>(ui_element)->set_color(value);
        }
            break;
        case EFFECT_FIELD_STRING:
			static_cast<TextEditEx*>(ui_element)->setPlainTextEx(before_data.toString());
            break;
        case EFFECT_FIELD_BOOL:
            static_cast<QCheckBox*>(ui_element)->setChecked(before_data.toBool());
            break;
        case EFFECT_FIELD_COMBO:
            static_cast<ComboBoxEx*>(ui_element)->setCurrentIndexEx(before_data.toInt());
            break;
        case EFFECT_FIELD_FONT:
            static_cast<FontCombobox*>(ui_element)->setCurrentTextEx(before_data.toString());
            break;
        }
    }
}

void EffectField::uiElementChange() {
	if (parent_row->isKeyframing()) {
		parent_row->set_keyframe_now(!(type == EFFECT_FIELD_DOUBLE && static_cast<LabelSlider*>(ui_element)->is_dragging()));
    }
    emit changed();
}

QWidget* EffectField::get_ui_element() {
	return ui_element;
}

void EffectField::set_enabled(bool e) {
	ui_element->setEnabled(e);
}

double EffectField::get_double_value(double timecode) {
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

int EffectField::get_combo_index(double timecode) {
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

bool EffectField::get_bool_value(double timecode) {
	validate_keyframe_data(timecode);
	return static_cast<QCheckBox*>(ui_element)->isChecked();
}

void EffectField::set_bool_value(bool b) {
	return static_cast<QCheckBox*>(ui_element)->setChecked(b);
}

const QString EffectField::get_string_value(double timecode) {
	validate_keyframe_data(timecode);
	return static_cast<TextEditEx*>(ui_element)->toPlainText();
}

void EffectField::set_string_value(const QString& s) {
	static_cast<TextEditEx*>(ui_element)->setText(s);
}

const QString EffectField::get_font_name(double timecode) {
	validate_keyframe_data(timecode);
	return static_cast<FontCombobox*>(ui_element)->currentText();
}

void EffectField::set_font_name(const QString& s) {
	static_cast<FontCombobox*>(ui_element)->setCurrentText(s);
}

QColor EffectField::get_color_value(double timecode) {
	validate_keyframe_data(timecode);
	return static_cast<ColorButton*>(ui_element)->get_color();
}

void EffectField::set_color_value(QColor color) {
	static_cast<ColorButton*>(ui_element)->set_color(color);
}
