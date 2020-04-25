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

#ifndef AUDIOBACKEND_H
#define AUDIOBACKEND_H

#include <QFile>

#include "../audiorenderbackend.h"

OLIVE_NAMESPACE_ENTER

class AudioBackend : public AudioRenderBackend
{
  Q_OBJECT
public:
  AudioBackend(QObject* parent = nullptr);

  virtual ~AudioBackend() override;

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual void ConnectWorkerToThis(RenderWorker* worker) override;

private slots:
  void ThreadCompletedCache(NodeDependency dep, NodeValueTable data, qint64 job_time);

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOBACKEND_H
