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

#include "timescaledobject.h"

#include <cfloat>
#include <QtMath>

#include "common/clamp.h"

namespace olive {

const int TimeScaledObject::kCalculateDimensionsPadding = 10;

TimeScaledObject::TimeScaledObject() :
  scale_(1.0),
  min_scale_(0),
  max_scale_(DBL_MAX)
{

}

void TimeScaledObject::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();

  TimebaseChangedEvent(timebase);
}

const rational &TimeScaledObject::timebase() const
{
  return timebase_;
}

const double &TimeScaledObject::timebase_dbl() const
{
  return timebase_dbl_;
}

rational TimeScaledObject::SceneToTime(const double &x, const double &x_scale, const rational &timebase, bool round)
{
  double unscaled_time = x / x_scale / timebase.toDouble();

  // Adjust screen point by scale and timebase
  qint64 rounded_x_mvmt;

  if (round) {
    rounded_x_mvmt = qRound64(unscaled_time);
  } else {
    rounded_x_mvmt = qFloor(unscaled_time);
  }

  // Return a time in the timebase
  return rational(rounded_x_mvmt * timebase.numerator(), timebase.denominator());
}

double TimeScaledObject::TimeToScene(const rational &time)
{
  return time.toDouble() * scale_;
}

rational TimeScaledObject::SceneToTime(const double &x, bool round)
{
  return SceneToTime(x, scale_, timebase_, round);
}

void TimeScaledObject::SetMaximumScale(const double &max)
{
  max_scale_ = max;

  if (GetScale() > max_scale_) {
    SetScale(max_scale_);
  }
}

void TimeScaledObject::SetMinimumScale(const double &min)
{
  min_scale_ = min;

  if (GetScale() < min_scale_) {
    SetScale(min_scale_);
  }
}

const double& TimeScaledObject::GetScale() const
{
  return scale_;
}

void TimeScaledObject::SetScale(const double& scale)
{
  Q_ASSERT(scale > 0);

  scale_ = clamp(scale, min_scale_, max_scale_);

  ScaleChangedEvent(scale_);
}

void TimeScaledObject::SetScaleFromDimensions(double viewport_width, double content_width)
{
  SetScale(CalculateScaleFromDimensions(viewport_width, content_width));
}

double TimeScaledObject::CalculateScaleFromDimensions(double viewport_sz, double content_sz)
{
  return static_cast<double>(viewport_sz / kCalculateDimensionsPadding * (kCalculateDimensionsPadding-1)) / static_cast<double>(content_sz);
}

double TimeScaledObject::CalculatePaddingFromDimensionScale(double viewport_sz)
{
  return (viewport_sz / (kCalculateDimensionsPadding * 2));
}

TimelineScaledWidget::TimelineScaledWidget(QWidget *parent) :
  QWidget(parent)
{
}

}
