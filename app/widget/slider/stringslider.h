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

#ifndef STRINGSLIDER_H
#define STRINGSLIDER_H

#include "base/sliderbase.h"

namespace olive {

class StringSlider : public SliderBase
{
  Q_OBJECT
public:
  StringSlider(QWidget* parent = nullptr);

  void SetDragMultiplier(const double& d) = delete;

  QString GetValue() const;

  void SetValue(const QString& v);

  void SetDefaultValue(const QString& v);

signals:
  void ValueChanged(const QString& str);

protected:
  virtual QString ValueToString(const QVariant& value) const override;

  virtual QVariant StringToValue(const QString &s, bool *ok) const override;

  virtual void ValueSignalEvent(const QVariant &value) override;

};

}

#endif // STRINGSLIDER_H
