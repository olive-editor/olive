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

#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include <QObject>

#include "project/item/footage/stream.h"
#include "task/conform/conform.h"
#include "task/index/index.h"

OLIVE_NAMESPACE_ENTER

class IndexManager : public QObject
{
  Q_OBJECT
public:
  IndexManager() = default;

  static void CreateInstance();
  static IndexManager* instance();
  static void DestroyInstance();

  bool IsIndexing(StreamPtr stream) const;
  bool IsConforming(AudioStreamPtr stream, const AudioRenderingParams& params) const;

public slots:
  void StartIndexingStream(OLIVE_NAMESPACE::StreamPtr stream);
  void StartConformingStream(OLIVE_NAMESPACE::AudioStreamPtr stream, OLIVE_NAMESPACE::AudioRenderingParams params);

signals:
  void StreamIndexUpdated(Stream* stream);
  void StreamConformAppended(Stream* stream, OLIVE_NAMESPACE::AudioRenderingParams params);

private:
  static IndexManager* instance_;

  struct IndexPair {
    StreamPtr stream;
    IndexTask* task;
  };

  struct ConformPair {
    StreamPtr stream;
    AudioRenderingParams params;
    ConformTask* task;
  };

  QList<IndexPair> indexing_;

  QList<ConformPair> conforming_;

private slots:
  void IndexTaskFinished();

  void StreamIndexUpdatedEvent();

  void StreamConformAppendedEvent(const AudioRenderingParams& params);

};

OLIVE_NAMESPACE_EXIT

#endif // INDEXMANAGER_H
