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

#ifndef LOADTHREAD_H
#define LOADTHREAD_H

#include <QThread>
#include <QDir>
#include <QXmlStreamReader>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>

#include "project/projectelements.h"
#include "timeline/clip.h"

class LoadThread : public QThread
{
  Q_OBJECT
public:
  LoadThread(const QString& filename, bool autorecovery, bool clear);
  void run();
public slots:
  void cancel();
signals:
  void start_question(const QString &title, const QString &text, int buttons);
  void success();
  void error();
  void report_progress(int p);
private slots:
  void question_func(const QString &title, const QString &text, int buttons);
  void error_func();
  void success_func();
private:
  bool autorecovery_;
  bool clear_;
  QString filename_;

  bool load_worker(QFile& f, QXmlStreamReader& stream, int type);
  void load_effect(QXmlStreamReader& stream, Clip* c);

  void read_next(QXmlStreamReader& stream);
  void read_next_start_element(QXmlStreamReader& stream);
  void update_current_element_count(QXmlStreamReader& stream);

  void show_message(const QString& title, const QString& body, int buttons);

  SequencePtr open_seq;
  QVector<Media*> loaded_media_items;
  QDir proj_dir;
  QDir internal_proj_dir;
  QString internal_proj_url;
  bool show_err;
  QString error_str;

  bool is_element(QXmlStreamReader& stream);

  QVector<MediaPtr> loaded_folders;
  QVector<ClipPtr> loaded_clips;
  QVector<Media*> loaded_sequences;
  Media* find_loaded_folder_by_id(int id);

  int current_element_count;
  int total_element_count;

  QMutex mutex;
  QWaitCondition waitCond;

  bool cancelled_;
  bool xml_error;

  QMessageBox::StandardButton question_btn;
};

#endif // LOADTHREAD_H
