/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2026 Oak Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "proxydialog.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "config/config.h"
#include "node/project.h"

namespace olive
{

ProxyDialog::ProxyDialog(QWidget *parent, const QVector<Footage *> &footage)
	: QDialog(parent)
	, footage_(footage)
	, footage_tree_(nullptr)
	, custom_params_checkbox_(nullptr)
{
	setWindowTitle(tr("Proxy Settings"));

	const ProxyManager::ProxyParams params =
		ProxyManager::ProxyParamsFromConfig();

	QVBoxLayout *layout = new QVBoxLayout(this);

	if (!footage_.isEmpty()) {
		QGroupBox *footage_group = new QGroupBox(tr("Selected Footage"));
		layout->addWidget(footage_group);
		QVBoxLayout *footage_layout = new QVBoxLayout(footage_group);

		footage_tree_ = new QTreeWidget();
		footage_tree_->setHeaderLabels({ tr("Footage"), tr("Proxy State") });
		footage_tree_->setRootIsDecorated(false);
		footage_layout->addWidget(footage_tree_);
		RefreshFootageList();

		custom_params_checkbox_ =
			new QCheckBox(tr("Use custom settings for selected footage"));
		bool any_custom = false;
		for (const Footage *item : footage_) {
			if (item->has_custom_proxy_params()) {
				any_custom = true;
				break;
			}
		}
		custom_params_checkbox_->setChecked(any_custom);
		footage_layout->addWidget(custom_params_checkbox_);
	}

	QGroupBox *settings_group = new QGroupBox(tr("Global Proxy Settings"));
	layout->addWidget(settings_group);
	QGridLayout *settings_layout = new QGridLayout(settings_group);

	int row = 0;

	settings_layout->addWidget(new QLabel(tr("Proxy Width:")), row, 0);
	width_slider_ = new IntegerSlider();
	width_slider_->SetMinimum(160);
	width_slider_->SetMaximum(4096);
	width_slider_->SetValue(params.width);
	settings_layout->addWidget(width_slider_, row, 1);

	settings_layout->addWidget(new QLabel(tr("Proxy Height:")), row, 2);
	height_slider_ = new IntegerSlider();
	height_slider_->SetMinimum(120);
	height_slider_->SetMaximum(2160);
	height_slider_->SetValue(params.height);
	settings_layout->addWidget(height_slider_, row, 3);

	row++;

	settings_layout->addWidget(new QLabel(tr("Proxy CRF:")), row, 0);
	crf_slider_ = new IntegerSlider();
	crf_slider_->SetMinimum(0);
	crf_slider_->SetMaximum(51);
	crf_slider_->SetValue(params.crf);
	settings_layout->addWidget(crf_slider_, row, 1);

	settings_layout->addWidget(new QLabel(tr("Proxy Preset:")), row, 2);
	preset_combo_ = new QComboBox();
	const QStringList presets = {
		QStringLiteral("ultrafast"), QStringLiteral("superfast"),
		QStringLiteral("veryfast"),	 QStringLiteral("faster"),
		QStringLiteral("fast"),		 QStringLiteral("medium"),
		QStringLiteral("slow"),		 QStringLiteral("slower"),
		QStringLiteral("veryslow"),
	};
	for (const QString &preset : presets) {
		preset_combo_->addItem(preset);
	}
	preset_combo_->setCurrentText(params.preset);
	settings_layout->addWidget(preset_combo_, row, 3);

	row++;

	include_audio_checkbox_ = new QCheckBox(tr("Include audio in proxies"));
	include_audio_checkbox_->setChecked(params.include_audio);
	settings_layout->addWidget(include_audio_checkbox_, row, 0, 1, 2);

	row++;

	settings_layout->addWidget(new QLabel(tr("ffmpeg Executable:")), row, 0);
	ffmpeg_path_edit_ = new QLineEdit(OLIVE_CONFIG("FFmpegPath").toString());
	ffmpeg_path_edit_->setPlaceholderText(tr("Auto-detect"));
	settings_layout->addWidget(ffmpeg_path_edit_, row, 1);

	QPushButton *ffmpeg_browse_btn = new QPushButton(tr("Browse..."));
	connect(ffmpeg_browse_btn, &QPushButton::clicked, this,
			&ProxyDialog::BrowseForFFmpeg);
	settings_layout->addWidget(ffmpeg_browse_btn, row, 2);

	QHBoxLayout *button_layout = new QHBoxLayout();
	layout->addLayout(button_layout);

	if (!footage_.isEmpty()) {
		QPushButton *generate_btn = new QPushButton(tr("Generate Proxies"));
		connect(generate_btn, &QPushButton::clicked, this,
				&ProxyDialog::GenerateProxies);
		button_layout->addWidget(generate_btn);

		QPushButton *delete_btn = new QPushButton(tr("Delete Proxies"));
		connect(delete_btn, &QPushButton::clicked, this,
				&ProxyDialog::DeleteProxies);
		button_layout->addWidget(delete_btn);
	}

	button_layout->addStretch();

	QPushButton *close_btn = new QPushButton(tr("Close"));
	connect(close_btn, &QPushButton::clicked, this, &ProxyDialog::accept);
	button_layout->addWidget(close_btn);
}

void ProxyDialog::accept()
{
	SaveGlobalSettings();

	if (!footage_.isEmpty()) {
		for (Footage *item : footage_) {
			if (custom_params_checkbox_->isChecked()) {
				item->SetCustomProxyParams(CurrentParams());
			} else {
				item->ClearCustomProxyParams();
			}
		}
	}

	QDialog::accept();
}

int ProxyDialog::ProxyWidth() const
{
	return static_cast<int>(width_slider_->GetValue());
}

int ProxyDialog::ProxyHeight() const
{
	return static_cast<int>(height_slider_->GetValue());
}

int ProxyDialog::ProxyCRF() const
{
	return static_cast<int>(crf_slider_->GetValue());
}

QString ProxyDialog::ProxyPreset() const
{
	return preset_combo_->currentText();
}

bool ProxyDialog::ProxyIncludeAudio() const
{
	return include_audio_checkbox_->isChecked();
}

QString ProxyDialog::FFmpegPath() const
{
	return ffmpeg_path_edit_->text();
}

void ProxyDialog::SetProxyWidth(int width)
{
	width_slider_->SetValue(width);
}

void ProxyDialog::SetProxyHeight(int height)
{
	height_slider_->SetValue(height);
}

void ProxyDialog::SetProxyCRF(int crf)
{
	crf_slider_->SetValue(crf);
}

void ProxyDialog::SetProxyPreset(const QString &preset)
{
	preset_combo_->setCurrentText(preset);
}

void ProxyDialog::SetProxyIncludeAudio(bool include_audio)
{
	include_audio_checkbox_->setChecked(include_audio);
}

void ProxyDialog::SetFFmpegPath(const QString &path)
{
	ffmpeg_path_edit_->setText(path);
}

ProxyManager::ProxyParams ProxyDialog::CurrentParams() const
{
	ProxyManager::ProxyParams params = ProxyManager::ProxyParamsFromConfig();
	params.width = static_cast<int>(width_slider_->GetValue());
	params.height = static_cast<int>(height_slider_->GetValue());
	params.crf = static_cast<int>(crf_slider_->GetValue());
	params.preset = preset_combo_->currentText();
	params.include_audio = include_audio_checkbox_->isChecked();
	return params;
}

void ProxyDialog::SaveGlobalSettings()
{
	OLIVE_CONFIG("ProxyWidth") = static_cast<int>(width_slider_->GetValue());
	OLIVE_CONFIG("ProxyHeight") = static_cast<int>(height_slider_->GetValue());
	OLIVE_CONFIG("ProxyCRF") = static_cast<int>(crf_slider_->GetValue());
	OLIVE_CONFIG("ProxyPreset") = preset_combo_->currentText();
	OLIVE_CONFIG("ProxyIncludeAudio") = include_audio_checkbox_->isChecked();
	OLIVE_CONFIG("FFmpegPath") = ffmpeg_path_edit_->text().trimmed();
}

void ProxyDialog::RefreshFootageList()
{
	if (!footage_tree_) {
		return;
	}

	footage_tree_->clear();
	for (const Footage *item : footage_) {
		QTreeWidgetItem *tree_item = new QTreeWidgetItem(footage_tree_);
		tree_item->setText(0, item->filename());
		QString state = ProxyManager::ProxyStateToString(item->proxy_state());
		if (item->has_custom_proxy_params()) {
			state = tr("%1 (custom settings)").arg(state);
		}
		tree_item->setText(1, state);
	}
}

void ProxyDialog::GenerateProxies()
{
	if (!ProxyManager::instance()) {
		qWarning() << "ProxyDialog::GenerateProxies: ProxyManager unavailable";
		return;
	}

	for (Footage *item : footage_) {
		const VideoParams video = item->GetFirstEnabledVideoStream();
		if (!video.is_valid()) {
			qWarning()
				<< "ProxyDialog::GenerateProxies: skipping item with no valid video stream"
				<< item->filename();
			continue;
		}

		const ProxyManager::ProxyParams params =
			custom_params_checkbox_->isChecked() ? CurrentParams()
												 : item->GetEffectiveProxyParams();
		const ProxyManager::Proxy proxy =
			ProxyManager::instance()->GetOrStartProxy(
				item->project()->cache_path(), item->filename(),
				video.stream_index(), params);
		item->SetProxy(proxy.filename, proxy.state, video.stream_index(),
					   params.version, true);
		item->InvalidateAll(Footage::kFilenameInput);
	}

	RefreshFootageList();
}

void ProxyDialog::DeleteProxies()
{
	for (Footage *item : footage_) {
		if (item->proxy_path().isEmpty()) {
			continue;
		}

		QFile::remove(item->proxy_path());
		QFile::remove(ProxyManager::GetWorkingProxyFilename(item->proxy_path()));
		item->ClearProxy();
		item->InvalidateAll(Footage::kFilenameInput);
	}

	RefreshFootageList();
}

void ProxyDialog::BrowseForFFmpeg()
{
	const QString file =
		QFileDialog::getOpenFileName(this, tr("Select ffmpeg Executable"));
	if (!file.isEmpty()) {
		ffmpeg_path_edit_->setText(file);
	}
}

}
