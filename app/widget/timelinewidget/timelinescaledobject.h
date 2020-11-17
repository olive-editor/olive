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

#ifndef TIMELINESCALEDOBJECT_H
#define TIMELINESCALEDOBJECT_H

#include <QWidget>

#include "common/rational.h"

OLIVE_NAMESPACE_ENTER

class TimelineScaledObject
{
public:
  TimelineScaledObject();
  virtual ~TimelineScaledObject() = default;

  void SetTimebase(const rational &timebase);

  const rational& timebase() const;
  const double& timebase_dbl() const;

  static rational SceneToTime(const double &x, const double& x_scale, const rational& timebase, bool round = false);

  const double& GetScale() const;

  void SetScale(const double& scale);

  void SetScaleFromDimensions(double viewport_width, double content_width);
  static double CalculateScaleFromDimensions(double viewport_sz, double content_sz);
  static double CalculatePaddingFromDimensionScale(double viewport_sz);

  double TimeToScene(const rational& time);
  rational SceneToTime(const double &x, bool round = false);

protected:
  virtual void TimebaseChangedEvent(const rational&){}

  virtual void ScaleChangedEvent(const double&){}

  void SetMaximumScale(const double& max);

  void SetMinimumScale(const double& min);

private:
  rational timebase_;

  double timebase_dbl_;

  double scale_;

  double min_scale_;

  double max_scale_;

  static const int kCalculateDimensionsPadding;

};

class TimelineScaledWidget : public QWidget, public TimelineScaledObject
{
  Q_OBJECT
public:
  TimelineScaledWidget(QWidget* parent = nullptr);
};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINESCALEDOBJECT_H
