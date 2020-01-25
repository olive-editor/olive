#include "exportaudiotab.h"

#include <QGridLayout>
#include <QLabel>

#include "core.h"

ExportAudioTab::ExportAudioTab(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  layout->addWidget(codec_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);

  sample_rate_combobox_ = new QComboBox();
  sample_rates_ = Core::SupportedSampleRates();
  foreach (const int& sr, sample_rates_) {
    sample_rate_combobox_->addItem(Core::SampleRateToString(sr), sr);
  }
  layout->addWidget(sample_rate_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Channel Layout:")), row, 0);

  channel_layout_combobox_ = new QComboBox();
  channel_layouts_ = Core::SupportedChannelLayouts();
  foreach (const uint64_t& ch_layout, channel_layouts_) {
    channel_layout_combobox_->addItem(Core::ChannelLayoutToString(ch_layout), QVariant::fromValue(ch_layout));
  }
  layout->addWidget(channel_layout_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Format:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  outer_layout->addStretch();
}

QComboBox *ExportAudioTab::codec_combobox() const
{
  return codec_combobox_;
}

QComboBox *ExportAudioTab::sample_rate_combobox() const
{
  return sample_rate_combobox_;
}

QComboBox *ExportAudioTab::channel_layout_combobox() const
{
  return channel_layout_combobox_;
}

void ExportAudioTab::set_sample_rate(int rate)
{
  sample_rate_combobox_->setCurrentIndex(sample_rates_.indexOf(rate));
}

void ExportAudioTab::set_channel_layout(uint64_t layout)
{
  channel_layout_combobox_->setCurrentIndex(channel_layouts_.indexOf(layout));
}
