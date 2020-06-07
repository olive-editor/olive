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

  // Set up video section
  QGroupBox* video_group = new QGroupBox();
  video_group->setTitle(tr("Video"));
  QGridLayout* video_layout = new QGridLayout(video_group);
  video_layout->addWidget(new QLabel(tr("Width:")), 0, 0);
  video_width_field_ = new QSpinBox();
  video_width_field_->setMaximum(99999);
  video_layout->addWidget(video_width_field_, 0, 1);
  video_layout->addWidget(new QLabel(tr("Height:")), 1, 0);
  video_height_field_ = new QSpinBox();
  video_height_field_->setMaximum(99999);
  video_layout->addWidget(video_height_field_, 1, 1);
  video_layout->addWidget(new QLabel(tr("Frame Rate:")), 2, 0);
  video_frame_rate_field_ = new QComboBox();
  video_layout->addWidget(video_frame_rate_field_, 2, 1);
  layout->addWidget(video_group);

  // Set up audio section
  QGroupBox* audio_group = new QGroupBox();
  audio_group->setTitle(tr("Audio"));
  QGridLayout* audio_layout = new QGridLayout(audio_group);
  audio_layout->addWidget(new QLabel(tr("Sample Rate:")), 0, 0);
  audio_sample_rate_field_ = new QComboBox();
  // FIXME: No sample rate made
  audio_layout->addWidget(audio_sample_rate_field_, 0, 1);
  audio_layout->addWidget(new QLabel(tr("Channels:")), 1, 0);
  audio_channels_field_ = new QComboBox();
  // FIXME: No channels made
  audio_layout->addWidget(audio_channels_field_, 1, 1);
  layout->addWidget(audio_group);

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

  // Set values based on input sequence
  video_width_field_->setValue(sequence->video_params().width());
  video_height_field_->setValue(sequence->video_params().height());

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

  layout->addStretch();

  QPushButton* save_preset_btn = new QPushButton(tr("Save Preset"));
  connect(save_preset_btn, &QPushButton::clicked, this, &SequenceDialogParameterTab::SavePresetClicked);
  layout->addWidget(save_preset_btn);
}

int SequenceDialogParameterTab::GetSelectedVideoWidth() const
{
  return video_width_field_->value();
}

int SequenceDialogParameterTab::GetSelectedVideoHeight() const
{
  return video_height_field_->value();
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

void SequenceDialogParameterTab::PresetChanged(const SequencePreset &preset)
{
  video_width_field_->setValue(preset.width);
  video_height_field_->setValue(preset.height);
  video_frame_rate_field_->setCurrentIndex(frame_rate_list_.indexOf(preset.frame_rate));
  audio_sample_rate_field_->setCurrentIndex(sample_rate_list_.indexOf(preset.sample_rate));
  audio_channels_field_->setCurrentIndex(channel_layout_list_.indexOf(preset.channel_layout));
}

void SequenceDialogParameterTab::SavePresetClicked()
{
  emit SaveParametersAsPreset({QString(),
                               video_width_field_->value(),
                               video_height_field_->value(),
                               frame_rate_list_.at(video_frame_rate_field_->currentIndex()),
                               sample_rate_list_.at(audio_sample_rate_field_->currentIndex()),
                               channel_layout_list_.at(audio_channels_field_->currentIndex())});
}

OLIVE_NAMESPACE_EXIT
