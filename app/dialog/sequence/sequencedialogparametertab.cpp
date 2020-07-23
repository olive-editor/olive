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
  video_frame_rate_field_ = new QComboBox();
  video_layout->addWidget(video_frame_rate_field_, row, 1);
  layout->addWidget(video_group);

  row = 0;

  // Set up audio section
  QGroupBox* audio_group = new QGroupBox();
  audio_group->setTitle(tr("Audio"));
  QGridLayout* audio_layout = new QGridLayout(audio_group);
  audio_layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);
  audio_sample_rate_field_ = new QComboBox();
  audio_layout->addWidget(audio_sample_rate_field_, row, 1);
  row++;
  audio_layout->addWidget(new QLabel(tr("Channels:")), row, 0);
  audio_channels_field_ = new QComboBox();
  audio_layout->addWidget(audio_channels_field_, row, 1);
  layout->addWidget(audio_group);

  row = 0;

  // Set up preview section
  QGroupBox* preview_group = new QGroupBox();
  preview_group->setTitle(tr("Preview"));
  QGridLayout* preview_layout = new QGridLayout(preview_group);
  preview_layout->addWidget(new QLabel(tr("Resolution:")), row, 0);
  preview_resolution_field_ = new QComboBox();
  preview_layout->addWidget(preview_resolution_field_, row, 1);
  preview_resolution_label_ = new QLabel();
  preview_layout->addWidget(preview_resolution_label_, row, 2);
  row++;
  preview_layout->addWidget(new QLabel(tr("Format:")), row, 0);
  preview_format_field_ = new QComboBox();
  preview_layout->addWidget(preview_format_field_, row, 1, 1, 2);
  layout->addWidget(preview_group);

  // Set up available frame rates
  frame_rate_list_ = Core::SupportedFrameRates();
  foreach (const rational& fr, frame_rate_list_) {
    video_frame_rate_field_->addItem(Core::FrameRateToString(fr));
  }

  // Set up available sample rates
  sample_rate_list_ = Core::SupportedSampleRates();
  foreach (const int& sr, sample_rate_list_) {
    audio_sample_rate_field_->addItem(Core::SampleRateToString(sr));
  }

  // Set up available channel layouts
  channel_layout_list_ = Core::SupportedChannelLayouts();
  foreach (const uint64_t& ch_layout, channel_layout_list_) {
    audio_channels_field_->addItem(Core::ChannelLayoutToString(ch_layout), QVariant::fromValue(ch_layout));
  }

  // Set up preview dividers
  divider_list_ = Core::SupportedDividers();
  foreach (int d, divider_list_) {
    QString name;

    if (d == 1) {
      name = tr("Full");
    } else {
      name = tr("1/%1").arg(d);
    }

    preview_resolution_field_->addItem(name);
  }
  connect(preview_resolution_field_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &SequenceDialogParameterTab::UpdatePreviewResolutionLabel);

  // Set up preview formats
  for (int i=0;i<PixelFormat::PIX_FMT_COUNT;i++) {
    PixelFormat::Format pix_fmt = static_cast<PixelFormat::Format>(i);

    // We always render with an alpha channel internally
    if (PixelFormat::FormatHasAlphaChannel(pix_fmt)
        && PixelFormat::FormatIsFloat(pix_fmt)) {
      preview_format_field_->addItem(PixelFormat::GetName(pix_fmt));

      preview_format_list_.append(pix_fmt);
    }
  }

  // Set values based on input sequence
  video_width_field_->SetValue(sequence->video_params().width());
  video_height_field_->SetValue(sequence->video_params().height());

  int frame_rate_index = frame_rate_list_.indexOf(sequence->video_params().time_base().flipped());
  video_frame_rate_field_->setCurrentIndex(frame_rate_index);

  int sample_rate_index = sample_rate_list_.indexOf(sequence->audio_params().sample_rate());
  audio_sample_rate_field_->setCurrentIndex(sample_rate_index);

  for (int i=0;i<audio_channels_field_->count();i++) {
    if (audio_channels_field_->itemData(i).toULongLong() == sequence->audio_params().channel_layout()) {
      audio_channels_field_->setCurrentIndex(i);
      break;
    }
  }

  preview_resolution_field_->setCurrentIndex(divider_list_.indexOf(sequence->video_params().divider()));

  preview_format_field_->setCurrentIndex(preview_format_list_.indexOf(sequence->video_params().format()));

  layout->addStretch();

  QPushButton* save_preset_btn = new QPushButton(tr("Save Preset"));
  connect(save_preset_btn, &QPushButton::clicked, this, &SequenceDialogParameterTab::SavePresetClicked);
  layout->addWidget(save_preset_btn);

  UpdatePreviewResolutionLabel();
}

int SequenceDialogParameterTab::GetSelectedVideoWidth() const
{
  return video_width_field_->GetValue();
}

int SequenceDialogParameterTab::GetSelectedVideoHeight() const
{
  return video_height_field_->GetValue();
}

const rational &SequenceDialogParameterTab::GetSelectedVideoFrameRate() const
{
  return frame_rate_list_.at(video_frame_rate_field_->currentIndex());
}

int SequenceDialogParameterTab::GetSelectedAudioSampleRate() const
{
  return sample_rate_list_.at(audio_sample_rate_field_->currentIndex());
}

uint64_t SequenceDialogParameterTab::GetSelectedAudioChannelLayout() const
{
  return audio_channels_field_->currentData().toULongLong();
}

int SequenceDialogParameterTab::GetSelectedPreviewResolution() const
{
  return divider_list_.at(preview_resolution_field_->currentIndex());
}

PixelFormat::Format SequenceDialogParameterTab::GetSelectedPreviewFormat() const
{
  return preview_format_list_.at(preview_format_field_->currentIndex());
}

void SequenceDialogParameterTab::PresetChanged(const SequencePreset &preset)
{
  video_width_field_->SetValue(preset.width());
  video_height_field_->SetValue(preset.height());
  video_frame_rate_field_->setCurrentIndex(frame_rate_list_.indexOf(preset.frame_rate()));
  audio_sample_rate_field_->setCurrentIndex(sample_rate_list_.indexOf(preset.sample_rate()));
  audio_channels_field_->setCurrentIndex(channel_layout_list_.indexOf(preset.channel_layout()));
  preview_resolution_field_->setCurrentIndex(divider_list_.indexOf(preset.preview_divider()));
  preview_format_field_->setCurrentIndex(preview_format_list_.indexOf(preset.preview_format()));
}

void SequenceDialogParameterTab::SavePresetClicked()
{
  emit SaveParametersAsPreset({QString(),
                               static_cast<int>(video_width_field_->GetValue()),
                               static_cast<int>(video_height_field_->GetValue()),
                               frame_rate_list_.at(video_frame_rate_field_->currentIndex()),
                               sample_rate_list_.at(audio_sample_rate_field_->currentIndex()),
                               channel_layout_list_.at(audio_channels_field_->currentIndex()),
                               divider_list_.at(preview_resolution_field_->currentIndex()),
                               preview_format_list_.at(preview_format_field_->currentIndex())});
}

void SequenceDialogParameterTab::UpdatePreviewResolutionLabel()
{
  VideoParams test_param(video_width_field_->GetValue(),
                         video_height_field_->GetValue(),
                         PixelFormat::PIX_FMT_INVALID,
                         divider_list_.at(preview_resolution_field_->currentIndex()));

  preview_resolution_label_->setText(tr("(%1x%2)").arg(QString::number(test_param.effective_width()),
                                                       QString::number(test_param.effective_height())));
}

OLIVE_NAMESPACE_EXIT
