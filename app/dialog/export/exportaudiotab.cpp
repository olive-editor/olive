#include "exportaudiotab.h"

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>

ExportAudioTab::ExportAudioTab(QWidget* parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGridLayout* layout = new QGridLayout();
  outer_layout->addLayout(layout);

  int row = 0;

  layout->addWidget(new QLabel(tr("Codec:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Channel Layout:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Format:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  outer_layout->addStretch();
}
