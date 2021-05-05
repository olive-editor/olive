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

#ifndef TIMESLIDER_H
#define TIMESLIDER_H

#include "common/rational.h"
#include "integerslider.h"

namespace olive {

class TimeSlider : public IntegerSlider
{
  Q_OBJECT
public:
  TimeSlider(QWidget* parent = nullptr);

public slots:
  void SetTimebase(const rational& timebase);

protected:
  virtual QString ValueToString(const QVariant& v) const override;

  virtual QVariant StringToValue(const QString& s, bool* ok) const override;

private:
  rational timebase_;

};

}

#endif // TIMESLIDER_H
