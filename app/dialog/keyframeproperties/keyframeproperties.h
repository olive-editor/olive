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

#ifndef KEYFRAMEPROPERTIESDIALOG_H
#define KEYFRAMEPROPERTIESDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QGroupBox>

#include "node/keyframe.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/timeslider.h"

namespace olive {

class KeyframePropertiesDialog : public QDialog
{
  Q_OBJECT
public:
  KeyframePropertiesDialog(const QList<NodeKeyframePtr>& keys, const rational& timebase, QWidget* parent = nullptr);

public slots:
  virtual void accept() override;

private:
  void SetUpBezierSlider(FloatSlider *slider, bool all_same, double value);

  const QList<NodeKeyframePtr>& keys_;

  rational timebase_;

  TimeSlider* time_slider_;

  QComboBox* type_select_;

  QGroupBox* bezier_group_;

  FloatSlider* bezier_in_x_slider_;

  FloatSlider* bezier_in_y_slider_;

  FloatSlider* bezier_out_x_slider_;

  FloatSlider* bezier_out_y_slider_;

private slots:
  void KeyTypeChanged(int index);

};

}

#endif // KEYFRAMEPROPERTIESDIALOG_H
