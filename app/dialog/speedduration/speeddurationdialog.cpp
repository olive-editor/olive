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

#include "speeddurationdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>

namespace olive {

SpeedDurationDialog::SpeedDurationDialog(const QVector<ClipBlock *> &clips, QWidget *parent) :
  QDialog(parent)
{
  QGridLayout *layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Speed:")), row, 0);

  FloatSlider *speed_slider_ = new FloatSlider();
  layout->addWidget(speed_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Duration:")), row, 0);

  dur_slider_ = new RationalSlider();
  layout->addWidget(dur_slider_, row, 1);

  row++;

  ripple_box_ = new QCheckBox();
  layout->addWidget(ripple_box_, row, 0, 1, 2);

  row++;

  QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btns->setCenterButtons(true);
  connect(btns, &QDialogButtonBox::accepted, this, &SpeedDurationDialog::accept);
  connect(btns, &QDialogButtonBox::rejected, this, &SpeedDurationDialog::reject);
  layout->addWidget(btns, row, 0, 1, 2);
}

}
