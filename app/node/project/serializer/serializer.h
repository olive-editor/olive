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

#ifndef PROJECTSERIALIZER_H
#define PROJECTSERIALIZER_H

#include <vector>

#include "common/define.h"
#include "node/project.h"
#include "typeserializer.h"

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
  enum LoadType
  {
    kProject,
    kOnlyNodes,
    kOnlyMarkers,
    kOnlyKeyframes
  };

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
  using SerializedKeyframes = QHash<QString, QVector<NodeKeyframe*> >;

  class LoadData
  {
  public:
    LoadData() = default;

    SerializedProperties properties;

    std::vector<TimelineMarker*> markers;

    SerializedKeyframes keyframes;

    MainWindowLayoutInfo layout;

    QVector<Node*> nodes;

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

  private:
    ResultCode code_;

    QString details_;

    LoadData load_data_;

  };

  class SaveData
  {
  public:
    SaveData(LoadType type, Project *project = nullptr, const QString &filename = QString())
    {
      type_ = type;
      project_ = project;
      filename_ = filename;
    }

    Project *GetProject() const { return project_; }
    void SetProject(Project *p) { project_ = p; }

    const QString &GetFilename() const { return filename_; }
    void SetFilename(const QString &s) { filename_ = s; }

    LoadType type() const { return type_; }

    const MainWindowLayoutInfo &GetLayout() const { return layout_; }
    void SetLayout(const MainWindowLayoutInfo &layout) { layout_ = layout; }

    const QVector<Node*> &GetOnlySerializeNodes() const { return only_serialize_nodes_; }
    void SetOnlySerializeNodes(const QVector<Node*> &only) { only_serialize_nodes_ = only; }
    void SetOnlySerializeNodesAndResolveGroups(QVector<Node*> only);

    const std::vector<TimelineMarker*> &GetOnlySerializeMarkers() const { return only_serialize_markers_; }
    void SetOnlySerializeMarkers(const std::vector<TimelineMarker*> &only) { only_serialize_markers_ = only; }

    const std::vector<NodeKeyframe*> &GetOnlySerializeKeyframes() const { return only_serialize_keyframes_; }
    void SetOnlySerializeKeyframes(const std::vector<NodeKeyframe*> &only) { only_serialize_keyframes_ = only; }

    const SerializedProperties &GetProperties() const { return properties_; }
    void SetProperties(const SerializedProperties &p) { properties_ = p; }

  private:
    LoadType type_;

    Project *project_;

    QString filename_;

    MainWindowLayoutInfo layout_;

    QVector<Node*> only_serialize_nodes_;

    SerializedProperties properties_;

    std::vector<TimelineMarker*> only_serialize_markers_;

    std::vector<NodeKeyframe*> only_serialize_keyframes_;

  };

  static void Initialize();

  static void Destroy();

  static Result Load(Project *project, const QString &filename, LoadType load_type);
  static Result Load(Project *project, QXmlStreamReader *read_device, LoadType load_type);
  static Result Paste(LoadType load_type);

  static Result Save(const SaveData &data, bool compress);
  static Result Save(QXmlStreamWriter *write_device, const SaveData &data);
  static Result Copy(const SaveData &data);

  static bool CheckCompressedID(QFile *file);

protected:
  virtual LoadData Load(Project *project, QXmlStreamReader *reader, LoadType load_type, void *reserved) const = 0;

  virtual void Save(QXmlStreamWriter *writer, const SaveData &data, void *reserved) const {}

  virtual uint Version() const = 0;

  bool IsCancelled() const;

private:
  static Result LoadWithSerializerVersion(uint version, Project *project, QXmlStreamReader *reader, LoadType load_type);

  static QVector<ProjectSerializer*> instances_;

};

}

#endif // PROJECTSERIALIZER_H
