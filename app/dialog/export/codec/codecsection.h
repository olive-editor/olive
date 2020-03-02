#ifndef CODECSECTION_H
#define CODECSECTION_H

#include <QWidget>

#include "codec/encoder.h"

class CodecSection : public QWidget
{
public:
  CodecSection(QWidget* parent = nullptr);

  virtual void AddOpts(EncodingParams* params){}

};

#endif // CODECSECTION_H
