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

#ifndef FUNCTIONTIMER_H
#define FUNCTIONTIMER_H

#include <QDateTime>
#include <QDebug>

#define TIME_THIS_FUNCTION FunctionTimer __f(__FUNCTION__)

class FunctionTimer {
public:
  FunctionTimer(const char* s)
  {
    name_ = s;
    time_ = QDateTime::currentMSecsSinceEpoch();
  }

  ~FunctionTimer()
  {
    qint64 elapsed = (QDateTime::currentMSecsSinceEpoch() - time_);

    if (elapsed > 1) {
      qDebug() << name_ << "took" << elapsed;
    }
  }

private:
  const char* name_;

  qint64 time_;

};

#endif // FUNCTIONTIMER_H
