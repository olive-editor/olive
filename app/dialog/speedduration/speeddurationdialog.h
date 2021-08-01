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

#ifndef SPEEDDURATIONDIALOG_H
#define SPEEDDURATIONDIALOG_H

#include <QCheckBox>
#include <QDialog>

#include "node/block/clip/clip.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/rationalslider.h"

namespace olive {

class SpeedDurationDialog : public QDialog
{
  Q_OBJECT
public:
  explicit SpeedDurationDialog(const QVector<ClipBlock*> &clips, QWidget *parent = nullptr);

signals:

private:
  FloatSlider *speed_slider_;

  RationalSlider *dur_slider_;

  QCheckBox *ripple_box_;

};

}

#endif // SPEEDDURATIONDIALOG_H
