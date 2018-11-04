#include "effect.h"

#include "effects/qpainterwrapper.h"
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
#include "debug.h"
#include "io/path.h"
#include "mainwindow.h"

#include "effects/internal/transformeffect.h"
#include "effects/internal/texteffect.h"
#include "effects/internal/solideffect.h"
#include "effects/internal/audionoiseeffect.h"
#include "effects/internal/toneeffect.h"
#include "effects/internal/volumeeffect.h"
#include "effects/internal/paneffect.h"
#include "effects/internal/shakeeffect.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QDir>
#include <QPainter>
#include <QtMath>
#include <QMenu>

QVector<EffectMeta> video_effects;
QVector<EffectMeta> audio_effects;
QMutex effects_loaded;

Effect* create_effect(Clip* c, const EffectMeta* em) {
	if (!em->filename.isEmpty()) {
		// load effect from file
		return new Effect(c, em);
	} else if (em->internal >= 0 && em->internal < EFFECT_INTERNAL_COUNT) {
		// must be an internal effect
		switch (em->internal) {
		case EFFECT_INTERNAL_TRANSFORM: return new TransformEffect(c, em); break;
		case EFFECT_INTERNAL_TEXT: return new TextEffect(c, em); break;
		case EFFECT_INTERNAL_SOLID: return new SolidEffect(c, em); break;
		case EFFECT_INTERNAL_NOISE: return new AudioNoiseEffect(c, em); break;
		case EFFECT_INTERNAL_VOLUME: return new VolumeEffect(c, em); break;
		case EFFECT_INTERNAL_PAN: return new PanEffect(c, em); break;
		case EFFECT_INTERNAL_TONE: return new ToneEffect(c, em); break;
		case EFFECT_INTERNAL_SHAKE: return new ShakeEffect(c, em); break;
		}
	} else {
		dout << "[ERROR] Invalid effect data";
		QMessageBox::critical(mainWindow, "Invalid effect", "No candidate for effect '" + em->name + "'. This effect may be corrupt. Try reinstalling it or Olive.");
	}
	return NULL;
}

const EffectMeta* get_internal_meta(int internal_id) {
	for (int i=0;i<audio_effects.size();i++) {
		if (audio_effects.at(i).internal == internal_id) {
			return &audio_effects.at(i);
		}
	}
	for (int i=0;i<video_effects.size();i++) {
		if (video_effects.at(i).internal == internal_id) {
			return &video_effects.at(i);
		}
	}
    return NULL;
}

void load_internal_effects() {
	EffectMeta em;

	em.name = "Volume";
	em.internal = EFFECT_INTERNAL_VOLUME;
	audio_effects.append(em);

	em.name = "Pan";
	em.internal = EFFECT_INTERNAL_PAN;
	audio_effects.append(em);

	em.name = "Tone";
	em.internal = EFFECT_INTERNAL_TONE;
	audio_effects.append(em);

	em.name = "Noise";
	em.internal = EFFECT_INTERNAL_NOISE;
	audio_effects.append(em);

	em.name = "Transform";
	em.category = "Distort";
	em.internal = EFFECT_INTERNAL_TRANSFORM;
	video_effects.append(em);

	em.name = "Text";
	em.category = "Render";
	em.internal = EFFECT_INTERNAL_TEXT;
	video_effects.append(em);

	em.name = "Solid";
	em.category = "Render";
	em.internal = EFFECT_INTERNAL_SOLID;
	video_effects.append(em);

	em.name = "Shake";
	em.category = "Distort";
	em.internal = EFFECT_INTERNAL_SHAKE;
	video_effects.append(em);
}

void load_shader_effects() {
	QString effects_path = get_effects_dir();
	QDir effects_dir(effects_path);
	if (effects_dir.exists()) {
		QList<QString> entries = effects_dir.entryList(QStringList("*.xml"), QDir::Files);
		for (int i=0;i<entries.size();i++) {
			QFile file(effects_path + "/" + entries.at(i));
			if (!file.open(QIODevice::ReadOnly)) {
				dout << "[ERROR] Could not open" << entries.at(i);
				return;
			}

			QXmlStreamReader reader(&file);
			while (!reader.atEnd()) {
				if (reader.name() == "effect") {
					QString effect_name = "";
					QString effect_cat = "";
					const QXmlStreamAttributes attr = reader.attributes();
					for (int j=0;j<attr.size();j++) {
						if (attr.at(j).name() == "name") {
							effect_name = attr.at(j).value().toString();
						} else if (attr.at(j).name() == "category") {
							effect_cat = attr.at(j).value().toString();
						}
					}
					if (!effect_name.isEmpty()) {
						EffectMeta em;
						em.name = effect_name;
						em.category = effect_cat;
						em.filename = entries.at(i);
						em.internal = -1;
						video_effects.append(em);
					} else {
						dout << "[ERROR] Invalid effect found in" << entries.at(i);
					}
					break;
				}
				reader.readNext();
			}

			file.close();
		}
	}
}

void load_vst_effects() {

}

void init_effects() {
	EffectInit* init_thread = new EffectInit();
	QObject::connect(init_thread, SIGNAL(finished()), init_thread, SLOT(deleteLater()));
	init_thread->start();
}

EffectInit::EffectInit() {
	effects_loaded.lock();
}

void EffectInit::run() {
	dout << "[INFO] Initializing effects...";
	load_internal_effects();
	load_shader_effects();
	load_vst_effects();
	effects_loaded.unlock();
	dout << "[INFO] Finished initializing effects";
}

double double_lerp(double a, double b, double t) {
    return ((1.0 - t) * a) + (t * b);
}

Effect::Effect(Clip* c, const EffectMeta *em) :
	parent_clip(c),
    meta(em),
	enable_shader(false),
	enable_coords(false),
    enable_superimpose(false),
	isOpen(false),
	glslProgram(NULL),
	texture(NULL),
	bound(false)
{
    // set up base UI
    container = new CollapsibleWidget();    
    connect(container->enabled_check, SIGNAL(clicked(bool)), this, SLOT(field_changed()));
    ui = new QWidget();
    ui_layout = new QGridLayout();
	ui_layout->setSpacing(4);
    ui->setLayout(ui_layout);
	container->setContents(ui);

	connect(container->title_bar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));

    // set up UI from effect file
	container->setText(em->name);

	if (!em->filename.isEmpty()) {
		QFile effect_file(get_effects_dir() + "/" + em->filename);
		if (effect_file.open(QFile::ReadOnly)) {
			QXmlStreamReader reader(&effect_file);

			while (!reader.atEnd()) {
				if (reader.name() == "row" && reader.isStartElement()) {
					QString row_name;
					const QXmlStreamAttributes& attributes = reader.attributes();
					for (int i=0;i<attributes.size();i++) {
						const QXmlStreamAttribute& attr = attributes.at(i);
						if (attr.name() == "name") {
							row_name = attr.value().toString();
						}
					}
					if (!row_name.isEmpty()) {
						EffectRow* row = add_row(row_name + ":");
						while (!reader.atEnd() && !(reader.name() == "row" && reader.isEndElement())) {
							reader.readNext();
							if (reader.name() == "field" && reader.isStartElement()) {
								int type = EFFECT_TYPE_VIDEO;
								QString id;

								// get field type
								const QXmlStreamAttributes& attributes = reader.attributes();
								for (int i=0;i<attributes.size();i++) {
									const QXmlStreamAttribute& attr = attributes.at(i);
									if (attr.name() == "type") {
										QString comp = attr.value().toString().toUpper();
										if (comp == "DOUBLE") {
											type = EFFECT_FIELD_DOUBLE;
										} else if (comp == "BOOL") {
											type = EFFECT_FIELD_BOOL;
										} else if (comp == "COLOR") {
											type = EFFECT_FIELD_COLOR;
										} else if (comp == "COMBO") {
											type = EFFECT_FIELD_COMBO;
										} else if (comp == "FONT") {
											type = EFFECT_FIELD_FONT;
										} else if (comp == "STRING") {
											type = EFFECT_FIELD_STRING;
										}
									} else if (attr.name() == "id") {
										id = attr.value().toString();
									}
								}

								if (id.isEmpty()) {
									dout << "[ERROR] Couldn't load field from" << em->filename << "- ID cannot be empty.";
								} else if (type > -1) {
									EffectField* field = row->add_field(type);
									field->id = id;
									connect(field, SIGNAL(changed()), this, SLOT(field_changed()));
									switch (type) {
									case EFFECT_FIELD_DOUBLE:
										for (int i=0;i<attributes.size();i++) {
											const QXmlStreamAttribute& attr = attributes.at(i);
											if (attr.name() == "default") {
												field->set_double_default_value(attr.value().toDouble());
											} else if (attr.name() == "min") {
												field->set_double_minimum_value(attr.value().toDouble());
											} else if (attr.name() == "max") {
												field->set_double_maximum_value(attr.value().toDouble());
											}
										}
										break;
									case EFFECT_FIELD_COLOR:
									{
										QColor color;
										for (int i=0;i<attributes.size();i++) {
											const QXmlStreamAttribute& attr = attributes.at(i);
											if (attr.name() == "r") {
												color.setRed(attr.value().toInt());
											} else if (attr.name() == "g") {
												color.setGreen(attr.value().toInt());
											} else if (attr.name() == "b") {
												color.setBlue(attr.value().toInt());
											} else if (attr.name() == "rf") {
												color.setRedF(attr.value().toFloat());
											} else if (attr.name() == "gf") {
												color.setGreenF(attr.value().toFloat());
											} else if (attr.name() == "bf") {
												color.setBlueF(attr.value().toFloat());
											} else if (attr.name() == "hex") {
												color.setNamedColor(attr.value().toString());
											}
										}
										field->set_color_value(color);
									}
										break;
									case EFFECT_FIELD_STRING:
										for (int i=0;i<attributes.size();i++) {
											const QXmlStreamAttribute& attr = attributes.at(i);
											if (attr.name() == "default") {
												field->set_string_value(attr.value().toString());
											}
										}
										break;
									case EFFECT_FIELD_BOOL:
										for (int i=0;i<attributes.size();i++) {
											const QXmlStreamAttribute& attr = attributes.at(i);
											if (attr.name() == "default") {
												field->set_bool_value(attr.value() == "1");
											}
										}
										break;
									case EFFECT_FIELD_COMBO:
									{
										int combo_index = 0;
										for (int i=0;i<attributes.size();i++) {
											const QXmlStreamAttribute& attr = attributes.at(i);
											if (attr.name() == "default") {
												combo_index = attr.value().toInt();
												break;
											}
										}
										while (!reader.atEnd() && !(reader.name() == "field" && reader.isEndElement())) {
											reader.readNext();
											if (reader.name() == "option" && reader.isStartElement()) {
												reader.readNext();
												field->add_combo_item(reader.text().toString(), 0);
											}
										}
										field->set_combo_index(combo_index);
									}
										break;
									case EFFECT_FIELD_FONT:
										for (int i=0;i<attributes.size();i++) {
											const QXmlStreamAttribute& attr = attributes.at(i);
											if (attr.name() == "default") {
												field->set_font_name(attr.value().toString());
											}
										}
										break;
									}
								}
							}
						}
					}
				} else if (reader.name() == "shader" && reader.isStartElement()) {
					enable_shader = true;
					const QXmlStreamAttributes& attributes = reader.attributes();
					for (int i=0;i<attributes.size();i++) {
						const QXmlStreamAttribute& attr = attributes.at(i);
						if (attr.name() == "vert") {
							vertPath = attr.value().toString();
						} else if (attr.name() == "frag") {
							fragPath = attr.value().toString();
						}
					}
				} else if (reader.name() == "superimpose" && reader.isStartElement()) {
					enable_superimpose = true;
					const QXmlStreamAttributes& attributes = reader.attributes();
					for (int i=0;i<attributes.size();i++) {
						const QXmlStreamAttribute& attr = attributes.at(i);
						if (attr.name() == "script") {
							QFile script_file(get_effects_dir() + "/" + attr.value().toString());
							if (script_file.open(QFile::ReadOnly)) {
								script = script_file.readAll();
							} else {
								dout << "[ERROR] Failed to open superimpose script file for" << em->filename;
								enable_superimpose = false;
							}
							break;
						}
					}
				}
				reader.readNext();
			}

			effect_file.close();
		} else {
			dout << "[ERROR] Failed to open effect file" << em->filename;
		}
	}
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
		EffectRow* copy_row = e->rows.at(i);
		copy_row->setKeyframing(row->isKeyframing());
		copy_row->keyframe_times = row->keyframe_times;
		copy_row->keyframe_types = row->keyframe_types;
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			copy_row->field(j)->set_current_data(field->get_current_data());
			copy_row->field(j)->keyframe_data = field->keyframe_data;
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
	panel_sequence_viewer->viewer_widget->update();
}

void Effect::show_context_menu(const QPoint& pos) {
	QMenu menu(mainWindow);

	int index = get_index_in_clip();

	if (index > 0) {
		QAction* move_up = menu.addAction("Move &Up");
		connect(move_up, SIGNAL(triggered(bool)), this, SLOT(move_up()));
	}

	if (index < parent_clip->effects.size() - 1) {
		QAction* move_down = menu.addAction("Move &Down");
		connect(move_down, SIGNAL(triggered(bool)), this, SLOT(move_down()));
	}

	menu.addSeparator();

	QAction* del_action = menu.addAction("D&elete");
	connect(del_action, SIGNAL(triggered(bool)), this, SLOT(delete_self()));

	menu.exec(container->title_bar->mapToGlobal(pos));
}

void Effect::delete_self() {
	EffectDeleteCommand* command = new EffectDeleteCommand();
	command->clips.append(parent_clip);
	command->fx.append(get_index_in_clip());
	undo_stack.push(command);
	update_ui(true);
}

void Effect::move_up() {
	MoveEffectCommand* command = new MoveEffectCommand();
	command->clip = parent_clip;
	command->from = get_index_in_clip();
	command->to = command->from - 1;
	undo_stack.push(command);
	update_ui(true);
}

void Effect::move_down() {
	MoveEffectCommand* command = new MoveEffectCommand();
	command->clip = parent_clip;
	command->from = get_index_in_clip();
	command->to = command->from + 1;
	undo_stack.push(command);
	update_ui(true);
}

int Effect::get_index_in_clip() {
	if (parent_clip != NULL) {
		for (int i=0;i<parent_clip->effects.size();i++) {
			if (parent_clip->effects.at(i) == this) {
				return i;
			}
		}
	}
	return -1;
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
							// match field using ID
							bool found_field_by_id = false;
							int field_number = field_count;
							for (int k=0;k<stream.attributes().size();k++) {
								const QXmlStreamAttribute& attr = stream.attributes().at(k);
								if (attr.name() == "id") {
									for (int l=0;l<row->fieldCount();l++) {
										if (row->field(l)->id == attr.value()) {
											field_number = l;
											found_field_by_id = true;
											dout << "[INFO] Found field by ID";
											break;
										}
									}
									break;
								}
							}

							// TODO DEPRECATED, only used for backwards compatibility with 180820
							if (!found_field_by_id) dout << "[INFO] Found field by field number";

							EffectField* field = row->field(field_number);

							// get current field value
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
							dout << "[ERROR] Too many fields for effect" << id << "row" << row_count << ". Project might be corrupt. (Got" << field_count << ", expected <" << row->fieldCount()-1 << ")";
						}
						field_count++;
					}
				}

			} else {
				dout << "[ERROR] Too many rows for effect" << id << ". Project might be corrupt. (Got" << row_count << ", expected <" << rows.size()-1 << ")";
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
			stream.writeAttribute("id", field->id);
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
		dout << "[WARNING] Tried to open an effect that was already open";
		close();
	}
	if (enable_shader) {
		if (QOpenGLContext::currentContext() == NULL) {
			dout << "[WARNING] No current context to create a shader program for - will retry next repaint";
		} else {
			glslProgram = new QOpenGLShaderProgram();
			if (!vertPath.isEmpty()) glslProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, get_effects_dir() + "/" + vertPath);
			if (!fragPath.isEmpty()) glslProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, get_effects_dir() + "/" + fragPath);
			glslProgram->link();
			isOpen = true;
		}
	} else {
		isOpen = true;
	}

	if (enable_superimpose) {
		texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
	}
}

void Effect::close() {
	if (!isOpen) {
		dout << "[WARNING] Tried to close an effect that was already closed";
	}
	if (glslProgram != NULL) {
		delete glslProgram;
		glslProgram = NULL;
	}
	delete_texture();
	isOpen = false;
}

void Effect::startEffect() {
	if (!isOpen) {
		open();
		dout << "[WARNING] Tried to start a closed effect - opening";
	}
	if (enable_shader) bound = glslProgram->bind();
}

void Effect::endEffect() {
	if (bound) glslProgram->release();
	bound = false;
}

Effect* Effect::copy(Clip* c) {
	Effect* copy = create_effect(c, meta);
	copy->set_enabled(is_enabled());
    copy_field_keyframes(copy);
    return copy;
}

void Effect::process_shader(double timecode) {
	glslProgram->setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());

	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			if (!field->id.isEmpty()) {
				switch (field->type) {
				case EFFECT_FIELD_DOUBLE:
					glslProgram->setUniformValue(field->id.toLatin1().constData(), (GLfloat) field->get_double_value(timecode));
					break;
				case EFFECT_FIELD_COLOR:
					glslProgram->setUniformValue(field->id.toLatin1().constData(), field->get_color_value(timecode));
					break;
				case EFFECT_FIELD_STRING: break; // can you even send a string to a uniform value?
				case EFFECT_FIELD_BOOL:
					glslProgram->setUniformValue(field->id.toLatin1().constData(), field->get_bool_value(timecode));
					break;
				case EFFECT_FIELD_COMBO:
					glslProgram->setUniformValue(field->id.toLatin1().constData(), field->get_combo_index(timecode));
					break;
				case EFFECT_FIELD_FONT: break; // can you even send a string to a uniform value?
				}
			}
		}
	}
}

void Effect::process_coords(double, GLTextureCoords&) {}

GLuint Effect::process_superimpose(double timecode) {
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
			delete_texture();
			texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
			texture->setData(img);
		} else {
			texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, img.constBits());
		}
		return texture->textureId();
	}
	return 0;
}

void Effect::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count) {
	// only volume/pan, hand off to AU and VST for all other cases

    /*double interval = (timecode_end-timecode_start)/nb_bytes;

	for (int i=0;i<nb_bytes;i+=2) {
		qint32 samp = (qint16) (((samples[i+1] & 0xFF) << 8) | (samples[i] & 0xFF));

        jsEngine.globalObject().setProperty("sample", samp);
		jsEngine.globalObject().setProperty("volume", row(0)->field(0)->get_double_value(timecode_start+(interval*i), true));
		QJSValue result = eval.call();
        samp = result.toInt();
        QJSValueList args;
        args << samples << nb_bytes;


		samples[i+1] = (quint8) (samp >> 8);
		samples[i] = (quint8) samp;
    }*/
}

void Effect::redraw(double timecode) {
	/*
	// run javascript
	QPainter p(&img);
	painter_wrapper.img = &img;
	painter_wrapper.painter = &p;

	jsEngine.globalObject().setProperty("painter", wrapper_obj);
	jsEngine.globalObject().setProperty("width", parent_clip->getWidth());
	jsEngine.globalObject().setProperty("height", parent_clip->getHeight());

	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			if (!field->id.isEmpty()) {
				switch (field->type) {
				case EFFECT_FIELD_DOUBLE:
					jsEngine.globalObject().setProperty(field->id, field->get_double_value(timecode));
					break;
				case EFFECT_FIELD_COLOR:
					jsEngine.globalObject().setProperty(field->id, field->get_color_value(timecode).name());
					break;
				case EFFECT_FIELD_STRING:
					jsEngine.globalObject().setProperty(field->id, field->get_string_value(timecode));
					break;
				case EFFECT_FIELD_BOOL:
					jsEngine.globalObject().setProperty(field->id, field->get_bool_value(timecode));
					break;
				case EFFECT_FIELD_COMBO:
					jsEngine.globalObject().setProperty(field->id, field->get_combo_index(timecode));
					break;
				case EFFECT_FIELD_FONT:
					jsEngine.globalObject().setProperty(field->id, field->get_font_name(timecode));
					break;
				}
			}
		}
	}

	jsEngine.evaluate(script);
	*/
}

bool Effect::valueHasChanged(double timecode) {
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

void Effect::delete_texture() {
	if (texture != NULL) {
		texture->destroy();
		delete texture;
		texture = NULL;
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

	QSize button_size(20, 20);
	QSize icon_size(12, 12);

	QHBoxLayout* key_controls = new QHBoxLayout();
	key_controls->setSpacing(0);
	key_controls->setMargin(0);
	key_controls->addStretch();

	left_key_nav = new QPushButton("<");
	left_key_nav->setVisible(false);
	left_key_nav->setMaximumSize(button_size);
	key_controls->addWidget(left_key_nav);
	connect(left_key_nav, SIGNAL(clicked(bool)), this, SLOT(goto_previous_key()));

	key_addremove = new QPushButton(".");
	key_addremove->setVisible(false);
	key_addremove->setMaximumSize(button_size);
	key_controls->addWidget(key_addremove);
	connect(key_addremove, SIGNAL(clicked(bool)), this, SLOT(toggle_key()));

	right_key_nav = new QPushButton(">");
	right_key_nav->setVisible(false);
	right_key_nav->setMaximumSize(button_size);
	key_controls->addWidget(right_key_nav);
	connect(right_key_nav, SIGNAL(clicked(bool)), this, SLOT(goto_next_key()));

	keyframe_enable = new QPushButton(QIcon(":/icons/clock.png"), "");
	keyframe_enable->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
	keyframe_enable->setMaximumSize(button_size);
	keyframe_enable->setIconSize(icon_size);
	keyframe_enable->setCheckable(true);
	keyframe_enable->setToolTip("Enable Keyframes");
	connect(keyframe_enable, SIGNAL(clicked(bool)), this, SLOT(set_keyframe_enabled(bool)));
	connect(keyframe_enable, SIGNAL(toggled(bool)), this, SLOT(keyframe_ui_enabled(bool)));
	key_controls->addWidget(keyframe_enable);

	ui->addLayout(key_controls, row, 6);
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

void EffectRow::keyframe_ui_enabled(bool enabled) {
	left_key_nav->setVisible(enabled);
	key_addremove->setVisible(enabled);
	right_key_nav->setVisible(enabled);
}

void EffectRow::goto_previous_key() {
	long key = LONG_MIN;
	Clip* c = parent_effect->parent_clip;
	for (int i=0;i<keyframe_times.size();i++) {
		long comp = keyframe_times.at(i) - c->clip_in + c->timeline_in;
		if (comp < sequence->playhead) {
			key = qMax(comp, key);
		}
	}
	if (key != LONG_MIN) panel_sequence_viewer->seek(key);
}

void EffectRow::toggle_key() {
	int index = -1;
	Clip* c = parent_effect->parent_clip;
	for (int i=0;i<keyframe_times.size();i++) {
		long comp = c->timeline_in - c->clip_in + keyframe_times.at(i);
		if (comp == sequence->playhead) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		// keyframe doesn't exist, set one
		set_keyframe_now(true);
	} else {
		KeyframeDelete* kd = new KeyframeDelete();
		delete_keyframe(kd, index);
		undo_stack.push(kd);
		panel_effect_controls->update_keyframes();
		panel_sequence_viewer->viewer_widget->update();
	}
}

void EffectRow::goto_next_key() {
	long key = LONG_MAX;
	Clip* c = parent_effect->parent_clip;
	for (int i=0;i<keyframe_times.size();i++) {
		long comp = c->timeline_in - c->clip_in + keyframe_times.at(i);
		if (comp > sequence->playhead) {
			key = qMin(comp, key);
		}
	}
	if (key != LONG_MAX) panel_sequence_viewer->seek(key);
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
	long time = sequence->playhead-parent_effect->parent_clip->timeline_in+parent_effect->parent_clip->clip_in;
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

qint16 mix_audio_sample(qint16 a, qint16 b) {
	qint32 mixed_sample = static_cast<qint32>(a) + static_cast<qint32>(b);
	mixed_sample = qMax(qMin(mixed_sample, static_cast<qint32>(INT16_MAX)), static_cast<qint32>(INT16_MIN));
	return static_cast<qint16>(mixed_sample);
}

double log_volume(double linear) {
	// expects a value between 0 and 1 (or more if amplifying)
	return (qExp(linear)-1)/(M_E-1);
}
