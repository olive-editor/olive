/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef NUMERICSLIDERBASE_H
#define NUMERICSLIDERBASE_H

#include "sliderbase.h"

namespace olive {

class NumericSliderBase : public SliderBase
{
  Q_OBJECT
public:
  NumericSliderBase(QWidget *parent = nullptr);

  void SetLadderElementCount(int b)
  {
    ladder_element_count_ = b;
  }

  void SetDragMultiplier(const double& d);

  void SetOffset(const InternalType& v);

  bool IsDragging() const;

protected:
  const InternalType& GetOffset() const
  {
    return offset_;
  }

  virtual InternalType AdjustDragDistanceInternal(const InternalType &start, const double &drag) const;

  void SetMinimumInternal(const InternalType& v);

  void SetMaximumInternal(const InternalType& v);

  virtual bool ValueGreaterThan(const InternalType& lhs, const InternalType& rhs) const;

  virtual bool ValueLessThan(const InternalType& lhs, const InternalType& rhs) const;

  virtual bool CanSetValue() const override;

private:
  bool UsingLadders() const;

  virtual InternalType AdjustValue(const InternalType& value) const override;

  SliderLadder* drag_ladder_;

  int ladder_element_count_;

  bool dragged_;

  bool has_min_;
  InternalType min_value_;

  bool has_max_;
  InternalType max_value_;

  double dragged_diff_;

  InternalType drag_start_value_;

  InternalType offset_;

  double drag_multiplier_;

  bool setting_drag_value_;

  /**
   * @brief An effects slider somewhere is being dragged
   */
  static bool effects_slider_is_being_dragged_;

private slots:
  void LabelPressed();

  void RepositionLadder();

  void LadderDragged(int value, double multiplier);

  void LadderReleased();

};

}

#endif // NUMERICSLIDERBASE_H
