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
  QList<int> sample_rates = Core::SupportedSampleRates();
  foreach (const int& sr, sample_rates) {
    sample_rate_combobox_->addItem(Core::SampleRateToString(sr));
  }
  layout->addWidget(sample_rate_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Channel Layout:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

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
