/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#ifndef PREFERENCESDISKTAB_H
#define PREFERENCESDISKTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

#include "dialog/configbase/configdialogbase.h"
#include "render/diskmanager.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/integerslider.h"
#include "widget/path/pathwidget.h"

namespace olive
{

class PreferencesDiskTab : public ConfigDialogBaseTab {
	Q_OBJECT
public:
	PreferencesDiskTab();

	virtual bool Validate() override;

	virtual void Accept(MultiUndoCommand *command) override;

private:
	PathWidget *disk_cache_location_;

	FloatSlider *cache_ahead_slider_;

	FloatSlider *cache_behind_slider_;

	DiskCacheFolder *default_disk_cache_folder_;

	IntegerSlider *proxy_width_slider_;
	IntegerSlider *proxy_height_slider_;
	IntegerSlider *proxy_crf_slider_;
	QComboBox *proxy_preset_combo_;
	QCheckBox *proxy_include_audio_checkbox_;
	QLineEdit *proxy_ffmpeg_path_edit_;
};

}

#endif // PREFERENCESDISKTAB_H
