#include "imagesection.h"

#include <QGridLayout>
#include <QLabel>

ImageSection::ImageSection(QWidget* parent) :
  CodecSection(parent)
{
  QGridLayout* layout = new QGridLayout(this);
  layout->setMargin(0);

  int row = 0;

  layout->addWidget(new QLabel(tr("Image Sequence:")), row, 0);

  image_sequence_checkbox_ = new QCheckBox();
  layout->addWidget(new QCheckBox(), row, 1);
}

QCheckBox *ImageSection::image_sequence_checkbox() const
{
  return image_sequence_checkbox_;
}
