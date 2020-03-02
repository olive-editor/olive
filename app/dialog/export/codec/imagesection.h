#ifndef IMAGESECTION_H
#define IMAGESECTION_H

#include <QCheckBox>

#include "codecsection.h"

class ImageSection : public CodecSection
{
public:
  ImageSection(QWidget* parent = nullptr);

  QCheckBox* image_sequence_checkbox() const;

private:
  QCheckBox* image_sequence_checkbox_;

};

#endif // IMAGESECTION_H
