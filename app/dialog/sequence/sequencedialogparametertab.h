/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
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

#ifndef SEQUENCEDIALOGPARAMETERTAB_H
#define SEQUENCEDIALOGPARAMETERTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QList>
#include <QSpinBox>

#include "node/project/sequence/sequence.h"
#include "sequencepreset.h"
#include "widget/slider/integerslider.h"
#include "widget/standardcombos/standardcombos.h"

namespace olive
{

class SequenceDialogParameterTab : public QWidget {
	Q_OBJECT
public:
	SequenceDialogParameterTab(Sequence *sequence, QWidget *parent = nullptr);

	int GetSelectedVideoWidth() const
	{
		return width_slider_->GetValue();
	}

	int GetSelectedVideoHeight() const
	{
		return height_slider_->GetValue();
	}

	rational GetSelectedVideoFrameRate() const
	{
		return framerate_combo_->GetFrameRate();
	}

	rational GetSelectedVideoPixelAspect() const
	{
		return pixelaspect_combo_->GetPixelAspectRatio();
	}

	VideoParams::Interlacing GetSelectedVideoInterlacingMode() const
	{
		return interlacing_combo_->GetInterlaceMode();
	}

	int GetSelectedAudioSampleRate() const
	{
		return audio_sample_rate_field_->GetSampleRate();
	}

	[[nodiscard]] uint64_t GetSelectedAudioChannelLayout() const
	{
		return audio_channels_field_->GetChannelLayout();
	}

	int GetSelectedPreviewResolution() const
	{
		return preview_resolution_field_->GetDivider();
	}

	PixelFormat GetSelectedPreviewFormat() const
	{
		return preview_format_field_->GetPixelFormat();
	}

	bool GetSelectedPreviewAutoCache() const
	{
		//return preview_autocache_field_->isChecked();
		// TEMP: Disable sequence auto-cache, wanna see if clip cache supersedes it.
		return false;
	}

public slots:
	void PresetChanged(const SequencePreset &preset);

signals:
	void SaveParametersAsPreset(const SequencePreset &preset);

private:
	IntegerSlider *width_slider_;

	IntegerSlider *height_slider_;

	FrameRateComboBox *framerate_combo_;

	PixelAspectRatioComboBox *pixelaspect_combo_;

	InterlacedComboBox *interlacing_combo_;

	SampleRateComboBox *audio_sample_rate_field_;

	ChannelLayoutComboBox *audio_channels_field_;

	VideoDividerComboBox *preview_resolution_field_;

	QLabel *preview_resolution_label_;

	PixelFormatComboBox *preview_format_field_;

	QCheckBox *preview_autocache_field_;

private slots:
	void SavePresetClicked();

	void UpdatePreviewResolutionLabel();
};

}

#endif // SEQUENCEDIALOGPARAMETERTAB_H
