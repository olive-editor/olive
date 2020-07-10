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

#ifndef RENDERTICKET_H
#define RENDERTICKET_H

#include <QMutex>
#include <QWaitCondition>

#include "codec/frame.h"
#include "codec/samplebuffer.h"
#include "common/timerange.h"

OLIVE_NAMESPACE_ENTER

class RenderTicket : public QObject
{
  Q_OBJECT
public:
  enum Type {
    kTypeHash,
    kTypeVideo,
    kTypeAudio
  };

  RenderTicket(Type type, const QVariant& time);

  const QVariant& GetTime() const
  {
    return time_;
  }

  Type GetType() const
  {
    return type_;
  }

  void WaitForFinished();

  QVariant Get();

  bool IsFinished(bool lock = true);

  bool WasCancelled();

  QMutex* lock()
  {
    return &lock_;
  }

  void Finish(QVariant result);

  void Cancel();

signals:
  void Finished();

private:
  bool finished_;

  bool cancelled_;

  QVariant result_;

  QMutex lock_;

  QWaitCondition wait_;

  QVariant time_;

  Type type_;

};

using RenderTicketPtr = std::shared_ptr<RenderTicket>;

OLIVE_NAMESPACE_EXIT

#endif // RENDERTICKET_H
