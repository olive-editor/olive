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

#ifndef ABSTRACTPARAMWIDGET_H
#define ABSTRACTPARAMWIDGET_H

#include <QObject>

#include "node/value.h"
#include "widget/slider/base/numericsliderbase.h"

namespace olive {

class AbstractParamWidget : public QObject
{
  Q_OBJECT
public:
  explicit AbstractParamWidget(QObject *parent = nullptr);

  virtual ~AbstractParamWidget() override;

  const std::vector<QWidget *> &GetWidgets() const { return widgets_; }

  virtual void Initialize(QWidget *parent, size_t channels) = 0;

  virtual void SetValue(const value_t &val) = 0;

  virtual void SetDefaultValue(const value_t &val) {}

  virtual void SetProperty(const QString &key, const value_t &value);

  virtual void SetTimebase(const rational &timebase){}

signals:
  void ChannelValueChanged(size_t channel, const olive::value_t::component_t &val);

  void SliderDragged(NumericSliderBase *slider, size_t track);

protected:
  void AddWidget(QWidget *w) { widgets_.push_back(w); }

protected slots:
  void ArbitrateSliders();

private:
  std::vector<QWidget *> widgets_;

};

}

#endif // ABSTRACTPARAMWIDGET_H
