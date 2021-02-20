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
  QHBoxLayout* video_layout = new QHBoxLayout(video_group);
  video_section_ = new VideoParamEdit();
  video_section_->SetParameterMask(Sequence::kVideoParamEditMask);
  connect(video_section_, &VideoParamEdit::Changed, this, &SequenceDialogParameterTab::UpdatePreviewResolutionLabel);
  video_layout->addWidget(video_section_);
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
  layout->addWidget(preview_group);

  // Set values based on input sequence
  video_section_->SetVideoParams(sequence->video_params());
  preview_resolution_field_->SetDivider(sequence->video_params().divider());
  preview_format_field_->SetPixelFormat(sequence->video_params().format());
  audio_sample_rate_field_->SetSampleRate(sequence->audio_params().sample_rate());
  audio_channels_field_->SetChannelLayout(sequence->audio_params().channel_layout());

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
  video_section_->SetWidth(preset.width());
  video_section_->SetHeight(preset.height());
  video_section_->SetFrameRate(preset.frame_rate());
  video_section_->SetPixelAspectRatio(preset.pixel_aspect());
  video_section_->SetInterlaceMode(preset.interlacing());
  audio_sample_rate_field_->SetSampleRate(preset.sample_rate());
  audio_channels_field_->SetChannelLayout(preset.channel_layout());
  preview_resolution_field_->SetDivider(preset.preview_divider());
  preview_format_field_->SetPixelFormat(preset.preview_format());
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
                               GetSelectedPreviewFormat()});
}

void SequenceDialogParameterTab::UpdatePreviewResolutionLabel()
{
  VideoParams test_param(video_section_->GetWidth(),
                         video_section_->GetHeight(),
                         VideoParams::kFormatInvalid,
                         VideoParams::kInternalChannelCount,
                         rational(1),
                         VideoParams::kInterlaceNone,
                         preview_resolution_field_->currentData().toInt());

  preview_resolution_label_->setText(tr("(%1x%2)").arg(QString::number(test_param.effective_width()),
                                                       QString::number(test_param.effective_height())));
}

}
