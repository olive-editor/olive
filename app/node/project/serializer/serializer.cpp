/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include "node/group/group.h"
#include "serializer190219.h"
#include "serializer210528.h"
#include "serializer210907.h"
#include "serializer211228.h"
#include "serializer220403.h"
#include "serializer230220.h"

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
  instances_.append(new ProjectSerializer220403);
  instances_.append(new ProjectSerializer230220);
}

void ProjectSerializer::Destroy()
{
  qDeleteAll(instances_);
  instances_.clear();
}

ProjectSerializer::Result ProjectSerializer::Load(Project *project, const QString &filename, LoadType load_type)
{
  QFile project_file(filename);

  if (project_file.open(QFile::ReadOnly)) {
    // Some project files are compressed, marked with "OVEC" at the beginning of the file. Check for
    // that signature now.
    std::unique_ptr<QXmlStreamReader> reader;
    if (CheckCompressedID(&project_file)) {
      // File is compressed, decompress into memory
      QByteArray b;
      b = qUncompress(project_file.readAll());
      reader.reset(new QXmlStreamReader(b));
    } else {
      project_file.seek(0);
      reader.reset(new QXmlStreamReader(&project_file));
    }

    Result inner_result = Load(project, reader.get(), load_type);

    project_file.close();

    if (inner_result.code() != kSuccess) {
      return inner_result;
    }

    if (reader->hasError()) {
      Result r(kXmlError);
      r.SetDetails(reader->errorString());
      return r;
    } else {
      return inner_result;
    }
  } else {
    return kFileError;
  }
}

ProjectSerializer::Result ProjectSerializer::Load(Project *project, QXmlStreamReader *reader, LoadType load_type)
{
  // Determine project version
  uint version = 0;
  Result res = kUnknownVersion;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("olive")
        || reader->name() == QStringLiteral("project")) { // 0.1 projects only

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("version")) { // 230220+ projects
          version = attr.value().toUInt();
        } else if (reader->name() == QStringLiteral("url")) { // 230220+ projects
          project->SetSavedURL(attr.value().toString());
        }
      }

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("version")) { // projects <= 220403
          version = reader->readElementText().toUInt();
        } else if (reader->name() == QStringLiteral("url")) { // projects <= 220403
          if (project) {
            project->SetSavedURL(reader->readElementText());
          } else {
            reader->skipCurrentElement();
          }
        } else {
          // Handle any other value with the serializer
          res = LoadWithSerializerVersion(version, project, reader, load_type);
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  return res;
}

ProjectSerializer::Result ProjectSerializer::Paste(LoadType load_type)
{
  QString clipboard = Core::PasteStringFromClipboard();
  if (clipboard.isEmpty()) {
    return kNoData;
  }

  QXmlStreamReader reader(clipboard);

  return ProjectSerializer::Load(nullptr, &reader, load_type);
}

ProjectSerializer::Result ProjectSerializer::Save(const SaveData &data, bool compress)
{
  QString temp_save = FileFunctions::GetSafeTemporaryFilename(data.GetFilename());

  QFile project_file(temp_save);

  if (project_file.open(QFile::WriteOnly)) {
    QByteArray b;
    QXmlStreamWriter writer(&b);

    Result inner_result = Save(&writer, data);

    if (writer.hasError()) {
      Result r(kXmlError);
      return r;
    }

    if (compress) {
      project_file.write("OVEC");
      project_file.write(qCompress(b));
    } else {
      project_file.write(b);
    }

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
  writer->writeAttribute(QStringLiteral("version"), QString::number(serializer->Version()));

  if (!data.GetFilename().isEmpty()) {
    writer->writeAttribute("url", data.GetFilename());
  }

  serializer->Save(writer, data, nullptr);

  writer->writeEndElement(); // olive

  writer->writeEndDocument();

  if (writer->hasError()) {
    return kXmlError;
  }

  return kSuccess;
}

ProjectSerializer::Result ProjectSerializer::Copy(const SaveData &data)
{
  QString copy_str;
  QXmlStreamWriter writer(&copy_str);

  ProjectSerializer::Result res = ProjectSerializer::Save(&writer, data);

  if (res == kSuccess) {
    Core::CopyStringToClipboard(copy_str);
  }

  return res;
}

bool ProjectSerializer::CheckCompressedID(QFile *file)
{
  QByteArray b = file->read(4);
  return !memcmp(b.data(), "OVEC", 4);
}

bool ProjectSerializer::IsCancelled() const
{
  return false;
}

ProjectSerializer::Result ProjectSerializer::LoadWithSerializerVersion(uint version, Project *project, QXmlStreamReader *reader, LoadType load_type)
{
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
    LoadData ld = serializer->Load(project, reader, load_type, nullptr);
    Result r(kSuccess);
    if (reader->hasError()) {
      r = Result(kXmlError);
      r.SetDetails(QCoreApplication::translate("Serializer", "%1 on line %2").arg(reader->errorString(), QString::number(reader->lineNumber())));
    }
    r.SetLoadData(ld);
    return r;
  } else {
    // Reached the end of the list with no serializer, assume too new
    return kProjectTooNew;
  }
}

void ProjectSerializer::SaveData::SetOnlySerializeNodesAndResolveGroups(QVector<Node *> nodes)
{
  // For any groups, add children
  for (int i=0; i<nodes.size(); i++) {
    // If this is a group, add the child nodes too
    if (NodeGroup *g = dynamic_cast<NodeGroup*>(nodes.at(i))) {
      for (auto it=g->GetContextPositions().cbegin(); it!=g->GetContextPositions().cend(); it++) {
        if (!nodes.contains(it.key())) {
          nodes.append(it.key());
        }
      }
    }
  }

  SetOnlySerializeNodes(nodes);
}

}
