/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef RATIONALSLIDERPARAMWIDGET_H
#define RATIONALSLIDERPARAMWIDGET_H

#include "numericsliderparamwidget.h"

namespace olive {

class RationalSliderParamWidget : public NumericSliderParamWidget
{
public:
  explicit RationalSliderParamWidget(QObject *parent = nullptr);

  virtual void Initialize(QWidget *parent, size_t channels) override;

  virtual void SetValue(const value_t &val) override;

  virtual void SetDefaultValue(const value_t &val) override;

  virtual void SetProperty(const QString &key, const value_t &val) override;

  virtual void SetTimebase(const rational &timebase) override;

};

}

#endif // RATIONALSLIDERPARAMWIDGET_H
