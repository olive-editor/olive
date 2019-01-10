#include "loadthread.h"

#include "mainwindow.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "panels/project.h"
#include "project/footage.h"
#include "io/config.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "project/transition.h"
#include "project/effect.h"
#include "playback/playback.h"
#include "io/previewgenerator.h"
#include "dialogs/loaddialog.h"
#include "project/media.h"
#include "effects/internal/voideffect.h"
#include "debug.h"

#include <QFile>
#include <QMessageBox>
#include <QTreeWidgetItem>

struct TransitionData {
	int id;
	QString name;
	long length;
	Clip* otc;
	Clip* ctc;
};

LoadThread::LoadThread(LoadDialog* l, bool a) : ld(l), autorecovery(a), cancelled(false) {
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
	connect(this, SIGNAL(success()), this, SLOT(success_func()));
	connect(this, SIGNAL(error()), this, SLOT(error_func()));
	connect(this, SIGNAL(start_create_dual_transition(const TransitionData*,Clip*,Clip*,const EffectMeta*)), this, SLOT(create_dual_transition(const TransitionData*,Clip*,Clip*,const EffectMeta*)));
	connect(this, SIGNAL(start_create_effect_ui(QXmlStreamReader*, Clip*, int, const QString*, const EffectMeta*, long, bool)), this, SLOT(create_effect_ui(QXmlStreamReader*, Clip*, int, const QString*, const EffectMeta*, long, bool)));
}

const EffectMeta* get_meta_from_name(const QString& name) {
	for (int j=0;j<effects.size();j++) {
		if (effects.at(j).name == name) {
			return &effects.at(j);
		}
	}
	return NULL;
}

void LoadThread::load_effect(QXmlStreamReader& stream, Clip* c) {
	int effect_id = -1;
	QString effect_name;
	bool effect_enabled = true;
	long effect_length = -1;
	for (int j=0;j<stream.attributes().size();j++) {
		const QXmlStreamAttribute& attr = stream.attributes().at(j);
		if (attr.name() == "id") {
			effect_id = attr.value().toInt();
		} else if (attr.name() == "enabled") {
			effect_enabled = (attr.value() == "1");
		} else if (attr.name() == "name") {
			effect_name = attr.value().toString();
		} else if (attr.name() == "length") {
			effect_length = attr.value().toLong();
		}
	}

	// wait for effects to be loaded
	panel_effect_controls->effects_loaded.lock();

	const EffectMeta* meta = NULL;

	// find effect with this name
	if (!effect_name.isEmpty()) {
		meta = get_meta_from_name(effect_name);
	}

	panel_effect_controls->effects_loaded.unlock();

	QString tag = stream.name().toString();

	int type;
	if (tag == "opening") {
		type = TA_OPENING_TRANSITION;
	} else if (tag == "closing") {
		type = TA_CLOSING_TRANSITION;
	} else {
		type = TA_NO_TRANSITION;
	}

	emit start_create_effect_ui(&stream, c, type, &effect_name, meta, effect_length, effect_enabled);
	waitCond.wait(&mutex);
}

void LoadThread::read_next(QXmlStreamReader &stream) {
	stream.readNext();
	update_current_element_count(stream);
}

void LoadThread::read_next_start_element(QXmlStreamReader &stream) {
	stream.readNextStartElement();
	update_current_element_count(stream);
}

void LoadThread::update_current_element_count(QXmlStreamReader &stream) {
	if (is_element(stream)) {
		current_element_count++;
		report_progress((current_element_count * 100) / total_element_count);
	}
}

bool LoadThread::is_element(QXmlStreamReader &stream) {
	return stream.isStartElement()
			&& (stream.name() == "folder"
			|| stream.name() == "footage"
			|| stream.name() == "sequence"
			|| stream.name() == "clip"
			|| stream.name() == "effect");
}

bool LoadThread::load_worker(QFile& f, QXmlStreamReader& stream, int type) {
	f.seek(0);
	stream.setDevice(stream.device());

	QString root_search;
	QString child_search;

	switch (type) {
	case LOAD_TYPE_VERSION:
		root_search = "version";
		break;
	case LOAD_TYPE_URL:
		root_search = "url";
		break;
	case MEDIA_TYPE_FOLDER:
		root_search = "folders";
		child_search = "folder";
		break;
	case MEDIA_TYPE_FOOTAGE:
		root_search = "media";
		child_search = "footage";
		break;
	case MEDIA_TYPE_SEQUENCE:
		root_search = "sequences";
		child_search = "sequence";
		break;
	}

	show_err = true;

	while (!stream.atEnd() && !cancelled) {
		read_next_start_element(stream);
		if (stream.name() == root_search) {
			if (type == LOAD_TYPE_VERSION) {
				int proj_version = stream.readElementText().toInt();
				if (proj_version < MIN_SAVE_VERSION && proj_version > SAVE_VERSION) {
					if (QMessageBox::warning(mainWindow, "Version Mismatch", "This project was saved in a different version of Olive and may not be fully compatible with this version. Would you like to attempt loading it anyway?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
						show_err = false;
						return false;
					}
				}
			} else if (type == LOAD_TYPE_URL) {
				internal_proj_url = stream.readElementText();
				internal_proj_dir = QFileInfo(internal_proj_url).absoluteDir();
			} else {
				while (!cancelled && !stream.atEnd() && !(stream.name() == root_search && stream.isEndElement())) {
					read_next(stream);
					if (stream.name() == child_search && stream.isStartElement()) {
						switch (type) {
						case MEDIA_TYPE_FOLDER:
						{
							Media* folder = panel_project->new_folder(0);
							folder->temp_id2 = 0;
							for (int j=0;j<stream.attributes().size();j++) {
								const QXmlStreamAttribute& attr = stream.attributes().at(j);
								if (attr.name() == "id") {
									folder->temp_id = attr.value().toInt();
								} else if (attr.name() == "name") {
									folder->set_name(attr.value().toString());
								} else if (attr.name() == "parent") {
									folder->temp_id2 = attr.value().toInt();
								}
							}
							loaded_folders.append(folder);
						}
							break;
						case MEDIA_TYPE_FOOTAGE:
						{
							int folder = 0;

							Media* item = new Media(0);
							Footage* m = new Footage();

							m->using_inout = false;

							for (int j=0;j<stream.attributes().size();j++) {
								const QXmlStreamAttribute& attr = stream.attributes().at(j);
								if (attr.name() == "id") {
									m->save_id = attr.value().toInt();
								} else if (attr.name() == "folder") {
									folder = attr.value().toInt();
								} else if (attr.name() == "name") {
									m->name = attr.value().toString();
								} else if (attr.name() == "url") {
									m->url = attr.value().toString();

									if (!QFileInfo::exists(m->url)) { // if path is not absolute
										QString proj_dir_test = proj_dir.absoluteFilePath(m->url);
										QString internal_proj_dir_test = internal_proj_dir.absoluteFilePath(m->url);

										if (QFileInfo::exists(proj_dir_test)) { // if path is relative to the project's current dir
											m->url = proj_dir_test;
											dout << "[INFO] Matched" << attr.value().toString() << "relative to project's current directory";
										} else if (QFileInfo::exists(internal_proj_dir_test)) { // if path is relative to the last directory the project was saved in
											m->url = internal_proj_dir_test;
											dout << "[INFO] Matched" << attr.value().toString() << "relative to project's internal directory";
										} else if (m->url.contains('%')) {
											// hack for image sequences (qt won't be able to find the URL with %, but ffmpeg may)
											m->url = internal_proj_dir_test;
											dout << "[INFO] Guess image sequence" << attr.value().toString() << "path to project's internal directory";
										} else {
											dout << "[INFO] Failed to match" << attr.value().toString() << "to file";
										}
									} else {
										dout << "[INFO] Matched" << attr.value().toString() << "with absolute path";
									}
								} else if (attr.name() == "duration") {
									m->length = attr.value().toLongLong();
								} else if (attr.name() == "using_inout") {
									m->using_inout = (attr.value() == "1");
								} else if (attr.name() == "in") {
									m->in = attr.value().toLong();
								} else if (attr.name() == "out") {
									m->out = attr.value().toLong();
								} else if (attr.name() == "speed") {
									m->speed = attr.value().toDouble();
								}
							}

							item->set_footage(m);

							project_model.appendChild(find_loaded_folder_by_id(folder), item);

							// analyze media to see if it's the same
							loaded_media_items.append(item);
						}
							break;
						case MEDIA_TYPE_SEQUENCE:
						{
							Media* parent = NULL;
							Sequence* s = new Sequence();

							// load attributes about sequence
							for (int j=0;j<stream.attributes().size();j++) {
								const QXmlStreamAttribute& attr = stream.attributes().at(j);
								if (attr.name() == "name") {
									s->name = attr.value().toString();
								} else if (attr.name() == "folder") {
									int folder = attr.value().toInt();
									if (folder > 0) parent = find_loaded_folder_by_id(folder);
								} else if (attr.name() == "id") {
									s->save_id = attr.value().toInt();
								} else if (attr.name() == "width") {
									s->width = attr.value().toInt();
								} else if (attr.name() == "height") {
									s->height = attr.value().toInt();
								} else if (attr.name() == "framerate") {
									s->frame_rate = attr.value().toDouble();
								} else if (attr.name() == "afreq") {
									s->audio_frequency = attr.value().toInt();
								} else if (attr.name() == "alayout") {
									s->audio_layout = attr.value().toInt();
								} else if (attr.name() == "open") {
									open_seq = s;
								} else if (attr.name() == "workarea") {
									s->using_workarea = (attr.value() == "1");
                                } else if (attr.name() == "workareaEnabled") {
                                    s->enable_workarea = (attr.value() == "1");
								} else if (attr.name() == "workareaIn") {
									s->workarea_in = attr.value().toLong();
								} else if (attr.name() == "workareaOut") {
									s->workarea_out = attr.value().toLong();
								}
							}

							QVector<TransitionData> transition_data;

							// load all clips and clip information
							while (!cancelled && !(stream.name() == child_search && stream.isEndElement()) && !stream.atEnd()) {
								read_next_start_element(stream);
								if (stream.name() == "marker" && stream.isStartElement()) {
									Marker m;
									for (int j=0;j<stream.attributes().size();j++) {
										const QXmlStreamAttribute& attr = stream.attributes().at(j);
										if (attr.name() == "frame") {
											m.frame = attr.value().toLong();
										} else if (attr.name() == "name") {
											m.name = attr.value().toString();
										}
									}
									s->markers.append(m);
								} else if (stream.name() == "transition" && stream.isStartElement()) {
									TransitionData td;
									td.otc = NULL;
									td.ctc = NULL;
									for (int j=0;j<stream.attributes().size();j++) {
										const QXmlStreamAttribute& attr = stream.attributes().at(j);
										if (attr.name() == "id") {
											td.id = attr.value().toInt();
										} else if (attr.name() == "name") {
											td.name = attr.value().toString();
										} else if (attr.name() == "length") {
											td.length = attr.value().toLong();
										}
									}
									transition_data.append(td);
								} else if (stream.name() == "clip" && stream.isStartElement()) {
									int media_type = -1;
									int media_id, stream_id;
									Clip* c = new Clip(s);

									// backwards compatibility code
									c->autoscale = false;

									c->media = NULL;

									for (int j=0;j<stream.attributes().size();j++) {
										const QXmlStreamAttribute& attr = stream.attributes().at(j);
										if (attr.name() == "name") {
											c->name = attr.value().toString();
										} else if (attr.name() == "enabled") {
											c->enabled = (attr.value() == "1");
										} else if (attr.name() == "id") {
											c->load_id = attr.value().toInt();
										} else if (attr.name() == "clipin") {
											c->clip_in = attr.value().toLong();
										} else if (attr.name() == "in") {
											c->timeline_in = attr.value().toLong();
										} else if (attr.name() == "out") {
											c->timeline_out = attr.value().toLong();
										} else if (attr.name() == "track") {
											c->track = attr.value().toInt();
										} else if (attr.name() == "r") {
											c->color_r = attr.value().toInt();
										} else if (attr.name() == "g") {
											c->color_g = attr.value().toInt();
										} else if (attr.name() == "b") {
											c->color_b = attr.value().toInt();
										} else if (attr.name() == "autoscale") {
											c->autoscale = (attr.value() == "1");
										} else if (attr.name() == "media") {
											media_type = MEDIA_TYPE_FOOTAGE;
											media_id = attr.value().toInt();
										} else if (attr.name() == "stream") {
											stream_id = attr.value().toInt();
										} else if (attr.name() == "speed") {
											c->speed = attr.value().toDouble();
										} else if (attr.name() == "maintainpitch") {
											c->maintain_audio_pitch = (attr.value() == "1");
										} else if (attr.name() == "reverse") {
											c->reverse = (attr.value() == "1");
										} else if (attr.name() == "opening") {
											c->opening_transition = attr.value().toInt();
										} else if (attr.name() == "closing") {
											c->closing_transition = attr.value().toInt();
										} else if (attr.name() == "sequence") {
											media_type = MEDIA_TYPE_SEQUENCE;

											// since we haven't finished loading sequences, we defer linking this until later
											c->media = NULL;
											c->media_stream = attr.value().toInt();
											loaded_clips.append(c);
										}
									}

									// set media and media stream
									switch (media_type) {
									case MEDIA_TYPE_FOOTAGE:
										if (media_id >= 0) {
											for (int j=0;j<loaded_media_items.size();j++) {
												Footage* m = loaded_media_items.at(j)->to_footage();
												if (m->save_id == media_id) {
													c->media = loaded_media_items.at(j);
													c->media_stream = stream_id;
													break;
												}
											}
										}
										break;
									}

									// load links and effects
									while (!cancelled && !(stream.name() == "clip" && stream.isEndElement()) && !stream.atEnd()) {
										read_next(stream);
										if (stream.isStartElement()) {
											if (stream.name() == "linked") {
												while (!cancelled && !(stream.name() == "linked" && stream.isEndElement()) && !stream.atEnd()) {
													read_next(stream);
													if (stream.name() == "link" && stream.isStartElement()) {
														for (int k=0;k<stream.attributes().size();k++) {
															const QXmlStreamAttribute& link_attr = stream.attributes().at(k);
															if (link_attr.name() == "id") {
																c->linked.append(link_attr.value().toInt());
																break;
															}
														}
													}
												}
												if (cancelled) return false;
											} else if (stream.isStartElement() && (stream.name() == "effect" || stream.name() == "opening" || stream.name() == "closing")) {
												// "opening" and "closing" are backwards compatibility code
												load_effect(stream, c);
											}
										}
									}
									if (cancelled) return false;

									s->clips.append(c);
								}
							}
							if (cancelled) return false;

							// correct links, clip IDs, transitions
							for (int i=0;i<s->clips.size();i++) {
								// correct links
								Clip* correct_clip = s->clips.at(i);
								for (int j=0;j<correct_clip->linked.size();j++) {
									bool found = false;
									for (int k=0;k<s->clips.size();k++) {
										if (s->clips.at(k)->load_id == correct_clip->linked.at(j)) {
											correct_clip->linked[j] = k;
											found = true;
											break;
										}
									}
									if (!found) {
										correct_clip->linked.removeAt(j);
										j--;
										if (QMessageBox::warning(mainWindow, "Invalid Clip Link", "This project contains an invalid clip link. It may be corrupt. Would you like to continue loading it?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
											delete s;
											return false;
										}
									}
								}

								// re-link clips to transitions
								if (correct_clip->opening_transition > -1) {
									for (int j=0;j<transition_data.size();j++) {
										if (transition_data.at(j).id == correct_clip->opening_transition) {
											transition_data[j].otc = correct_clip;
										}
									}
								}
								if (correct_clip->closing_transition > -1) {
									for (int j=0;j<transition_data.size();j++) {
										if (transition_data.at(j).id == correct_clip->closing_transition) {
											transition_data[j].ctc = correct_clip;
										}
									}
								}
							}

							// create transitions
							for (int i=0;i<transition_data.size();i++) {
								const TransitionData& td = transition_data.at(i);
								Clip* primary = td.otc;
								Clip* secondary = td.ctc;
								if (primary != NULL || secondary != NULL) {
									if (primary == NULL) {
										primary = secondary;
										secondary = NULL;
									}
									const EffectMeta* meta = get_meta_from_name(td.name);
									if (meta == NULL) {
										dout << "[WARNING] Failed to link transition with name:" << td.name;
										if (td.otc != NULL) td.otc->opening_transition = -1;
										if (td.ctc != NULL) td.ctc->closing_transition = -1;
									} else {
										emit start_create_dual_transition(&td, primary, secondary, meta);

										waitCond.wait(&mutex);
									}
								}
							}

							Media* m = panel_project->new_sequence(NULL, s, false, parent);

							loaded_sequences.append(m);
						}
							break;
						}
					}
				}
				if (cancelled) return false;
			}
			break;
		}
	}
	return !cancelled;
}

Media* LoadThread::find_loaded_folder_by_id(int id) {
	if (id == 0) return NULL;
	for (int j=0;j<loaded_folders.size();j++) {
		Media* parent_item = loaded_folders.at(j);
		if (parent_item->temp_id == id) {
			return parent_item;
		}
	}
	return NULL;
}

void LoadThread::run() {
	mutex.lock();

	QFile file(project_url);
	if (!file.open(QIODevice::ReadOnly)) {
		dout << "[ERROR] Could not open file";
		return;
	}

	/* set up directories to search for media
	 * most of the time, these will be the same but in
	 * case the project file has moved without the footage,
	 * we check both
	 */
	proj_dir = QFileInfo(project_url).absoluteDir();
	internal_proj_dir = QFileInfo(project_url).absoluteDir();
	internal_proj_url = project_url;

	QXmlStreamReader stream(&file);

	bool cont = false;
	error_str.clear();
	show_err = true;

	// temp variables for loading (unnecessary?)
	open_seq = NULL;
	loaded_folders.clear();
	loaded_media_items.clear();
	loaded_clips.clear();
	loaded_sequences.clear();

	// get "element" count
	current_element_count = 0;
	total_element_count = 0;
	while (!cancelled && !stream.atEnd()) {
		stream.readNextStartElement();
		if (is_element(stream)) {
			total_element_count++;
		}
	}
	cont = !cancelled;

	// find project file version
	cont = load_worker(file, stream, LOAD_TYPE_VERSION);

	// find project's internal URL
	cont = load_worker(file, stream, LOAD_TYPE_URL);

	// load folders first
	if (cont) {
		cont = load_worker(file, stream, MEDIA_TYPE_FOLDER);
	}

	// load media
	if (cont) {
		// since folders loaded correctly, organize them appropriately
		for (int i=0;i<loaded_folders.size();i++) {
			Media* folder = loaded_folders.at(i);
			int parent = folder->temp_id2;
			project_model.appendChild(find_loaded_folder_by_id(parent), folder);
		}

		cont = load_worker(file, stream, MEDIA_TYPE_FOOTAGE);
	}

	// load sequences
	if (cont) {
		cont = load_worker(file, stream, MEDIA_TYPE_SEQUENCE);
	}

	if (!cancelled) {
		if (!cont) {
			xml_error = false;
			if (show_err) emit error();
		} else if (stream.hasError()) {
			error_str = stream.errorString() + " - Line: " + QString::number(stream.lineNumber()) + " Col:" + QString::number(stream.columnNumber());
			xml_error = true;
			emit error();
			cont = false;

		} else {
			// attach nested sequence clips to their sequences
			for (int i=0;i<loaded_clips.size();i++) {
				for (int j=0;j<loaded_sequences.size();j++) {
					if (loaded_clips.at(i)->media == NULL && loaded_clips.at(i)->media_stream == loaded_sequences.at(j)->to_sequence()->save_id) {
						loaded_clips.at(i)->media = loaded_sequences.at(j);
						loaded_clips.at(i)->refresh();
						break;
					}
				}
			}
		}
	}

	if (cont) {
		emit success(); // run in main thread

		for (int i=0;i<loaded_media_items.size();i++) {
			panel_project->start_preview_generator(loaded_media_items.at(i), true);
		}
	}

	file.close();

	mutex.unlock();
}

void LoadThread::cancel() {
	waitCond.wakeAll();
	cancelled = true;
}

void LoadThread::error_func() {
	if (xml_error) {
		dout << "[ERROR] Error parsing XML." << error_str;
		QMessageBox::critical(mainWindow, "XML Parsing Error", "Couldn't load '" + project_url + "'. " + error_str, QMessageBox::Ok);
	} else {
		QMessageBox::critical(mainWindow, "Project Load Error", "Error loading project: " + error_str, QMessageBox::Ok);
	}
}

void LoadThread::success_func() {
	if (autorecovery) {
		QString orig_filename = internal_proj_url;
		int insert_index = internal_proj_url.lastIndexOf(".ove", -1, Qt::CaseInsensitive);
		if (insert_index == -1) insert_index = internal_proj_url.length();
		int counter = 1;
		while (QFileInfo::exists(orig_filename)) {
			orig_filename = internal_proj_url;
			QString recover_text = "recovered";
			if (counter > 1) {
				recover_text += " " + QString::number(counter);
			}
			orig_filename.insert(insert_index, " (" + recover_text + ")");
			counter++;
		}
		mainWindow->updateTitle(orig_filename);
	} else {
		panel_project->add_recent_project(project_url);
	}

	mainWindow->setWindowModified(autorecovery);
	if (open_seq != NULL) set_sequence(open_seq);
	update_ui(false);
}

void LoadThread::create_effect_ui(
		QXmlStreamReader* stream,
		Clip* c,
		int type,
		const QString* effect_name,
		const EffectMeta* meta,
		long effect_length,
		bool effect_enabled)
{
	/* This is extremely hacky - prepare yourself.
	 *
	 * When moving project loading to a separate thread, it was soon discovered
	 * that effects wouldn't load correctly anymore. They were actually still
	 * "functional", but there were no controls appearing in EffectControls.
	 *
	 * Turns out since Effect creates its UI in its constructor, the UI was
	 * created in this thread rather than the main GUI thread, which is a big
	 * no-no. Unfortunately the design of Effect does not separate UI and data,
	 * so having the UI set up was integral to creating annd loading the effect.
	 *
	 * Therefore, rather than rewrite the class (I just rewrote QTreeWidget to
	 * QTreeView with a custom model/item so I'm exhausted), for
	 * quick-n-dirty-ness, I made LoadThread offload the effect creation to the
	 * main thread (and since the effect loads data from the same XML stream,
	 * the LoadThread has to wait for the effect to finish before it can
	 * continue.
	 *
	 * Sorry. I'll fix it one day.
	 */

	if (cancelled) return;
	if (type == TA_NO_TRANSITION) {
		if (meta == NULL) {
			// create void effect
			VoidEffect* ve = new VoidEffect(c, *effect_name);
			ve->set_enabled(effect_enabled);
			ve->load(*stream);
			c->effects.append(ve);
		} else {
			Effect* e = create_effect(c, meta);
			e->set_enabled(effect_enabled);
			e->load(*stream);

			c->effects.append(e);
		}
	} else {
		int transition_index = create_transition(c, NULL, meta);
		Transition* t = c->sequence->transitions.at(transition_index);
		if (effect_length > -1) t->set_length(effect_length);
		t->set_enabled(effect_enabled);
		t->load(*stream);

		if (type == TA_OPENING_TRANSITION) {
			c->opening_transition = transition_index;
		} else {
			c->closing_transition = transition_index;
		}
	}

	waitCond.wakeAll();
}

void LoadThread::create_dual_transition(const TransitionData* td, Clip* primary, Clip* secondary, const EffectMeta* meta) {
	int transition_index = create_transition(primary, secondary, meta);
	primary->sequence->transitions.at(transition_index)->set_length(td->length);
	if (td->otc != NULL) td->otc->opening_transition = transition_index;
	if (td->ctc != NULL) td->ctc->closing_transition = transition_index;
	waitCond.wakeAll();
}
