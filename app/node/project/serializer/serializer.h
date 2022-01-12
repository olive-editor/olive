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

  virtual ~ProjectSerializer(){};

  DISABLE_COPY_MOVE(ProjectSerializer)

  enum ResultCode {
    kSuccess,
    kProjectTooOld,
    kProjectTooNew,
    kUnknownVersion,
    kFileError,
    kXmlError,
    kOverwriteError,
  };

  class Result
  {
  public:
    Result(const ResultCode &code) :
      code_(code)
    {}

    bool operator==(const ResultCode &code) { return code_ == code; }
    bool operator!=(const ResultCode &code) { return code_ != code; }

    const ResultCode &code() const
    {
      return code_;
    }

    const QString &GetDetails() const
    {
      return details_;
    }

    void SetDetails(const QString &s)
    {
      details_ = s;
    }

  private:
    ResultCode code_;

    QString details_;

  };

  static void Initialize();

  static void Destroy();

  static Result Load(Project *project, const QString &filename);
  static Result Load(Project *project, QXmlStreamReader *read_device);

  static Result Save(Project *project, const QString &filename, const QVector<Node*> &only = QVector<Node*>());
  static Result Save(Project *project, QXmlStreamWriter *write_device, const QString &filename, const QVector<Node*> &only = QVector<Node*>());
  static Result Save(Project *project, QXmlStreamWriter *write_device, const QVector<Node*> &only = QVector<Node*>())
  {
    return Save(project, write_device, QString(), only);
  }

protected:
  virtual void Load(Project *project, QXmlStreamReader *reader, void *reserved) const = 0;

  virtual void Save(Project *project, QXmlStreamWriter *writer, const QVector<Node*> &only, void *reserved) const {}

  virtual uint Version() const = 0;

  bool IsCancelled() const;

private:
  static QVector<ProjectSerializer*> instances_;

};

}

#endif // PROJECTSERIALIZER_H
