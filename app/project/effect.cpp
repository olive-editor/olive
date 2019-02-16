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

#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "panels/project.h"
#include "project/undo.h"
#include "project/sequence.h"
#include "project/clip.h"
#include "panels/timeline.h"
#include "panels/effectcontrols.h"
#include "panels/grapheditor.h"
#include "ui/checkboxex.h"
#include "debug.h"
#include "io/path.h"
#include "ui/mainwindow.h"
#include "io/math.h"
#include "io/clipboard.h"
#include "io/config.h"
#include "transition.h"

#include "effects/internal/transformeffect.h"
#include "effects/internal/texteffect.h"
#include "effects/internal/timecodeeffect.h"
#include "effects/internal/solideffect.h"
#include "effects/internal/audionoiseeffect.h"
#include "effects/internal/toneeffect.h"
#include "effects/internal/volumeeffect.h"
#include "effects/internal/paneffect.h"
#include "effects/internal/shakeeffect.h"
#include "effects/internal/cornerpineffect.h"
#include "effects/internal/vsthost.h"
#include "effects/internal/fillleftrighteffect.h"
#include "effects/internal/frei0reffect.h"

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
#include <QApplication>
#include <QFileDialog>

QVector<EffectMeta> effects;

Effect* create_effect(Clip* c, const EffectMeta* em) {
	if (em->internal >= 0 && em->internal < EFFECT_INTERNAL_COUNT) {
		// must be an internal effect
		switch (em->internal) {
		case EFFECT_INTERNAL_TRANSFORM: return new TransformEffect(c, em);
		case EFFECT_INTERNAL_TEXT: return new TextEffect(c, em);
		case EFFECT_INTERNAL_TIMECODE: return new TimecodeEffect(c, em);
		case EFFECT_INTERNAL_SOLID: return new SolidEffect(c, em);
		case EFFECT_INTERNAL_NOISE: return new AudioNoiseEffect(c, em);
		case EFFECT_INTERNAL_VOLUME: return new VolumeEffect(c, em);
		case EFFECT_INTERNAL_PAN: return new PanEffect(c, em);
		case EFFECT_INTERNAL_TONE: return new ToneEffect(c, em);
		case EFFECT_INTERNAL_SHAKE: return new ShakeEffect(c, em);
		case EFFECT_INTERNAL_CORNERPIN: return new CornerPinEffect(c, em);
		case EFFECT_INTERNAL_FILLLEFTRIGHT: return new FillLeftRightEffect(c, em);
#ifndef NOVST
		case EFFECT_INTERNAL_VST: return new VSTHost(c, em);
#endif
#ifndef NOFREI0R
		case EFFECT_INTERNAL_FREI0R: return new Frei0rEffect(c, em);
#endif
		}
	} else if (!em->filename.isEmpty()) {
		// load effect from file
		return new Effect(c, em);
	} else {
		qCritical() << "Invalid effect data";
        QMessageBox::critical(Olive::MainWindow,
							  QCoreApplication::translate("Effect", "Invalid effect"),
							  QCoreApplication::translate("Effect", "No candidate for effect '%1'. This effect may be corrupt. Try reinstalling it or Olive.").arg(em->name));
	}
	return nullptr;
}

const EffectMeta* get_internal_meta(int internal_id, int type) {
	for (int i=0;i<effects.size();i++) {
		if (effects.at(i).internal == internal_id && effects.at(i).type == type) {
			return &effects.at(i);
		}
	}
	return nullptr;
}

Effect::Effect(Clip* c, const EffectMeta *em) :
	parent_clip(c),
	meta(em),
	enable_shader(false),
	enable_coords(false),
	enable_superimpose(false),
	enable_image(false),
	glslProgram(nullptr),
	texture(nullptr),
	enable_always_update(false),
	isOpen(false),
	bound(false),
	iterations(1)
{
	// set up base UI
	container = new CollapsibleWidget();
	connect(container->enabled_check, SIGNAL(clicked(bool)), this, SLOT(field_changed()));
	ui = new QWidget(container);
	ui_layout = new QGridLayout(ui);
	ui_layout->setSpacing(4);
	container->setContents(ui);

	connect(container->title_bar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu(const QPoint&)));

	if (em != nullptr) {
		// set up UI from effect file
		container->setText(em->name);

		if (!em->filename.isEmpty() && em->internal == -1) {
			QFile effect_file(em->filename);
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
							EffectRow* row = add_row(row_name);
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
											} else if (comp == "FILE") {
												type = EFFECT_FIELD_FILE;
											}
										} else if (attr.name() == "id") {
											id = attr.value().toString();
										}
									}

									if (id.isEmpty()) {
										qCritical() << "Couldn't load field from" << em->filename << "- ID cannot be empty.";
									} else if (type > -1) {
										EffectField* field = row->add_field(type, id);
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
													color.setRedF(attr.value().toDouble());
												} else if (attr.name() == "gf") {
													color.setGreenF(attr.value().toDouble());
												} else if (attr.name() == "bf") {
													color.setBlueF(attr.value().toDouble());
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
										case EFFECT_FIELD_FILE:
											for (int i=0;i<attributes.size();i++) {
												const QXmlStreamAttribute& attr = attributes.at(i);
												if (attr.name() == "filename") {
													field->set_filename(attr.value().toString());
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
							} else if (attr.name() == "iterations") {
								setIterations(attr.value().toInt());
							}
						}
					}/* else if (reader.name() == "superimpose" && reader.isStartElement()) {
						enable_superimpose = true;
						const QXmlStreamAttributes& attributes = reader.attributes();
						for (int i=0;i<attributes.size();i++) {
							const QXmlStreamAttribute& attr = attributes.at(i);
							if (attr.name() == "script") {
								QFile script_file(get_effects_dir() + "/" + attr.value().toString());
								if (script_file.open(QFile::ReadOnly)) {
									script = script_file.readAll();
								} else {
									qCritical() << "Failed to open superimpose script file for" << em->filename;
									enable_superimpose = false;
								}
								break;
							}
						}
					}*/
					reader.readNext();
				}

				effect_file.close();
			} else {
				qCritical() << "Failed to open effect file" << em->filename;
			}
		}
	}
}

Effect::~Effect() {
	if (isOpen) {
		close();
	}

	delete container;

	for (int i=0;i<rows.size();i++) {
		delete rows.at(i);
	}
	for (int i=0;i<gizmos.size();i++) {
		delete gizmos.at(i);
	}
}

void Effect::copy_field_keyframes(Effect* e) {
	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		EffectRow* copy_row = e->rows.at(i);
		copy_row->setKeyframing(row->isKeyframing());
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			EffectField* copy_field = copy_row->field(j);
			copy_field->keyframes = field->keyframes;
			copy_field->set_current_data(field->get_current_data());
		}
	}
}

EffectRow* Effect::add_row(const QString& name, bool savable, bool keyframable) {
	EffectRow* row = new EffectRow(this, savable, ui_layout, name, rows.size(), keyframable);
	rows.append(row);
	return row;
}

EffectRow* Effect::row(int i) {
	return rows.at(i);
}

int Effect::row_count() {
	return rows.size();
}

EffectGizmo *Effect::add_gizmo(int type) {
	EffectGizmo* gizmo = new EffectGizmo(type);
	gizmos.append(gizmo);
	return gizmo;
}

EffectGizmo *Effect::gizmo(int i) {
	return gizmos.at(i);
}

int Effect::gizmo_count() {
	return gizmos.size();
}

void Effect::refresh() {}

void Effect::field_changed() {
	panel_sequence_viewer->viewer_widget->frame_update();
	panel_graph_editor->update_panel();
}

void Effect::show_context_menu(const QPoint& pos) {
	if (meta->type == EFFECT_TYPE_EFFECT) {
        QMenu menu(Olive::MainWindow);

		int index = get_index_in_clip();

		menu.addAction(tr("Cu&t"), panel_effect_controls, SLOT(cut()));
		menu.addAction(tr("&Copy"), panel_effect_controls, SLOT(copy(bool)));

		panel_effect_controls->add_effect_paste_action(&menu);

		menu.addSeparator();

		if (index > 0) {
			menu.addAction(tr("Move &Up"), this, SLOT(move_up()));
		}

		if (index < parent_clip->effects.size() - 1) {
			menu.addAction(tr("Move &Down"), this, SLOT(move_down()));
		}

		menu.addSeparator();

		menu.addAction(tr("D&elete"), this, SLOT(delete_self()));

		menu.addSeparator();

		menu.addAction(tr("Load Settings From File"), this, SLOT(load_from_file()));

		menu.addAction(tr("Save Settings to File"), this, SLOT(save_to_file()));

		menu.exec(container->title_bar->mapToGlobal(pos));
	}
}

void Effect::delete_self() {
	EffectDeleteCommand* command = new EffectDeleteCommand();
	command->clips.append(parent_clip);
	command->fx.append(get_index_in_clip());
	Olive::UndoStack.push(command);
	update_ui(true);
}

void Effect::move_up() {
	MoveEffectCommand* command = new MoveEffectCommand();
	command->clip = parent_clip;
	command->from = get_index_in_clip();
	command->to = command->from - 1;
	Olive::UndoStack.push(command);
	panel_effect_controls->reload_clips();
	panel_sequence_viewer->viewer_widget->frame_update();
}

void Effect::move_down() {
	MoveEffectCommand* command = new MoveEffectCommand();
	command->clip = parent_clip;
	command->from = get_index_in_clip();
	command->to = command->from + 1;
	Olive::UndoStack.push(command);
	panel_effect_controls->reload_clips();
	panel_sequence_viewer->viewer_widget->frame_update();
}

void Effect::save_to_file() {
	// save effect settings to file
    QString file = QFileDialog::getSaveFileName(Olive::MainWindow,
												tr("Save Effect Settings"),
												QString(),
												tr("Effect XML Settings %1").arg("(*.xml)"));

	// if the user picked a file
	if (!file.isEmpty()) {

        // ensure file ends with .xml extension
        if (!file.endsWith(".xml", Qt::CaseInsensitive)) {
            file.append(".xml");
        }

		QFile file_handle(file);
		if (file_handle.open(QFile::WriteOnly)) {

			file_handle.write(save_to_string());

			file_handle.close();
		} else {
            QMessageBox::critical(Olive::MainWindow,
								  tr("Save Settings Failed"),
								  tr("Failed to open \"%1\" for writing.").arg(file),
								  QMessageBox::Ok);
		}
	}
}

void Effect::load_from_file() {
	// load effect settings from file
    QString file = QFileDialog::getOpenFileName(Olive::MainWindow,
												tr("Load Effect Settings"),
												QString(),
												tr("Effect XML Settings %1").arg("(*.xml)"));

	// if the user picked a file
	if (!file.isEmpty()) {
		QFile file_handle(file);
		if (file_handle.open(QFile::ReadOnly)) {

			Olive::UndoStack.push(new SetEffectData(this, file_handle.readAll()));

			file_handle.close();

			update_ui(false);
		} else {
            QMessageBox::critical(Olive::MainWindow,
								  tr("Load Settings Failed"),
								  tr("Failed to open \"%1\" for reading.").arg(file),
								  QMessageBox::Ok);
		}
	}
}

int Effect::get_index_in_clip() {
	if (parent_clip != nullptr) {
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
	case EFFECT_FIELD_DOUBLE: return string.toDouble();
	case EFFECT_FIELD_COLOR: return QColor(string);
	case EFFECT_FIELD_BOOL: return (string == "1");
	case EFFECT_FIELD_COMBO: return string.toInt();
	case EFFECT_FIELD_STRING:
	case EFFECT_FIELD_FONT:
	case EFFECT_FIELD_FILE:
		 return string;
	}
	return QVariant();
}

QString save_data_to_string(int type, const QVariant& data) {
	switch (type) {
	case EFFECT_FIELD_DOUBLE: return QString::number(data.toDouble());
	case EFFECT_FIELD_COLOR: return data.value<QColor>().name();
	case EFFECT_FIELD_BOOL: return QString::number(data.toBool());
	case EFFECT_FIELD_COMBO: return QString::number(data.toInt());
	case EFFECT_FIELD_STRING:
	case EFFECT_FIELD_FONT:
	case EFFECT_FIELD_FILE:
		return data.toString();
	}
	return QString();
}

void Effect::load(QXmlStreamReader& stream) {
	int row_count = 0;

	QString tag = stream.name().toString();

	while (!stream.atEnd() && !(stream.name() == tag && stream.isEndElement())) {
		stream.readNext();
		if (stream.name() == "row" && stream.isStartElement()) {
			if (row_count < rows.size()) {
				EffectRow* row = rows.at(row_count);
				int field_count = 0;

				while (!stream.atEnd() && !(stream.name() == "row" && stream.isEndElement())) {
					stream.readNext();

					// read field
					if (stream.name() == "field" && stream.isStartElement()) {
						if (field_count < row->fieldCount()) {
							// match field using ID
							int field_number = field_count;
							for (int k=0;k<stream.attributes().size();k++) {
								const QXmlStreamAttribute& attr = stream.attributes().at(k);
								if (attr.name() == "id") {
									for (int l=0;l<row->fieldCount();l++) {
										if (row->field(l)->id == attr.value()) {
											field_number = l;
//											qInfo() << "Found field by ID";
											break;
										}
									}
									break;
								}
							}

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

								// read keyframes
								if (stream.name() == "key" && stream.isStartElement()) {
									row->setKeyframing(true);

									EffectKeyframe key;
									for (int k=0;k<stream.attributes().size();k++) {
										const QXmlStreamAttribute& attr = stream.attributes().at(k);
										if (attr.name() == "value") {
											key.data = load_data_from_string(field->type, attr.value().toString());
										} else if (attr.name() == "frame") {
											key.time = attr.value().toLong();
										} else if (attr.name() == "type") {
											key.type = attr.value().toInt();
										} else if (attr.name() == "prehx") {
											key.pre_handle_x = attr.value().toDouble();
										} else if (attr.name() == "prehy") {
											key.pre_handle_y = attr.value().toDouble();
										} else if (attr.name() == "posthx") {
											key.post_handle_x = attr.value().toDouble();
										} else if (attr.name() == "posthy") {
											key.post_handle_y = attr.value().toDouble();
										}
									}
									field->keyframes.append(key);
								}
							}
						} else {
							qCritical() << "Too many fields for effect" << id << "row" << row_count << ". Project might be corrupt. (Got" << field_count << ", expected <" << row->fieldCount()-1 << ")";
						}
						field_count++;
					}
				}

			} else {
				qCritical() << "Too many rows for effect" << id << ". Project might be corrupt. (Got" << row_count << ", expected <" << rows.size()-1 << ")";
			}
			row_count++;
		} else if (stream.isStartElement()) {
			custom_load(stream);
		}
	}
}

void Effect::custom_load(QXmlStreamReader &) {}

void Effect::save(QXmlStreamWriter& stream) {
	stream.writeAttribute("name", meta->category + "/" + meta->name);
	stream.writeAttribute("enabled", QString::number(is_enabled()));

	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		if (row->savable) {
			stream.writeStartElement("row"); // row
			for (int j=0;j<row->fieldCount();j++) {
				EffectField* field = row->field(j);
				stream.writeStartElement("field"); // field
				stream.writeAttribute("id", field->id);
				stream.writeAttribute("value", save_data_to_string(field->type, field->get_current_data()));
				for (int k=0;k<field->keyframes.size();k++) {
					const EffectKeyframe& key = field->keyframes.at(k);
					stream.writeStartElement("key");
					stream.writeAttribute("value", save_data_to_string(field->type, key.data));
					stream.writeAttribute("frame", QString::number(key.time));
					stream.writeAttribute("type", QString::number(key.type));
					stream.writeAttribute("prehx", QString::number(key.pre_handle_x));
					stream.writeAttribute("prehy", QString::number(key.pre_handle_y));
					stream.writeAttribute("posthx", QString::number(key.post_handle_x));
					stream.writeAttribute("posthy", QString::number(key.post_handle_y));
					stream.writeEndElement(); // key
				}
				stream.writeEndElement(); // field
			}
			stream.writeEndElement(); // row
		}
	}
}

void Effect::load_from_string(const QByteArray &s) {
	// clear existing keyframe data
	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		row->setKeyframing(false);
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			field->keyframes.clear();
		}
	}

	// write settings with xml writer
	QXmlStreamReader stream(s);

	while (!stream.atEnd()) {
		stream.readNext();

		// find the effect opening tag
		if (stream.name() == "effect" && stream.isStartElement()) {

			// check the name to see if it matches this effect
			const QXmlStreamAttributes& attributes = stream.attributes();
			for (int i=0;i<attributes.size();i++) {
				const QXmlStreamAttribute& attr = attributes.at(i);
				if (attr.name() == "name") {
					if (get_meta_from_name(attr.value().toString()) == meta) {
						// pass off to standard loading function
						load(stream);
					} else {
                        QMessageBox::critical(Olive::MainWindow,
											  tr("Load Settings Failed"),
											  tr("This settings file doesn't match this effect."),
											  QMessageBox::Ok);
					}
					break;
				}
			}

			// we've found what we're looking for
			break;
		}
	}
}

QByteArray Effect::save_to_string() {
	QByteArray save_data;

	// write settings to string with xml writer
	QXmlStreamWriter stream(&save_data);

	stream.writeStartDocument();

	stream.writeStartElement("effect");

	// pass off to standard saving function
	save(stream);

	stream.writeEndElement(); // effect

	stream.writeEndDocument();

	return save_data;
}

bool Effect::is_open() {
	return isOpen;
}

void Effect::validate_meta_path() {
	if (!meta->path.isEmpty() || (vertPath.isEmpty() && fragPath.isEmpty())) return;
	QList<QString> effects_paths = get_effects_paths();
	const QString& test_fn = vertPath.isEmpty() ? fragPath : vertPath;
	for (int i=0;i<effects_paths.size();i++) {
		if (QFileInfo::exists(effects_paths.at(i) + "/" + test_fn)) {
			for (int j=0;j<effects.size();j++) {
				if (&effects.at(j) == meta) {
					effects[j].path = effects_paths.at(i);
					return;
				}
			}
			return;
		}
	}
}

void Effect::open() {
	if (isOpen) {
		qWarning() << "Tried to open an effect that was already open";
		close();
	}
	if (runtime_config.shaders_are_enabled && enable_shader) {
		if (QOpenGLContext::currentContext() == nullptr) {
			qWarning() << "No current context to create a shader program for - will retry next repaint";
		} else {
			glslProgram = new QOpenGLShaderProgram();
			validate_meta_path();
			bool glsl_compiled = true;
			if (!vertPath.isEmpty()) {
				if (glslProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, meta->path + "/" + vertPath)) {
					qInfo() << "Vertex shader added successfully";
				} else {
					glsl_compiled = false;
					qWarning() << "Vertex shader could not be added";
				}
			}
			if (!fragPath.isEmpty()) {
				if (glslProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, meta->path + "/" + fragPath)) {
					qInfo() << "Fragment shader added successfully";
				} else {
					glsl_compiled = false;
					qWarning() << "Fragment shader could not be added";
				}
			}
			if (glsl_compiled) {
				if (glslProgram->link()) {
					qInfo() << "Shader program linked successfully";
				} else {
					qWarning() << "Shader program failed to link";
				}
			}
			isOpen = true;
		}
	} else {
		isOpen = true;
	}
}

void Effect::close() {
	if (!isOpen) {
		qWarning() << "Tried to close an effect that was already closed";
	}
	delete_texture();
	if (glslProgram != nullptr) {
		delete glslProgram;
		glslProgram = nullptr;
	}
	isOpen = false;
}

bool Effect::is_glsl_linked() {
	return glslProgram != nullptr && glslProgram->isLinked();
}

void Effect::startEffect() {
	if (!isOpen) {
		open();
		qWarning() << "Tried to start a closed effect - opening";
	}
	if (runtime_config.shaders_are_enabled
			&& enable_shader
			&& glslProgram->isLinked()) {
		bound = glslProgram->bind();
	}
}

void Effect::endEffect() {
	if (bound) glslProgram->release();
	bound = false;
}

int Effect::getIterations() {
	return iterations;
}

void Effect::setIterations(int i) {
	iterations = i;
}

void Effect::process_image(double, uint8_t *, uint8_t *, int){}

Effect* Effect::copy(Clip* c) {
	Effect* copy = create_effect(c, meta);
	copy->set_enabled(is_enabled());
	copy_field_keyframes(copy);
	return copy;
}

void Effect::process_shader(double timecode, GLTextureCoords&, int iteration) {
	glslProgram->setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());
	glslProgram->setUniformValue("time", GLfloat(timecode));
	glslProgram->setUniformValue("iteration", iteration);

	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			if (!field->id.isEmpty()) {
				switch (field->type) {
				case EFFECT_FIELD_DOUBLE:
					glslProgram->setUniformValue(field->id.toUtf8().constData(), GLfloat(field->get_double_value(timecode)));
					break;
				case EFFECT_FIELD_COLOR:
					glslProgram->setUniformValue(
								field->id.toUtf8().constData(),
								GLfloat(field->get_color_value(timecode).redF()),
								GLfloat(field->get_color_value(timecode).greenF()),
								GLfloat(field->get_color_value(timecode).blueF())
							);
					break;
				case EFFECT_FIELD_STRING: break; // can you even send a string to a uniform value?
				case EFFECT_FIELD_BOOL:
					glslProgram->setUniformValue(field->id.toUtf8().constData(), field->get_bool_value(timecode));
					break;
				case EFFECT_FIELD_COMBO:
					glslProgram->setUniformValue(field->id.toUtf8().constData(), field->get_combo_index(timecode));
					break;
				case EFFECT_FIELD_FONT: break; // can you even send a string to a uniform value?
				case EFFECT_FIELD_FILE: break; // can you even send a string to a uniform value?
				}
			}
		}
	}
}

void Effect::process_coords(double, GLTextureCoords&, int) {}

GLuint Effect::process_superimpose(double timecode) {
	bool dimensions_changed = false;
	bool redrew_image = false;

	int width = parent_clip->getWidth();
	int height = parent_clip->getHeight();

	if (width != img.width() || height != img.height()) {
		img = QImage(width, height, QImage::Format_RGBA8888_Premultiplied);
		dimensions_changed = true;
	}

	if (valueHasChanged(timecode) || dimensions_changed || enable_always_update) {
		redraw(timecode);
		redrew_image = true;
	}

	if (texture == nullptr || texture->width() != img.width() || texture->height() != img.height()) {
		delete_texture();

		texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
		texture->setSize(img.width(), img.height());
		texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
		texture->setMipLevels(texture->maximumMipLevels());
		texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
		texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

		redrew_image = true;
	}

	if (redrew_image) {
		texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, img.constBits());
	}

	return texture->textureId();
}

void Effect::process_audio(double, double, quint8*, int, int) {}

void Effect::gizmo_draw(double, GLTextureCoords &) {}

void Effect::gizmo_move(EffectGizmo* gizmo, int x_movement, int y_movement, double timecode, bool done) {
	for (int i=0;i<gizmos.size();i++) {
		if (gizmos.at(i) == gizmo) {
			ComboAction* ca = nullptr;
			if (done) ca = new ComboAction();
			if (gizmo->x_field1 != nullptr) {
				gizmo->x_field1->set_double_value(gizmo->x_field1->get_double_value(timecode) + x_movement*gizmo->x_field_multi1);
				gizmo->x_field1->make_key_from_change(ca);
			}
			if (gizmo->y_field1 != nullptr) {
				gizmo->y_field1->set_double_value(gizmo->y_field1->get_double_value(timecode) + y_movement*gizmo->y_field_multi1);
				gizmo->y_field1->make_key_from_change(ca);
			}
			if (gizmo->x_field2 != nullptr) {
				gizmo->x_field2->set_double_value(gizmo->x_field2->get_double_value(timecode) + x_movement*gizmo->x_field_multi2);
				gizmo->x_field2->make_key_from_change(ca);
			}
			if (gizmo->y_field2 != nullptr) {
				gizmo->y_field2->set_double_value(gizmo->y_field2->get_double_value(timecode) + y_movement*gizmo->y_field_multi2);
				gizmo->y_field2->make_key_from_change(ca);
			}
			if (done) Olive::UndoStack.push(ca);
			break;
		}
	}
}

void Effect::gizmo_world_to_screen() {
	GLfloat view_val[16];
	GLfloat projection_val[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, view_val);
	glGetFloatv(GL_PROJECTION_MATRIX, projection_val);

	QMatrix4x4 view_matrix(view_val);
	QMatrix4x4 projection_matrix(projection_val);

	for (int i=0;i<gizmos.size();i++) {
		EffectGizmo* g = gizmos.at(i);

		for (int j=0;j<g->get_point_count();j++) {
			QVector4D screen_pos = QVector4D(g->world_pos[j].x(), g->world_pos[j].y(), 0, 1.0) * (view_matrix * projection_matrix);

			int adjusted_sx1 = qRound(((screen_pos.x()*0.5f)+0.5f)*parent_clip->sequence->width);
			int adjusted_sy1 = qRound((1.0f-((screen_pos.y()*0.5f)+0.5f))*parent_clip->sequence->height);

			g->screen_pos[j] = QPoint(adjusted_sx1, adjusted_sy1);
		}
	}
}

bool Effect::are_gizmos_enabled() {
	return (gizmos.size() > 0);
}

void Effect::redraw(double) {
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
	if (texture != nullptr) {
		delete texture;
		texture = nullptr;
	}
}

const EffectMeta* get_meta_from_name(const QString& input) {
	int split_index = input.indexOf('/');
	QString category;
	if (split_index > -1) {
		category = input.left(split_index);
	}
	QString name = input.mid(split_index + 1);

	for (int j=0;j<effects.size();j++) {
		if (effects.at(j).name == name
				&& (effects.at(j).category == category
					|| category.isEmpty())) {
			return &effects.at(j);
		}
	}
	return nullptr;
}

qint16 mix_audio_sample(qint16 a, qint16 b) {
	qint32 mixed_sample = static_cast<qint32>(a) + static_cast<qint32>(b);
	mixed_sample = qMax(qMin(mixed_sample, static_cast<qint32>(INT16_MAX)), static_cast<qint32>(INT16_MIN));
	return static_cast<qint16>(mixed_sample);
}
