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
#include "mainwindow.h"
#include "io/math.h"
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
#ifdef _WIN32
#include "effects/internal/vsthostwin.h"
#endif
#include "effects/internal/fillleftrighteffect.h"

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

QVector<EffectMeta> effects;

Effect* create_effect(Clip* c, const EffectMeta* em) {
	if (!em->filename.isEmpty()) {
		// load effect from file
		return new Effect(c, em);
	} else if (em->internal >= 0 && em->internal < EFFECT_INTERNAL_COUNT) {
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
#ifdef _WIN32
		case EFFECT_INTERNAL_VST: return new VSTHostWin(c, em);
#endif
		}
	} else {
		dout << "[ERROR] Invalid effect data";
		QMessageBox::critical(mainWindow, "Invalid effect", "No candidate for effect '" + em->name + "'. This effect may be corrupt. Try reinstalling it or Olive.");
	}
	return NULL;
}

const EffectMeta* get_internal_meta(int internal_id, int type) {
	for (int i=0;i<effects.size();i++) {
		if (effects.at(i).internal == internal_id && effects.at(i).type == type) {
			return &effects.at(i);
		}
	}
	return NULL;
}

void load_internal_effects() {
	EffectMeta em;

	// internal effects
	em.type = EFFECT_TYPE_EFFECT;
	em.subtype = EFFECT_TYPE_AUDIO;

	em.name = "Volume";
	em.internal = EFFECT_INTERNAL_VOLUME;
	effects.append(em);

	em.name = "Pan";
	em.internal = EFFECT_INTERNAL_PAN;
	effects.append(em);

#ifdef _WIN32
	em.name = "VST Plugin 2.x";
	em.internal = EFFECT_INTERNAL_VST;
	effects.append(em);
#endif

	em.name = "Tone";
	em.internal = EFFECT_INTERNAL_TONE;
	effects.append(em);

	em.name = "Noise";
	em.internal = EFFECT_INTERNAL_NOISE;
	effects.append(em);

	em.name = "Fill Left/Right";
	em.internal = EFFECT_INTERNAL_FILLLEFTRIGHT;
	effects.append(em);

	em.subtype = EFFECT_TYPE_VIDEO;

	em.name = "Transform";
	em.category = "Distort";
	em.internal = EFFECT_INTERNAL_TRANSFORM;
	effects.append(em);

	em.name = "Corner Pin";
	em.internal = EFFECT_INTERNAL_CORNERPIN;
	effects.append(em);

	/*em.name = "Mask";
	em.internal = EFFECT_INTERNAL_MASK;
	effects.append(em);*/

	em.name = "Shake";
	em.internal = EFFECT_INTERNAL_SHAKE;
	effects.append(em);

	em.name = "Text";
	em.category = "Render";
	em.internal = EFFECT_INTERNAL_TEXT;
	effects.append(em);

	em.name = "Timecode";
	em.internal = EFFECT_INTERNAL_TIMECODE;
	effects.append(em);

	em.name = "Solid";
	em.internal = EFFECT_INTERNAL_SOLID;
	effects.append(em);

	// internal transitions
	em.type = EFFECT_TYPE_TRANSITION;
	em.category = "";

	em.name = "Cross Dissolve";
	em.internal = TRANSITION_INTERNAL_CROSSDISSOLVE;
	effects.append(em);

	em.subtype = EFFECT_TYPE_AUDIO;

	em.name = "Linear Fade";
	em.internal = TRANSITION_INTERNAL_LINEARFADE;
	effects.append(em);

	em.name = "Exponential Fade";
	em.internal = TRANSITION_INTERNAL_EXPONENTIALFADE;
	effects.append(em);

	em.name = "Logarithmic Fade";
	em.internal = TRANSITION_INTERNAL_LOGARITHMICFADE;
	effects.append(em);
}

QList<QString> get_effects_paths() {
	QList<QString> effects_paths;
	effects_paths.append(get_app_dir() + "/effects");
	effects_paths.append(get_app_dir() + "/../share/olive-editor/effects");
	QString env_path(qgetenv("OLIVE_EFFECTS_PATH"));
	if (!env_path.isEmpty()) effects_paths.append(env_path);
	return effects_paths;
}

void load_shader_effects() {
	QList<QString> effects_paths = get_effects_paths();

	for (int h=0;h<effects_paths.size();h++) {
		const QString& effects_path = effects_paths.at(h);
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
							em.type = EFFECT_TYPE_EFFECT;
							em.subtype = EFFECT_TYPE_VIDEO;
							em.name = effect_name;
							em.category = effect_cat;
							em.filename = file.fileName();
							em.path = effects_path;
							em.internal = -1;
							effects.append(em);
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
}

void load_vst_effects() {

}

void init_effects() {
	EffectInit* init_thread = new EffectInit();
	QObject::connect(init_thread, SIGNAL(finished()), init_thread, SLOT(deleteLater()));
	init_thread->start();
}

EffectInit::EffectInit() {
	panel_effect_controls->effects_loaded.lock();
}

void EffectInit::run() {
	dout << "[INFO] Initializing effects...";
	load_internal_effects();
	load_shader_effects();
	load_vst_effects();
	panel_effect_controls->effects_loaded.unlock();
	dout << "[INFO] Finished initializing effects";
}

Effect::Effect(Clip* c, const EffectMeta *em) :
	parent_clip(c),
	meta(em),
	enable_shader(false),
	enable_coords(false),
	enable_superimpose(false),
	enable_image(false),
	glslProgram(NULL),
	texture(NULL),
	isOpen(false),
	bound(false),
	enable_always_update(false)
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

	if (em != NULL) {
		// set up UI from effect file
		container->setText(em->name);

		if (!em->filename.isEmpty()) {
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
										dout << "[ERROR] Couldn't load field from" << em->filename << "- ID cannot be empty.";
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
									dout << "[ERROR] Failed to open superimpose script file for" << em->filename;
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
				dout << "[ERROR] Failed to open effect file" << em->filename;
			}
		}
	}
}

Effect::~Effect() {
	if (isOpen) {
		close();
	}

	//delete container;

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
	EffectRow* row = new EffectRow(this, savable, ui_layout, name, rows.size());
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

int Effect::gizmo_count(){
	return gizmos.size();
}

void Effect::refresh() {}

void Effect::field_changed() {
	panel_sequence_viewer->viewer_widget->update();
	panel_graph_editor->update_panel();
}

void Effect::show_context_menu(const QPoint& pos) {
	if (meta->type == EFFECT_TYPE_EFFECT) {
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
							dout << "[ERROR] Too many fields for effect" << id << "row" << row_count << ". Project might be corrupt. (Got" << field_count << ", expected <" << row->fieldCount()-1 << ")";
						}
						field_count++;
					}
				}

			} else {
				dout << "[ERROR] Too many rows for effect" << id << ". Project might be corrupt. (Got" << row_count << ", expected <" << rows.size()-1 << ")";
			}
			row_count++;
		} else if (stream.isStartElement()) {
			custom_load(stream);
		}
	}
}

void Effect::custom_load(QXmlStreamReader &stream) {}

void Effect::save(QXmlStreamWriter& stream) {
	stream.writeAttribute("name", meta->name);
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
		dout << "[WARNING] Tried to open an effect that was already open";
		close();
	}
	if (enable_shader) {
		if (QOpenGLContext::currentContext() == NULL) {
			dout << "[WARNING] No current context to create a shader program for - will retry next repaint";
		} else {
			glslProgram = new QOpenGLShaderProgram();
			validate_meta_path();
			bool glsl_compiled = true;
			if (!vertPath.isEmpty()) {
				if (glslProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, meta->path + "/" + vertPath)) {
					dout << "[INFO] Vertex shader added successfully";
				} else {
					glsl_compiled = false;
					dout << "[WARNING] Vertex shader could not be added";
				}
			}
			if (!fragPath.isEmpty()) {
				if (glslProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, meta->path + "/" + fragPath)) {
					dout << "[INFO] Fragment shader added successfully";
				} else {
					glsl_compiled = false;
					dout << "[WARNING] Fragment shader could not be added";
				}
			}
			if (glsl_compiled) {
				if (glslProgram->link()) {
					dout << "[INFO] Shader program linked successfully";
				} else {
					dout << "[WARNING] Shader program failed to link";
				}
			}
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
	delete_texture();
	if (glslProgram != NULL) {
		delete glslProgram;
		glslProgram = NULL;
	}
	isOpen = false;
}

bool Effect::is_glsl_linked() {
	return glslProgram != NULL && glslProgram->isLinked();
}

void Effect::startEffect() {
	if (!isOpen) {
		open();
		dout << "[WARNING] Tried to start a closed effect - opening";
	}
	if (enable_shader && glslProgram->isLinked()) bound = glslProgram->bind();
}

void Effect::endEffect() {
	if (bound) glslProgram->release();
	bound = false;
}

void Effect::process_image(double, uint8_t *, int) {}

Effect* Effect::copy(Clip* c) {
	Effect* copy = create_effect(c, meta);
	copy->set_enabled(is_enabled());
	copy_field_keyframes(copy);
	return copy;
}

void Effect::process_shader(double timecode, GLTextureCoords&) {
	glslProgram->setUniformValue("resolution", parent_clip->getWidth(), parent_clip->getHeight());
	glslProgram->setUniformValue("time", (GLfloat) timecode);

	for (int i=0;i<rows.size();i++) {
		EffectRow* row = rows.at(i);
		for (int j=0;j<row->fieldCount();j++) {
			EffectField* field = row->field(j);
			if (!field->id.isEmpty()) {
				switch (field->type) {
				case EFFECT_FIELD_DOUBLE:
					glslProgram->setUniformValue(field->id.toUtf8().constData(), (GLfloat) field->get_double_value(timecode));
					break;
				case EFFECT_FIELD_COLOR:
					glslProgram->setUniformValue(field->id.toUtf8().constData(), field->get_color_value(timecode).redF(), field->get_color_value(timecode).greenF(), field->get_color_value(timecode).blueF());
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

void Effect::process_coords(double, GLTextureCoords&, int data) {}

GLuint Effect::process_superimpose(double timecode) {
	bool recreate_texture = false;
	int width = parent_clip->getWidth();
	int height = parent_clip->getHeight();

	if (width != img.width() || height != img.height()) {
		img = QImage(width, height, QImage::Format_RGBA8888);
		recreate_texture = true;
	}

	if (valueHasChanged(timecode) || recreate_texture || enable_always_update) {
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

void Effect::process_audio(double, double, quint8*, int, int) {}

void Effect::gizmo_draw(double, GLTextureCoords &) {}

void Effect::gizmo_move(EffectGizmo* gizmo, int x_movement, int y_movement, double timecode, bool done) {
	for (int i=0;i<gizmos.size();i++) {
		if (gizmos.at(i) == gizmo) {
			ComboAction* ca = NULL;
			if (done) ca = new ComboAction();
			if (gizmo->x_field1 != NULL) {
				gizmo->x_field1->set_double_value(gizmo->x_field1->get_double_value(timecode) + x_movement*gizmo->x_field_multi1);
				gizmo->x_field1->make_key_from_change(ca);
			}
			if (gizmo->y_field1 != NULL) {
				gizmo->y_field1->set_double_value(gizmo->y_field1->get_double_value(timecode) + y_movement*gizmo->y_field_multi1);
				gizmo->y_field1->make_key_from_change(ca);
			}
			if (gizmo->x_field2 != NULL) {
				gizmo->x_field2->set_double_value(gizmo->x_field2->get_double_value(timecode) + x_movement*gizmo->x_field_multi2);
				gizmo->x_field2->make_key_from_change(ca);
			}
			if (gizmo->y_field2 != NULL) {
				gizmo->y_field2->set_double_value(gizmo->y_field2->get_double_value(timecode) + y_movement*gizmo->y_field_multi2);
				gizmo->y_field2->make_key_from_change(ca);
			}
			if (done) undo_stack.push(ca);
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
	if (texture != NULL) {
		delete texture;
		texture = NULL;
	}
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
