#include "sequencedialogparametertab.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "core.h"

namespace olive {

SequenceDialogParameterTab::SequenceDialogParameterTab(Sequence* sequence, QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  int row = 0;

  // Set up video section
  QGroupBox* video_group = new QGroupBox();
  video_group->setTitle(tr("Video"));
  QGridLayout *video_layout = new QGridLayout(video_group);
  video_layout->addWidget(new QLabel(tr("Width:")), row, 0);
  width_slider_ = new IntegerSlider();
  width_slider_->SetMinimum(0);
  video_layout->addWidget(width_slider_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Height:")), row, 0);
  height_slider_ = new IntegerSlider();
  height_slider_->SetMinimum(0);
  video_layout->addWidget(height_slider_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Frame Rate:")), row, 0);
  framerate_combo_ = new FrameRateComboBox();
  video_layout->addWidget(framerate_combo_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Pixel Aspect Ratio:")), row, 0);
  pixelaspect_combo_ = new PixelAspectRatioComboBox();
  video_layout->addWidget(pixelaspect_combo_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Interlacing:")), row, 0);
  interlacing_combo_ = new InterlacedComboBox();
  video_layout->addWidget(interlacing_combo_, row, 1);
  layout->addWidget(video_group);

  row = 0;

  // Set up audio section
  QGroupBox* audio_group = new QGroupBox();
  audio_group->setTitle(tr("Audio"));
  QGridLayout* audio_layout = new QGridLayout(audio_group);
  audio_layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);
  audio_sample_rate_field_ = new SampleRateComboBox();
  audio_layout->addWidget(audio_sample_rate_field_, row, 1);
  row++;
  audio_layout->addWidget(new QLabel(tr("Channels:")), row, 0);
  audio_channels_field_ = new ChannelLayoutComboBox();
  audio_layout->addWidget(audio_channels_field_, row, 1);
  layout->addWidget(audio_group);

  row = 0;

  // Set up preview section
  QGroupBox* preview_group = new QGroupBox();
  preview_group->setTitle(tr("Preview"));
  QGridLayout* preview_layout = new QGridLayout(preview_group);
  preview_layout->addWidget(new QLabel(tr("Resolution:")), row, 0);
  preview_resolution_field_ = new VideoDividerComboBox();
  preview_layout->addWidget(preview_resolution_field_, row, 1);
  preview_resolution_label_ = new QLabel();
  preview_layout->addWidget(preview_resolution_label_, row, 2);
  row++;
  preview_layout->addWidget(new QLabel(tr("Quality:")), row, 0);
  preview_format_field_ = new PixelFormatComboBox(true);
  preview_layout->addWidget(preview_format_field_, row, 1, 1, 2);
  row++;
  preview_layout->addWidget(new QLabel(tr("Auto-Cache:")), row, 0);
  preview_autocache_field_ = new QCheckBox();
  preview_layout->addWidget(preview_autocache_field_, row, 1);
  layout->addWidget(preview_group);

  // Set values based on input sequence
  VideoParams vp = sequence->GetVideoParams();
  AudioParams ap = sequence->GetAudioParams();
  width_slider_->SetValue(vp.width());
  height_slider_->SetValue(vp.height());
  framerate_combo_->SetFrameRate(vp.time_base().flipped());
  pixelaspect_combo_->SetPixelAspectRatio(vp.pixel_aspect_ratio());
  interlacing_combo_->SetInterlaceMode(vp.interlacing());
  preview_resolution_field_->SetDivider(vp.divider());
  preview_format_field_->SetPixelFormat(vp.format());
  preview_autocache_field_->setChecked(sequence->GetVideoAutoCacheEnabled());
  audio_sample_rate_field_->SetSampleRate(ap.sample_rate());
  audio_channels_field_->SetChannelLayout(ap.channel_layout());

  connect(preview_resolution_field_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &SequenceDialogParameterTab::UpdatePreviewResolutionLabel);

  layout->addStretch();

  QPushButton* save_preset_btn = new QPushButton(tr("Save Preset"));
  connect(save_preset_btn, &QPushButton::clicked, this, &SequenceDialogParameterTab::SavePresetClicked);
  layout->addWidget(save_preset_btn);

  UpdatePreviewResolutionLabel();
}

void SequenceDialogParameterTab::PresetChanged(const SequencePreset &preset)
{
  width_slider_->SetValue(preset.width());
  height_slider_->SetValue(preset.height());
  framerate_combo_->SetFrameRate(preset.frame_rate());
  pixelaspect_combo_->SetPixelAspectRatio(preset.pixel_aspect());
  interlacing_combo_->SetInterlaceMode(preset.interlacing());
  audio_sample_rate_field_->SetSampleRate(preset.sample_rate());
  audio_channels_field_->SetChannelLayout(preset.channel_layout());
  preview_resolution_field_->SetDivider(preset.preview_divider());
  preview_format_field_->SetPixelFormat(preset.preview_format());
  preview_autocache_field_->setChecked(preset.preview_autocache());
}

void SequenceDialogParameterTab::SavePresetClicked()
{
  emit SaveParametersAsPreset({QString(),
                               GetSelectedVideoWidth(),
                               GetSelectedVideoHeight(),
                               GetSelectedVideoFrameRate(),
                               GetSelectedVideoPixelAspect(),
                               GetSelectedVideoInterlacingMode(),
                               GetSelectedAudioSampleRate(),
                               GetSelectedAudioChannelLayout(),
                               GetSelectedPreviewResolution(),
                               GetSelectedPreviewFormat(),
                               GetSelectedPreviewAutoCache()});
}

void SequenceDialogParameterTab::UpdatePreviewResolutionLabel()
{
  VideoParams test_param(GetSelectedVideoWidth(),
                         GetSelectedVideoHeight(),
                         VideoParams::kFormatInvalid,
                         VideoParams::kInternalChannelCount,
                         rational(1),
                         VideoParams::kInterlaceNone,
                         preview_resolution_field_->currentData().toInt());

  preview_resolution_label_->setText(tr("(%1x%2)").arg(QString::number(test_param.effective_width()),
                                                       QString::number(test_param.effective_height())));
}

}
