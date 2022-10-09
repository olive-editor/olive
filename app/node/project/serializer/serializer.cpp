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
#include "serializer190219.h"
#include "serializer210528.h"
#include "serializer210907.h"
#include "serializer211228.h"
#include "serializer220403.h"

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
}

void ProjectSerializer::Destroy()
{
  qDeleteAll(instances_);
  instances_.clear();
}

ProjectSerializer::Result ProjectSerializer::Load(Project *project, const QString &filename, const QString &type)
{
  QFile project_file(filename);

  if (project_file.open(QFile::ReadOnly | QFile::Text)) {
    QXmlStreamReader reader(&project_file);

    Result inner_result = Load(project, &reader, type);

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

ProjectSerializer::Result ProjectSerializer::Load(Project *project, QXmlStreamReader *reader, const QString &type)
{
  // Determine project version
  uint version = 0;
  Result res = kUnknownVersion;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("olive")
        || reader->name() == QStringLiteral("project")) { // 0.1 projects only

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("version")) {
          version = reader->readElementText().toUInt();
        } else if (reader->name() == QStringLiteral("url")) {
          project->SetSavedURL(reader->readElementText());

          // HACK for 0.1 projects
          if (version == 190219) {
            res = LoadWithSerializerVersion(version, project, reader);
          }
        } else if (reader->name() == type) {
          // Found our data
          res = LoadWithSerializerVersion(version, project, reader);
        } else {
          reader->skipCurrentElement();
        }
      }

    } else {
      reader->skipCurrentElement();
    }
  }

  return res;
}

ProjectSerializer::Result ProjectSerializer::Paste(const QString &type)
{
  QString clipboard = Core::PasteStringFromClipboard();
  if (clipboard.isEmpty()) {
    return kNoData;
  }

  QXmlStreamReader reader(clipboard);

  Project temp;
  ProjectSerializer::Result res = ProjectSerializer::Load(&temp, &reader, type);

  if (res.code() != ProjectSerializer::kSuccess) {
    return res;
  }

  QVector<Node*> pasted_nodes;
  foreach (Node *n, temp.nodes()) {
    if (!temp.default_nodes().contains(n)) {
      // Move nodes out of Project
      n->setParent(nullptr);
      pasted_nodes.append(n);
    }
  }

  res.SetLoadedNodes(pasted_nodes);

  return res;
}

ProjectSerializer::Result ProjectSerializer::Save(const SaveData &data, const QString &type)
{
  QString temp_save = FileFunctions::GetSafeTemporaryFilename(data.GetFilename());

  QFile project_file(temp_save);

  if (project_file.open(QFile::WriteOnly | QFile::Text)) {
    QXmlStreamWriter writer(&project_file);

    Result inner_result = Save(&writer, data, type);

    if (writer.hasError()) {
      Result r(kXmlError);
      return r;
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

ProjectSerializer::Result ProjectSerializer::Save(QXmlStreamWriter *writer, const SaveData &data, const QString &type)
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

  writer->writeStartElement(type);

  serializer->Save(writer, data, nullptr);

  writer->writeEndElement(); // [type]

  writer->writeEndElement(); // olive

  writer->writeEndDocument();

  if (writer->hasError()) {
    return kXmlError;
  }

  return kSuccess;
}

ProjectSerializer::Result ProjectSerializer::Copy(const SaveData &data, const QString &type)
{
  QString copy_str;
  QXmlStreamWriter writer(&copy_str);

  ProjectSerializer::Result res = ProjectSerializer::Save(&writer, data, type);

  if (res == kSuccess) {
    Core::CopyStringToClipboard(copy_str);
  }

  return res;
}

bool ProjectSerializer::IsCancelled() const
{
  return false;
}

ProjectSerializer::Result ProjectSerializer::LoadWithSerializerVersion(uint version, Project *project, QXmlStreamReader *reader)
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
    LoadData ld = serializer->Load(project, reader, nullptr);
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
