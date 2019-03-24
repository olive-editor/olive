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

#ifndef MEDIA_H
#define MEDIA_H

#include <QList>
#include <QVariant>
#include <QIcon>

#include "timeline/marker.h"
#include "project/footage.h"

enum MediaType {
  MEDIA_TYPE_FOOTAGE,
  MEDIA_TYPE_SEQUENCE,
  MEDIA_TYPE_FOLDER
};

class Sequence;
using SequencePtr = std::shared_ptr<Sequence>;

using VoidPtr = std::shared_ptr<void>;

class Media;
using MediaPtr = std::shared_ptr<Media>;

class Media
{
public:
  Media();

  Footage *to_footage();
  SequencePtr to_sequence();
  void set_icon(const QString& str);
  void set_icon(const QIcon &ico);
  void set_footage(FootagePtr f);
  void set_sequence(SequencePtr s);
  void set_folder();
  void set_parent(Media* p);
  void update_tooltip(const QString& error = nullptr);
  VoidPtr to_object();
  int get_type();
  const QString& get_name();
  void set_name(const QString& n);

  double get_frame_rate(int stream = -1);
  int get_sampling_rate(int stream = -1);

  // item functions
  void appendChild(MediaPtr child);
  bool setData(int col, const QVariant &value);
  Media *child(int row);
  int childCount() const;
  int columnCount() const;
  QVariant data(int column, int role);
  int row() const;
  Media *parentItem();
  void removeChild(int i);
  MediaPtr get_shared_ptr(Media* m);

  // get markers from internal object
  QVector<Marker>& get_markers();

  bool root;
  int temp_id;
  int temp_id2;

private:
  int type;
  VoidPtr object;

  QString GetStringDuration();

  // item functions
  QList<MediaPtr> children;
  Media* parent;
  QString folder_name;
  QString tooltip;
  QIcon icon;
};

#endif // MEDIA_H
