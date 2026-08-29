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

#ifndef PROXYDIALOG_H
#define PROXYDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTreeWidget>

#include "codec/proxymanager.h"
#include "node/project/footage/footage.h"
#include "widget/slider/integerslider.h"

namespace olive
{

class ProxyDialog : public QDialog {
	Q_OBJECT
public:
	ProxyDialog(QWidget *parent, const QVector<Footage *> &footage = {});

	virtual void accept() override;

	int ProxyWidth() const;

	int ProxyHeight() const;

	int ProxyCRF() const;

	QString ProxyPreset() const;

	bool ProxyIncludeAudio() const;

	QString FFmpegPath() const;

	void SetProxyWidth(int width);

	void SetProxyHeight(int height);

	void SetProxyCRF(int crf);

	void SetProxyPreset(const QString &preset);

	void SetProxyIncludeAudio(bool include_audio);

	void SetFFmpegPath(const QString &path);

private:
	ProxyManager::ProxyParams CurrentParams() const;

	void SaveGlobalSettings();

	void RefreshFootageList();

	QVector<Footage *> footage_;

	QTreeWidget *footage_tree_;

	QCheckBox *custom_params_checkbox_;

	IntegerSlider *width_slider_;

	IntegerSlider *height_slider_;

	IntegerSlider *crf_slider_;

	QComboBox *preset_combo_;

	QCheckBox *include_audio_checkbox_;

	QLineEdit *ffmpeg_path_edit_;

private slots:
	void GenerateProxies();

	void DeleteProxies();

	void BrowseForFFmpeg();
};

}

#endif // PROXYDIALOG_H
