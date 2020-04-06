/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

OLIVE_NAMESPACE_ENTER

class ExportAudioTab : public QWidget
{
public:
  ExportAudioTab(QWidget* parent = nullptr);

  QComboBox* codec_combobox() const;
  QComboBox* sample_rate_combobox() const;
  QComboBox* channel_layout_combobox() const;

  void set_sample_rate(int rate);
  void set_channel_layout(uint64_t layout);

private:
  QComboBox* codec_combobox_;
  QComboBox* sample_rate_combobox_;
  QComboBox* channel_layout_combobox_;

  QList<int> sample_rates_;
  QList<uint64_t> channel_layouts_;
};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTAUDIOTAB_H
