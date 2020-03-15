/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "project.h"

#include <QDir>
#include <QFileInfo>

#include "common/xmlutils.h"
#include "core.h"
#include "dialog/progress/progress.h"
#include "window/mainwindow/mainwindow.h"

Project::Project()
{
  root_.set_project(this);
}

void Project::Load(QXmlStreamReader *reader, const QAtomicInt* cancelled)
{
  QHash<quintptr, StreamPtr> footage_ptrs;
  QList<NodeInput::FootageConnection> footage_connections;

  XMLReadLoop(reader, "project") {
    if (reader->isStartElement()) {
      if (reader->name() == "folder") {

        // Assume this folder is our root
        root_.Load(reader, footage_ptrs, footage_connections, cancelled);

      } else if (reader->name() == "colormanagement") {

        // Read color management info
        XMLReadLoop(reader, "colormanagement") {
          if (reader->name() == "config") {
            reader->readNext();
            set_ocio_config(reader->text().toString());
          } else if (reader->name() == "default") {
            reader->readNext();
            set_default_input_colorspace(reader->text().toString());
          }
        }
      }
    }
  }

  foreach (const NodeInput::FootageConnection& con, footage_connections) {
    if (con.footage) {
      con.input->set_standard_value(QVariant::fromValue(footage_ptrs.value(con.footage)));
    }
  }
}

void Project::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("project");

  writer->writeTextElement("url", filename_);

  root_.Save(writer);

  writer->writeStartElement("colormanagement");

  writer->writeTextElement("config", ocio_config_);

  writer->writeTextElement("default", default_input_colorspace_);

  writer->writeEndElement(); // colormanagement

  writer->writeEndElement(); // project
}

Folder *Project::root()
{
  return &root_;
}

QString Project::name() const
{
  if (filename_.isEmpty()) {
    return tr("(untitled)");
  } else {
    return QFileInfo(filename_).baseName();
  }
}

const QString &Project::filename() const
{
  return filename_;
}

void Project::set_filename(const QString &s)
{
  filename_ = s;

  emit NameChanged();
}

const QString &Project::ocio_config() const
{
  return ocio_config_;
}

void Project::set_ocio_config(const QString &ocio_config)
{
  ocio_config_ = ocio_config;
}

const QString &Project::default_input_colorspace() const
{
  return default_input_colorspace_;
}

void Project::set_default_input_colorspace(const QString &colorspace)
{
  default_input_colorspace_ = colorspace;
}

ColorManager *Project::color_manager()
{
  return &color_manager_;
}

QList<ItemPtr> Project::get_items_of_type(Item::Type type) const
{
  return root_.get_children_of_type(type, true);
}
