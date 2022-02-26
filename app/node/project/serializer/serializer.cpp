/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "serializer.h"

#include <QApplication>
#include <QFile>
#include <QXmlStreamReader>

#include "common/xmlutils.h"
#include "core.h"
#include "serializer190219.h"
#include "serializer210528.h"
#include "serializer210907.h"
#include "serializer211228.h"

namespace olive {

QVector<ProjectSerializer*> ProjectSerializer::instances_;

void ProjectSerializer::Initialize()
{
  // Make sure to order these from oldest to newest

  // FIXME: Implement this - yes it's a 0.1 project loader
  //instances_.append(new ProjectSerializer190219);

  instances_.append(new ProjectSerializer210528);
  instances_.append(new ProjectSerializer210907);
  instances_.append(new ProjectSerializer211228);
}

void ProjectSerializer::Destroy()
{
  qDeleteAll(instances_);
  instances_.clear();
}

ProjectSerializer::Result ProjectSerializer::Load(Project *project, const QString &filename)
{
  QFile project_file(filename);

  if (project_file.open(QFile::ReadOnly | QFile::Text)) {
    QXmlStreamReader reader(&project_file);

    Result inner_result = Load(project, &reader);

    project_file.close();

    if (inner_result.code() != kSuccess) {
      return inner_result;
    }

    if (reader.hasError()) {
      Result r(kXmlError);
      r.SetDetails(reader.errorString());
      return r;
    } else {
      return kSuccess;
    }
  } else {
    return kFileError;
  }
}

ProjectSerializer::Result ProjectSerializer::Load(Project *project, QXmlStreamReader *reader)
{
  // Determine project version
  uint version = 0;

  while (!version && XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("olive") || reader->name() == QStringLiteral("project")) {
      while(!version && XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("version")) {
          version = reader->readElementText().toUInt();
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  // Failed to find version in file
  if (version == 0) {
    return kUnknownVersion;
  }

  // We should now have the version, if we have a serializer for it, use it to load the project
  ProjectSerializer *serializer = nullptr;

  foreach (ProjectSerializer *s, instances_) {
    if (version == s->Version()) {
      serializer = s;
      break;
    } else if (version < s->Version()) {
      // Assuming the instance list is in order, if the project version is less than any version
      // we find, we must not support it anymore
      return kProjectTooOld;
    }
  }

  if (serializer) {
    LoadData ld = serializer->Load(project, reader, nullptr);
    Result r(kSuccess);
    if (reader->hasError()) {
      r = Result(kXmlError);
      r.SetDetails(reader->errorString());
    }
    r.SetLoadData(ld);
    return r;
  } else {
    // Reached the end of the list with no serializer, assume too new
    return kProjectTooNew;
  }
}

ProjectSerializer::Result ProjectSerializer::Save(const SaveData &data)
{
  QString temp_save = FileFunctions::GetSafeTemporaryFilename(data.GetFilename());

  QFile project_file(temp_save);

  if (project_file.open(QFile::WriteOnly | QFile::Text)) {
    QXmlStreamWriter writer(&project_file);

    Result inner_result = Save(&writer, data);

    project_file.close();

    if (inner_result != kSuccess) {
      return inner_result;
    }

    // Save was successful, we can now rewrite the original file
    if (FileFunctions::RenameFileAllowOverwrite(temp_save, data.GetFilename())) {
      return kSuccess;
    } else {
      Result r(kOverwriteError);
      r.SetDetails(temp_save);
      return r;
    }
  } else {
    Result r(kFileError);
    r.SetDetails(temp_save);
    return r;
  }
}

ProjectSerializer::Result ProjectSerializer::Save(QXmlStreamWriter *writer, const SaveData &data)
{
  writer->setAutoFormatting(true);

  writer->writeStartDocument();

  writer->writeStartElement("olive");

  // By default, save as last serializer which, assuming the instances are ordered correctly,
  // will be the newest file format. But we may allow saving as older versions later on.
  ProjectSerializer *serializer = instances_.last();

  // Version is stored in YYMMDD from whenever the project format was last changed
  // Allows easy integer math for checking project versions.
  writer->writeTextElement(QStringLiteral("version"), QString::number(serializer->Version()));

  if (!data.GetFilename().isEmpty()) {
    writer->writeTextElement("url", data.GetFilename());
  }

  writer->writeStartElement(QStringLiteral("project"));

  serializer->Save(writer, data, nullptr);

  writer->writeEndElement(); // project

  writer->writeEndElement(); // olive

  writer->writeEndDocument();

  if (writer->hasError()) {
    return kXmlError;
  }

  return kSuccess;
}

bool ProjectSerializer::IsCancelled() const
{
  return false;
}

}
