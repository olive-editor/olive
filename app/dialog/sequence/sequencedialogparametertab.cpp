#include "sequencedialogparametertab.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "core.h"

OLIVE_NAMESPACE_ENTER

SequenceDialogParameterTab::SequenceDialogParameterTab(Sequence* sequence, QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  int row = 0;

  // Set up video section
  QGroupBox* video_group = new QGroupBox();
  video_group->setTitle(tr("Video"));
  QGridLayout* video_layout = new QGridLayout(video_group);
  video_layout->addWidget(new QLabel(tr("Width:")), row, 0);
  video_width_field_ = new IntegerSlider();
  video_width_field_->SetMinimum(1);
  video_width_field_->SetMaximum(99999);
  connect(video_width_field_, &IntegerSlider::ValueChanged, this, &SequenceDialogParameterTab::UpdatePreviewResolutionLabel);
  video_layout->addWidget(video_width_field_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Height:")), row, 0);
  video_height_field_ = new IntegerSlider();
  video_height_field_->SetMinimum(1);
  video_height_field_->SetMaximum(99999);
  connect(video_height_field_, &IntegerSlider::ValueChanged, this, &SequenceDialogParameterTab::UpdatePreviewResolutionLabel);
  video_layout->addWidget(video_height_field_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Frame Rate:")), row, 0);
  video_frame_rate_field_ = new FrameRateComboBox();
  video_layout->addWidget(video_frame_rate_field_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Pixel Aspect Ratio:")), row, 0);
  video_pixel_aspect_field_ = new PixelAspectRatioComboBox();
  video_layout->addWidget(video_pixel_aspect_field_, row, 1);
  row++;
  video_layout->addWidget(new QLabel(tr("Interlacing:")));
  video_interlaced_field_ = new InterlacedComboBox();
  video_layout->addWidget(video_interlaced_field_, row, 1);
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
  preview_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  preview_format_field_ = new PixelFormatComboBox(true);
  preview_layout->addWidget(preview_format_field_, row, 1, 1, 2);
  layout->addWidget(preview_group);

  // Set values based on input sequence
  video_width_field_->SetValue(sequence->video_params().width());
  video_height_field_->SetValue(sequence->video_params().height());
  video_frame_rate_field_->SetFrameRate(sequence->video_params().time_base().flipped());
  video_pixel_aspect_field_->SetPixelAspectRatio(sequence->video_params().pixel_aspect_ratio());
  video_interlaced_field_->SetInterlaceMode(sequence->video_params().interlacing());
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
  video_width_field_->SetValue(preset.width());
  video_height_field_->SetValue(preset.height());
  video_frame_rate_field_->SetFrameRate(preset.frame_rate());
  video_pixel_aspect_field_->SetPixelAspectRatio(preset.pixel_aspect());
  video_interlaced_field_->SetInterlaceMode(preset.interlacing());
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
  VideoParams test_param(video_width_field_->GetValue(),
                         video_height_field_->GetValue(),
                         PixelFormat::PIX_FMT_INVALID,
                         rational(1),
                         VideoParams::kInterlaceNone,
                         preview_resolution_field_->currentData().toInt());

  preview_resolution_label_->setText(tr("(%1x%2)").arg(QString::number(test_param.effective_width()),
                                                       QString::number(test_param.effective_height())));
}

OLIVE_NAMESPACE_EXIT
