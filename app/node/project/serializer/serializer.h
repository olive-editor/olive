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

#ifndef PROJECTSERIALIZER_H
#define PROJECTSERIALIZER_H

#include <QIODevice>

#include "common/define.h"
#include "node/project/project.h"

namespace olive {

/**
 * @brief An abstract base class for serializing/deserializing project data
 *
 * The goal of this is to further abstract serialized project data from their
 * in-memory representations.
 */
class ProjectSerializer
{
public:
  ProjectSerializer() = default;

  virtual ~ProjectSerializer(){}

  DISABLE_COPY_MOVE(ProjectSerializer)

  enum ResultCode {
    kSuccess,
    kProjectTooOld,
    kProjectTooNew,
    kUnknownVersion,
    kFileError,
    kXmlError,
    kOverwriteError,
    kNoData
  };

  using SerializedProperties = QHash<Node*, QMap<QString, QString> >;

  class LoadData
  {
  public:
    LoadData() = default;

    SerializedProperties properties;

    std::vector<TimelineMarker*> markers;

  };

  class Result
  {
  public:
    Result(const ResultCode &code) :
      code_(code)
    {}

    bool operator==(const ResultCode &code) { return code_ == code; }
    bool operator!=(const ResultCode &code) { return code_ != code; }

    const ResultCode &code() const { return code_; }

    const QString &GetDetails() const { return details_; }

    void SetDetails(const QString &s) { details_ = s; }

    const LoadData &GetLoadData() const { return load_data_; }

    void SetLoadData(const LoadData &p) { load_data_ = p; }

    const QVector<Node*> &GetLoadedNodes() const { return loaded_nodes_; }

    void SetLoadedNodes(const QVector<Node*> &n) { loaded_nodes_ = n; }

  private:
    ResultCode code_;

    QString details_;

    LoadData load_data_;

    QVector<Node*> loaded_nodes_;

  };

  class SaveData
  {
  public:
    SaveData(Project *project, const QString &filename = QString())
    {
      project_ = project;
      filename_ = filename;
    }

    Project *GetProject() const
    {
      return project_;
    }

    const QString &GetFilename() const
    {
      return filename_;
    }

    const QVector<Node*> &GetOnlySerializeNodes() const { return only_serialize_nodes_; }
    void SetOnlySerializeNodes(const QVector<Node*> &only) { only_serialize_nodes_ = only; }
    void SetOnlySerializeNodesAndResolveGroups(QVector<Node*> only);

    const std::vector<TimelineMarker*> &GetOnlySerializeMarkers() const { return only_serialize_markers_; }
    void SetOnlySerializeMarkers(const std::vector<TimelineMarker*> &only) { only_serialize_markers_ = only; }

    const SerializedProperties &GetProperties() const { return properties_; }
    void SetProperties(const SerializedProperties &p) { properties_ = p; }

  private:
    Project *project_;

    QString filename_;

    QVector<Node*> only_serialize_nodes_;

    SerializedProperties properties_;

    std::vector<TimelineMarker*> only_serialize_markers_;

  };

  static void Initialize();

  static void Destroy();

  static Result Load(Project *project, const QString &filename, const QString &type);
  static Result Load(Project *project, QXmlStreamReader *read_device, const QString &type);
  static Result Paste(const QString &type);

  static Result Save(const SaveData &data, const QString &type);
  static Result Save(QXmlStreamWriter *write_device, const SaveData &data, const QString &type);
  static Result Copy(const SaveData &data, const QString &type);

protected:
  virtual LoadData Load(Project *project, QXmlStreamReader *reader, void *reserved) const = 0;

  virtual void Save(QXmlStreamWriter *writer, const SaveData &data, void *reserved) const {}

  virtual uint Version() const = 0;

  bool IsCancelled() const;

private:
  static Result LoadWithSerializerVersion(uint version, Project *project, QXmlStreamReader *reader);

  static QVector<ProjectSerializer*> instances_;

};

}

#endif // PROJECTSERIALIZER_H
