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

#include "dnxhdsection.h"

#include <QGridLayout>
#include <QLabel>

namespace olive {

DNxHDSection::DNxHDSection(QWidget *parent) :
  CodecSection(parent)
{
  QGridLayout *layout = new QGridLayout(this);

  layout->setMargin(0);

  int row = 0;

  layout->addWidget(new QLabel(tr("preset:")), row, 0);

  preset_combobox_ = new QComboBox();

  /* Correspond to the following indexes for FFmpeg
   *
   *-profile           <int>        E..V....... (from 0 to 5) (default dnxhd)
   *  dnxhd           0            E..V.......
   *  dnxhr_444       5            E..V.......
   *  dnxhr_hqx       4            E..V.......
   *  dnxhr_hq        3            E..V.......
   *  dnxhr_sq        2            E..V.......
   *  dnxhr_lb        1            E..V.......
   *
   */

  preset_combobox_->addItem(tr("dnxhd"));
  preset_combobox_->addItem(tr("dnxhr_lb"));
  preset_combobox_->addItem(tr("dnxhr_sq"));
  preset_combobox_->addItem(tr("dnxhr_hq"));
  preset_combobox_->addItem(tr("dnxhr_hqx"));
  preset_combobox_->addItem(tr("dnxhr_444"));

  //Default to "medium"
  preset_combobox_->setCurrentIndex(3);

  layout->addWidget(preset_combobox_, row, 1);
}

void DNxHDSection::AddOpts(EncodingParams *params)
{
  params->set_video_option(QStringLiteral("profile"), QString::number(preset_combobox_->currentIndex()));
}

} 
