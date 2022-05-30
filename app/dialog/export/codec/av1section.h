/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef AV1SECTION_H
#define AV1SECTION_H

#include <QSlider>
#include <QStackedWidget>
#include <QComboBox>

#include "codecsection.h"
#include "widget/slider/floatslider.h"

namespace olive {

class AV1CRFSection : public QWidget
{
  Q_OBJECT
public:
  AV1CRFSection(int default_crf, QWidget* parent = nullptr);

  int GetValue() const;

  static const int kDefaultAV1CRF = 30;

private:
  static const int kMinimumCRF = 0;
  static const int kMaximumCRF = 63;

  QSlider* crf_slider_;

};

class AV1Section : public CodecSection
{
  Q_OBJECT
public:
  enum CompressionMethod {
    kConstantRateFactor,
  };

  AV1Section(QWidget* parent = nullptr);
  AV1Section(int default_crf, QWidget* parent);

  virtual void AddOpts(EncodingParams* params) override;

private:
  QStackedWidget* compression_method_stack_;

  AV1CRFSection* crf_section_;

  QComboBox *preset_combobox_;
};

}

#endif // AV1SECTION_H
