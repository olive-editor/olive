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

#include "preferencesdisktab.h"

#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

#include "common/filefunctions.h"
#include "config/config.h"

namespace olive
{

PreferencesDiskTab::PreferencesDiskTab()
{
	// Get default disk cache folder
	default_disk_cache_folder_ =
		DiskManager::instance()->GetDefaultCacheFolder();

	QVBoxLayout *outer_layout = new QVBoxLayout(this);

	QGroupBox *disk_management_group = new QGroupBox(tr("Disk Management"));
	outer_layout->addWidget(disk_management_group);

	QGridLayout *disk_management_layout =
		new QGridLayout(disk_management_group);

	int row = 0;

	disk_management_layout->addWidget(new QLabel(tr("Disk Cache Location:")),
									  row, 0);

	disk_cache_location_ =
		new PathWidget(default_disk_cache_folder_->GetPath());
	disk_management_layout->addWidget(disk_cache_location_, row, 1);

	row++;

	QPushButton *disk_cache_settings_btn =
		new QPushButton(tr("Disk Cache Settings"));
	connect(disk_cache_settings_btn, &QPushButton::clicked, this, [this]() {
		DiskManager::instance()->ShowDiskCacheSettingsDialog(
			disk_cache_location_->text(), this);
	});
	disk_management_layout->addWidget(disk_cache_settings_btn, row, 1);

	row++;

	QGroupBox *cache_behavior = new QGroupBox(tr("Cache Behavior"));
	outer_layout->addWidget(cache_behavior);
	QGridLayout *cache_behavior_layout = new QGridLayout(cache_behavior);

	row = 0;

	cache_behavior_layout->addWidget(new QLabel(tr("Cache Ahead:")), row, 0);

	cache_ahead_slider_ = new FloatSlider();
	cache_ahead_slider_->SetFormat(tr("%1 seconds"));
	cache_ahead_slider_->SetMinimum(0);
	cache_ahead_slider_->SetValue(
		OLIVE_CONFIG("DiskCacheAhead").value<rational>().toDouble());
	cache_behavior_layout->addWidget(cache_ahead_slider_, row, 1);

	cache_behavior_layout->addWidget(new QLabel(tr("Cache Behind:")), row, 2);

	cache_behind_slider_ = new FloatSlider();
	cache_behind_slider_->SetMinimum(0);
	cache_behind_slider_->SetFormat(tr("%1 seconds"));
	cache_behind_slider_->SetValue(
		OLIVE_CONFIG("DiskCacheBehind").value<rational>().toDouble());
	cache_behavior_layout->addWidget(cache_behind_slider_, row, 3);

	row++;

	QGroupBox *proxy_group = new QGroupBox(tr("Proxy Settings"));
	outer_layout->addWidget(proxy_group);
	QGridLayout *proxy_layout = new QGridLayout(proxy_group);

	int proxy_row = 0;

	proxy_layout->addWidget(new QLabel(tr("Proxy Width:")), proxy_row, 0);
	proxy_width_slider_ = new IntegerSlider();
	proxy_width_slider_->SetMinimum(160);
	proxy_width_slider_->SetMaximum(4096);
	proxy_width_slider_->SetValue(OLIVE_CONFIG("ProxyWidth").value<int>());
	proxy_layout->addWidget(proxy_width_slider_, proxy_row, 1);

	proxy_layout->addWidget(new QLabel(tr("Proxy Height:")), proxy_row, 2);
	proxy_height_slider_ = new IntegerSlider();
	proxy_height_slider_->SetMinimum(120);
	proxy_height_slider_->SetMaximum(2160);
	proxy_height_slider_->SetValue(OLIVE_CONFIG("ProxyHeight").value<int>());
	proxy_layout->addWidget(proxy_height_slider_, proxy_row, 3);

	proxy_row++;

	proxy_layout->addWidget(new QLabel(tr("Proxy CRF:")), proxy_row, 0);
	proxy_crf_slider_ = new IntegerSlider();
	proxy_crf_slider_->SetMinimum(0);
	proxy_crf_slider_->SetMaximum(51);
	proxy_crf_slider_->SetValue(OLIVE_CONFIG("ProxyCRF").value<int>());
	proxy_layout->addWidget(proxy_crf_slider_, proxy_row, 1);

	proxy_layout->addWidget(new QLabel(tr("Proxy Preset:")), proxy_row, 2);
	proxy_preset_combo_ = new QComboBox();
	const QStringList presets = {
		QStringLiteral("ultrafast"), QStringLiteral("superfast"),
		QStringLiteral("veryfast"),	 QStringLiteral("faster"),
		QStringLiteral("fast"),		 QStringLiteral("medium"),
		QStringLiteral("slow"),		 QStringLiteral("slower"),
		QStringLiteral("veryslow"),
	};
	for (const QString &preset : presets) {
		proxy_preset_combo_->addItem(preset);
	}
	proxy_preset_combo_->setCurrentText(OLIVE_CONFIG("ProxyPreset").toString());
	proxy_layout->addWidget(proxy_preset_combo_, proxy_row, 3);

	proxy_row++;

	proxy_include_audio_checkbox_ =
		new QCheckBox(tr("Include audio in proxies"));
	proxy_include_audio_checkbox_->setChecked(
		OLIVE_CONFIG("ProxyIncludeAudio").toBool());
	proxy_layout->addWidget(proxy_include_audio_checkbox_, proxy_row, 0, 1, 2);

	proxy_row++;

	proxy_layout->addWidget(new QLabel(tr("ffmpeg Executable:")), proxy_row,
							0);
	proxy_ffmpeg_path_edit_ =
		new QLineEdit(OLIVE_CONFIG("FFmpegPath").toString());
	proxy_ffmpeg_path_edit_->setPlaceholderText(tr("Auto-detect"));
	proxy_layout->addWidget(proxy_ffmpeg_path_edit_, proxy_row, 1);

	QPushButton *ffmpeg_browse_btn = new QPushButton(tr("Browse..."));
	connect(ffmpeg_browse_btn, &QPushButton::clicked, this, [this]() {
		const QString file = QFileDialog::getOpenFileName(
			this, tr("Select ffmpeg Executable"));
		if (!file.isEmpty()) {
			proxy_ffmpeg_path_edit_->setText(file);
		}
	});
	proxy_layout->addWidget(ffmpeg_browse_btn, proxy_row, 2);

	outer_layout->addStretch();
}

bool PreferencesDiskTab::Validate()
{
	if (disk_cache_location_->text() != default_disk_cache_folder_->GetPath()) {
		// Disk cache location is changing

		// Check if the user is okay with invalidating the current cache
		if (!DiskManager::ShowDiskCacheChangeConfirmationDialog(this)) {
			return false;
		}

		// Check validity of the new path
		if (!FileFunctions::DirectoryIsValid(disk_cache_location_->text())) {
			QMessageBox::critical(
				this, tr("Disk Cache"),
				tr("Failed to set disk cache location. Access was denied."));
			return false;
		}
	}

	return true;
}

void PreferencesDiskTab::Accept(MultiUndoCommand *command)
{
	Q_UNUSED(command)

	if (disk_cache_location_->text() != default_disk_cache_folder_->GetPath()) {
		default_disk_cache_folder_->SetPath(disk_cache_location_->text());
	}

	OLIVE_CONFIG("DiskCacheBehind") = QVariant::fromValue(
		rational::fromDouble(cache_behind_slider_->GetValue()));
	OLIVE_CONFIG("DiskCacheAhead") = QVariant::fromValue(
		rational::fromDouble(cache_ahead_slider_->GetValue()));

	OLIVE_CONFIG("ProxyWidth") =
		static_cast<int>(proxy_width_slider_->GetValue());
	OLIVE_CONFIG("ProxyHeight") =
		static_cast<int>(proxy_height_slider_->GetValue());
	OLIVE_CONFIG("ProxyCRF") = static_cast<int>(proxy_crf_slider_->GetValue());
	OLIVE_CONFIG("ProxyPreset") = proxy_preset_combo_->currentText();
	OLIVE_CONFIG("ProxyIncludeAudio") =
		proxy_include_audio_checkbox_->isChecked();
	OLIVE_CONFIG("FFmpegPath") = proxy_ffmpeg_path_edit_->text().trimmed();
}

}
