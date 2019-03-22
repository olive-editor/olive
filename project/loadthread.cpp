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

#include "loadthread.h"

#include "global/global.h"

#include "ui/mainwindow.h"

#include "panels/panels.h"

#include "project/projectelements.h"

#include "global/config.h"
#include "rendering/renderfunctions.h"
#include "project/previewgenerator.h"
#include "effects/internal/voideffect.h"
#include "global/debug.h"

#include <QFile>
#include <QTreeWidgetItem>

LoadThread::LoadThread(const QString& filename, bool autorecovery) :
  filename_(filename),
  autorecovery_(autorecovery),
  cancelled_(false)
{
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));

  connect(this, SIGNAL(success()), this, SLOT(success_func()), Qt::QueuedConnection);

  connect(this, SIGNAL(error()), this, SLOT(error_func()), Qt::QueuedConnection);

  connect(this,
          SIGNAL(start_question(const QString&, const QString &, int)),
          this,
          SLOT(question_func(const QString &, const QString &, int)),
          Qt::QueuedConnection);
}

void LoadThread::load_effect(QXmlStreamReader& stream, Clip* c) {
  QString tag = stream.name().toString();

  // variables to store effect metadata in
  int effect_id = -1;
  QString effect_name;
  bool effect_enabled = true;
  long effect_length = -1;

  // loop through attributes for effect metadata
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
    } else if (attr.name() == "shared") {
      // if a transition has this tag, it's sharing a transition with another clip so we don't have to do any processing

      Clip* sharing_clip = nullptr;

      // Find the clip with the ID referenced in the transition
      int clip_id = attr.value().toInt();
      for (int i=0;i<c->sequence->clips.size();i++) {
        Clip* test_clip = c->sequence->clips.at(i).get();
        if (test_clip->load_id == clip_id) {
          sharing_clip = test_clip;
          break;
        }
      }

      if (sharing_clip == nullptr) {
        qWarning() << "Failed to link shared transition. Project may be corrupt.";
      } else if (tag == "opening") {
        c->opening_transition = (sharing_clip->closing_transition);

        // since this is the opened clip, switch secondaries and primaries
        c->opening_transition->secondary_clip = c->opening_transition->parent_clip;
        c->opening_transition->parent_clip = c;
        c->opening_transition->refresh();
      } else if (tag == "closing") {
        c->closing_transition = (sharing_clip->opening_transition);

        // since this is the closed clip, make this clip the secondary
        c->closing_transition->secondary_clip = c;
      }
      return;
    }
  }

  // Effect loading occurs in another thread, and while it's usually very quick, just for safety we wait here
  // for all the effects to finish loading
  panel_effect_controls->effects_loaded.lock();

  const EffectMeta* meta = nullptr;

  // find effect with this name
  if (!effect_name.isEmpty()) {
    meta = get_meta_from_name(effect_name);
  }

  panel_effect_controls->effects_loaded.unlock();

  int type;
  if (tag == "opening") {
    type = kTransitionOpening;
  } else if (tag == "closing") {
    type = kTransitionClosing;
  } else {
    type = kTransitionNone;
  }

  // effect construction
  if (cancelled_) return;
  if (type == kTransitionNone) {
    if (meta == nullptr) {
      // create void effect
      EffectPtr ve(new VoidEffect(c, effect_name));
      ve->SetEnabled(effect_enabled);
      ve->load(stream);

      ve->moveToThread(QApplication::instance()->thread());

      c->effects.append(ve);
    } else {
      EffectPtr e(Effect::Create(c, meta));
      e->SetEnabled(effect_enabled);
      e->load(stream);

      e->moveToThread(QApplication::instance()->thread());

      c->effects.append(e);
    }
  } else {
    TransitionPtr t = Transition::Create(c, nullptr, meta);
    if (effect_length > -1) t->set_length(effect_length);
    t->SetEnabled(effect_enabled);
    t->load(stream);

    t->moveToThread(QApplication::instance()->thread());

    if (type == kTransitionOpening) {
      c->opening_transition = t;
    } else {
      c->closing_transition = t;
    }
  }
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

void LoadThread::show_message(const QString& title, const QString& body, int buttons)
{
  emit start_question(
          title,
          body,
          buttons
        );
  waitCond.wait(&mutex);
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

  int proj_version = olive::kSaveVersion;

  while (!stream.atEnd() && !cancelled_) {
    read_next_start_element(stream);
    if (stream.name() == root_search) {
      if (type == LOAD_TYPE_VERSION) {
        proj_version = stream.readElementText().toInt();
        if (proj_version < olive::kMinimumSaveVersion || proj_version > olive::kSaveVersion) {
          show_message(
                  tr("Version Mismatch"),
                  tr("This project was saved in a different version of Olive and may not be fully compatible with this version. Would you like to attempt loading it anyway?"),
                  QMessageBox::Yes | QMessageBox::No
                );
          if (question_btn == QMessageBox::No) {
            show_err = false;
            return false;
          }
        }
      } else if (type == LOAD_TYPE_URL) {
        internal_proj_url = stream.readElementText();
        internal_proj_dir = QFileInfo(internal_proj_url).absoluteDir();
      } else {
        while (!cancelled_ && !stream.atEnd() && !(stream.name() == root_search && stream.isEndElement())) {
          read_next(stream);
          if (stream.name() == child_search && stream.isStartElement()) {
            switch (type) {
            case MEDIA_TYPE_FOLDER:
            {
              MediaPtr folder = panel_project->create_folder_internal(nullptr);
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

              MediaPtr item = std::make_shared<Media>();
              FootagePtr f = std::make_shared<Footage>();

              f->using_inout = false;

              for (int j=0;j<stream.attributes().size();j++) {
                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                if (attr.name() == "id") {
                  f->save_id = attr.value().toInt();
                } else if (attr.name() == "folder") {
                  folder = attr.value().toInt();
                } else if (attr.name() == "name") {
                  f->name = attr.value().toString();
                } else if (attr.name() == "url") {
                  f->url = attr.value().toString();

                  if (!QFileInfo::exists(f->url)) { // if path is not absolute
                    // tries to locate file using a file path relative to the project's current folder
                    QString proj_dir_test = proj_dir.absoluteFilePath(f->url);

                    // tries to locate file using a file path relative to the folder the project was saved in
                    // (unaffected by moving the project file)
                    QString internal_proj_dir_test = internal_proj_dir.absoluteFilePath(f->url);

                    // tries to locate file using the file name directly in the project's current folder
                    QString proj_dir_direct_test = proj_dir.filePath(QFileInfo(f->url).fileName());

                    if (QFileInfo::exists(proj_dir_test)) {

                      f->url = proj_dir_test;
                      qInfo() << "Matched" << attr.value().toString() << "relative to project's current directory";

                    } else if (QFileInfo::exists(internal_proj_dir_test)) {

                      f->url = internal_proj_dir_test;
                      qInfo() << "Matched" << attr.value().toString() << "relative to project's internal directory";

                    } else if (QFileInfo::exists(proj_dir_direct_test)) {

                      f->url = proj_dir_direct_test;
                      qInfo() << "Matched" << attr.value().toString() << "directly to project's current directory";

                    } else if (f->url.contains('%')) {

                      // hack for image sequences (qt won't be able to find the URL with %, but ffmpeg may)
                      f->url = internal_proj_dir_test;
                      qInfo() << "Guess image sequence" << attr.value().toString() << "path to project's internal directory";

                    } else {

                      qInfo() << "Failed to match" << attr.value().toString() << "to file";

                    }
                  } else {
                    f->url = QFileInfo(f->url).absoluteFilePath();
                    qInfo() << "Matched" << attr.value().toString() << "with absolute path";
                  }
                } else if (attr.name() == "duration") {
                  f->length = attr.value().toLongLong();
                } else if (attr.name() == "using_inout") {
                  f->using_inout = (attr.value() == "1");
                } else if (attr.name() == "in") {
                  f->in = attr.value().toLong();
                } else if (attr.name() == "out") {
                  f->out = attr.value().toLong();
                } else if (attr.name() == "speed") {
                  f->speed = attr.value().toDouble();
                } else if (attr.name() == "alphapremul") {
                  f->alpha_is_premultiplied = (attr.value() == "1");
                } else if (attr.name() == "proxy") {
                  f->proxy = (attr.value() == "1");
                } else if (attr.name() == "proxypath") {
                  f->proxy_path = attr.value().toString();
                } else if (attr.name() == "startnumber") {
                  f->start_number = attr.value().toInt();
                }
              }

              while (!cancelled_ && !(stream.name() == child_search && stream.isEndElement()) && !stream.atEnd()) {
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
                  f->markers.append(m);
                }
              }

              item->set_footage(f);

              olive::project_model.appendChild(find_loaded_folder_by_id(folder), item);

              // analyze media to see if it's the same
              loaded_media_items.append(item.get());
            }
              break;
            case MEDIA_TYPE_SEQUENCE:
            {
              Media* parent = nullptr;
              SequencePtr s = std::make_shared<Sequence>();

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
                } else if (attr.name() == "workareaIn") {
                  s->workarea_in = attr.value().toLong();
                } else if (attr.name() == "workareaOut") {
                  s->workarea_out = attr.value().toLong();
                }
              }

              // load all clips and clip information
              while (!cancelled_ && !(stream.name() == child_search && stream.isEndElement()) && !stream.atEnd()) {
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
                } else if (stream.name() == "clip" && stream.isStartElement()) {
                  int media_type = -1;
                  int media_id = -1;
                  int stream_id = -1;

                  ClipPtr c = std::make_shared<Clip>(s.get());

                  QColor clip_color;
                  ClipSpeed speed_info = c->speed();

                  for (int j=0;j<stream.attributes().size();j++) {
                    const QXmlStreamAttribute& attr = stream.attributes().at(j);
                    if (attr.name() == "name") {
                      c->set_name(attr.value().toString());
                    } else if (attr.name() == "enabled") {
                      c->set_enabled(attr.value() == "1");
                    } else if (attr.name() == "id") {
                      c->load_id = attr.value().toInt();
                    } else if (attr.name() == "clipin") {
                      c->set_clip_in(attr.value().toLong());
                    } else if (attr.name() == "in") {
                      c->set_timeline_in(attr.value().toLong());
                    } else if (attr.name() == "out") {
                      c->set_timeline_out(attr.value().toLong());
                    } else if (attr.name() == "track") {
                      c->set_track(attr.value().toInt());
                    } else if (attr.name() == "r") {
                      clip_color.setRed(attr.value().toInt());
                    } else if (attr.name() == "g") {
                      clip_color.setGreen(attr.value().toInt());
                    } else if (attr.name() == "b") {
                      clip_color.setBlue(attr.value().toInt());
                    } else if (attr.name() == "autoscale") {
                      c->set_autoscaled(attr.value() == "1");
                    } else if (attr.name() == "media") {
                      media_type = MEDIA_TYPE_FOOTAGE;
                      media_id = attr.value().toInt();
                    } else if (attr.name() == "stream") {
                      stream_id = attr.value().toInt();
                    } else if (attr.name() == "speed") {
                      speed_info.value = attr.value().toDouble();
                    } else if (attr.name() == "maintainpitch") {
                      speed_info.maintain_audio_pitch = (attr.value() == "1");
                    } else if (attr.name() == "reverse") {
                      c->set_reversed(attr.value() == "1");
                    } else if (attr.name() == "sequence") {
                      media_type = MEDIA_TYPE_SEQUENCE;

                      // since we haven't finished loading sequences, we defer linking this until later
                      c->set_media(nullptr, attr.value().toInt());
                      loaded_clips.append(c);
                    }
                  }

                  c->set_color(clip_color);
                  c->set_speed(speed_info);

                  // set media and media stream
                  switch (media_type) {
                  case MEDIA_TYPE_FOOTAGE:
                    if (media_id >= 0 && stream_id >= 0) {
                      for (int j=0;j<loaded_media_items.size();j++) {
                        Footage* m = loaded_media_items.at(j)->to_footage();
                        if (m->save_id == media_id) {
                          c->set_media(loaded_media_items.at(j), stream_id);
                          break;
                        }
                      }
                    }
                    break;
                  }

                  // load links and effects
                  while (!cancelled_ && !(stream.name() == "clip" && stream.isEndElement()) && !stream.atEnd()) {
                    read_next(stream);
                    if (stream.isStartElement()) {
                      if (stream.name() == "linked") {
                        while (!cancelled_ && !(stream.name() == "linked" && stream.isEndElement()) && !stream.atEnd()) {
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
                        if (cancelled_) return false;
                      } else if (stream.isStartElement()
                                 && (stream.name() == "effect"
                                     || stream.name() == "opening"
                                     || stream.name() == "closing")) {
                        load_effect(stream, c.get());
                      } else if (stream.name() == "marker" && stream.isStartElement()) {
                        Marker m;
                        for (int j=0;j<stream.attributes().size();j++) {
                          const QXmlStreamAttribute& attr = stream.attributes().at(j);
                          if (attr.name() == "frame") {
                            m.frame = attr.value().toLong();
                          } else if (attr.name() == "name") {
                            m.name = attr.value().toString();
                          }
                        }
                        c->get_markers().append(m);
                      }
                    }
                  }
                  if (cancelled_) return false;

                  s->clips.append(c);
                }
              }
              if (cancelled_) return false;

              // correct links, clip IDs, transitions
              for (int i=0;i<s->clips.size();i++) {
                // correct links
                Clip* correct_clip = s->clips.at(i).get();
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

                    show_message(
                          tr("Invalid Clip Link"),
                          tr("This project contains an invalid clip link. It may be corrupt. Would you like to continue loading it?"),
                          QMessageBox::Yes | QMessageBox::No
                          );

                    if (question_btn == QMessageBox::No) {
                      s.reset();
                      return false;
                    }
                  }
                }
              }

              MediaPtr m = panel_project->create_sequence_internal(nullptr, s, false, parent);

              loaded_sequences.append(m.get());
            }
              break;
            }
          }
        }
        if (cancelled_) return false;
      }
      break;
    }
  }
  return !cancelled_;
}

Media* LoadThread::find_loaded_folder_by_id(int id) {
  if (id == 0) return nullptr;
  for (int j=0;j<loaded_folders.size();j++) {
    Media* parent_item = loaded_folders.at(j).get();
    if (parent_item->temp_id == id) {
      return parent_item;
    }
  }
  return nullptr;
}

void LoadThread::OrganizeFolders(int folder) {
  for (int i=0;i<loaded_folders.size();i++) {
    MediaPtr item = loaded_folders.at(i);
    int parent_id = item->temp_id2;

    if (parent_id == folder) {
      olive::project_model.appendChild(find_loaded_folder_by_id(parent_id), item);

      OrganizeFolders(item->temp_id);
    }

  }
}

void LoadThread::run() {
  mutex.lock();

  QFile file(filename_);
  if (!file.open(QIODevice::ReadOnly)) {
    qCritical() << "Could not open file";
    return;
  }

  /* set up directories to search for media
   * most of the time, these will be the same but in
   * case the project file has moved without the footage,
   * we check both
   */
  proj_dir = QFileInfo(filename_).absoluteDir();
  internal_proj_dir = QFileInfo(filename_).absoluteDir();
  internal_proj_url = filename_;

  QXmlStreamReader stream(&file);

  bool cont = false;
  error_str.clear();
  show_err = true;

  open_seq = nullptr;

  // get "element" count
  current_element_count = 0;
  total_element_count = 0;
  while (!cancelled_ && !stream.atEnd()) {
    stream.readNextStartElement();
    if (is_element(stream)) {
      total_element_count++;
    }
  }
  cont = !cancelled_;

  // find project file version
  if (cont) {
    cont = load_worker(file, stream, LOAD_TYPE_VERSION);
  }

  // find project's internal URL
  if (cont) {
    cont = load_worker(file, stream, LOAD_TYPE_URL);
  }

  // load folders first
  if (cont) {
    cont = load_worker(file, stream, MEDIA_TYPE_FOLDER);
  }

  if (cont) {
    // since folders loaded correctly, organize them appropriately
    OrganizeFolders();

    // load media
    cont = load_worker(file, stream, MEDIA_TYPE_FOOTAGE);
  }

  // load sequences
  if (cont) {
    cont = load_worker(file, stream, MEDIA_TYPE_SEQUENCE);
  }

  if (!cancelled_) {
    if (!cont) {
      xml_error = false;
      if (show_err) emit error();
    } else if (stream.hasError()) {
      error_str = tr("%1 - Line: %2 Col: %3").arg(stream.errorString(), QString::number(stream.lineNumber()), QString::number(stream.columnNumber()));
      xml_error = true;
      emit error();
      cont = false;
    } else {
      // attach nested sequence clips to their sequences
      for (int i=0;i<loaded_clips.size();i++) {
        for (int j=0;j<loaded_sequences.size();j++) {
          if (loaded_clips.at(i)->media() == nullptr
              && loaded_clips.at(i)->media_stream_index() == loaded_sequences.at(j)->to_sequence()->save_id) {
            loaded_clips.at(i)->set_media(loaded_sequences.at(j), loaded_clips.at(i)->media_stream_index());
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
      PreviewGenerator::AnalyzeMedia(loaded_media_items.at(i));
    }
  } else {
    if (error_str.isEmpty()) {
      error_str = tr("User aborted loading");
    }

    emit error();
  }

  file.close();

  mutex.unlock();
}

void LoadThread::cancel() {
  waitCond.wakeAll();
  cancelled_ = true;
}

void LoadThread::question_func(const QString &title, const QString &text, int buttons) {
  mutex.lock();
  question_btn = QMessageBox::warning(
        olive::MainWindow,
        title,
        text,
        static_cast<enum QMessageBox::StandardButton>(buttons));
  mutex.unlock();
  waitCond.wakeAll();
}

void LoadThread::error_func() {
  if (xml_error) {
    qCritical() << "Error parsing XML." << error_str;
    QMessageBox::critical(olive::MainWindow,
                          tr("XML Parsing Error"),
                          tr("Couldn't load '%1'. %2").arg(filename_, error_str),
                          QMessageBox::Ok);
  } else {
    QMessageBox::critical(olive::MainWindow,
                          tr("Project Load Error"),
                          tr("Error loading project: %1").arg(error_str),
                          QMessageBox::Ok);
  }
}

void LoadThread::success_func() {
  if (autorecovery_) {
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

    olive::Global->update_project_filename(orig_filename);
  } else {
    panel_project->add_recent_project(filename_);
  }

  olive::Global->set_modified(autorecovery_);
  if (open_seq != nullptr) {
    olive::Global->set_sequence(open_seq);
  }
}
