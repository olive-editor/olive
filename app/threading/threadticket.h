/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef RENDERTICKET_H
#define RENDERTICKET_H

#include <QDateTime>
#include <QMutex>
#include <QWaitCondition>

#include "codec/frame.h"
#include "codec/samplebuffer.h"
#include "common/timerange.h"
#include "node/output/viewer/viewer.h"

namespace olive {

class RenderTicket : public QObject
{
  Q_OBJECT
public:
  RenderTicket();

  qint64 GetJobTime() const
  {
    return job_time_;
  }

  void SetJobTime()
  {
    job_time_ = QDateTime::currentMSecsSinceEpoch();
  }

  void WaitForFinished();

  QVariant Get();

  bool HasStarted();

  bool IsFinished(bool lock = true);

  bool WasCancelled();

  QMutex* lock()
  {
    return &lock_;
  }

  void Start();

  void Finish(QVariant result, bool cancelled);

  void Cancel();

signals:
  void Finished();

private:
  bool started_;

  bool finished_;

  bool cancelled_;

  QVariant result_;

  QMutex lock_;

  QWaitCondition wait_;

  qint64 job_time_;

};

using RenderTicketPtr = std::shared_ptr<RenderTicket>;

}

Q_DECLARE_METATYPE(olive::RenderTicketPtr)

#endif // RENDERTICKET_H
