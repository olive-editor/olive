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

#ifndef CUSTOMCACHETASK_H
#define CUSTOMCACHETASK_H

#include <QMutex>
#include <QWaitCondition>

#include "task/task.h"

namespace olive {

class CustomCacheTask : public Task
{
  Q_OBJECT
public:
  CustomCacheTask(const QString &sequence_name);

  void Finish();

signals:
  void Cancelled();

protected:
  virtual bool Run() override;

  virtual void CancelEvent() override;

private:
  QMutex mutex_;

  QWaitCondition wait_cond_;

  bool cancelled_through_finish_;

};

}

#endif // CUSTOMCACHETASK_H
