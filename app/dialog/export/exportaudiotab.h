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

#ifndef EXPORTAUDIOTAB_H
#define EXPORTAUDIOTAB_H

#include <QComboBox>
#include <QWidget>

#include "common/define.h"
#include "codec/exportformat.h"
#include "widget/slider/integerslider.h"
#include "widget/standardcombos/standardcombos.h"

namespace olive {

class ExportAudioTab : public QWidget
{
  Q_OBJECT
public:
  ExportAudioTab(QWidget* parent = nullptr);

  int SetFormat(ExportFormat::Format format);

  QComboBox* codec_combobox() const
  {
    return codec_combobox_;
  }

  SampleRateComboBox* sample_rate_combobox() const
  {
    return sample_rate_combobox_;
  }

  ChannelLayoutComboBox* channel_layout_combobox() const
  {
    return channel_layout_combobox_;
  }

  IntegerSlider* bit_rate_slider() const
  {
    return bit_rate_slider_;
  }

private:
  QComboBox* codec_combobox_;
  SampleRateComboBox* sample_rate_combobox_;
  ChannelLayoutComboBox* channel_layout_combobox_;
  IntegerSlider* bit_rate_slider_;

};

}

#endif // EXPORTAUDIOTAB_H
