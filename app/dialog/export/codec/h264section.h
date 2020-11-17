/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef H264SECTION_H
#define H264SECTION_H

#include <QSlider>
#include <QStackedWidget>

#include "codecsection.h"
#include "widget/slider/floatslider.h"

OLIVE_NAMESPACE_ENTER

class H264CRFSection : public QWidget
{
  Q_OBJECT
public:
  H264CRFSection(QWidget* parent = nullptr);

  int GetValue() const;

private:
  static const int kMinimumCRF = 0;

  static const int kDefaultCRF = 23;

  static const int kMaximumCRF = 51;

  QSlider* crf_slider_;

};

class H264BitRateSection : public QWidget
{
  Q_OBJECT
public:
  H264BitRateSection(QWidget* parent = nullptr);

  /**
   * @brief Get user-selected target bit rate (returns in BITS)
   */
  int64_t GetTargetBitRate() const;

  /**
   * @brief Get user-selected maximum bit rate (returns in BITS)
   */
  int64_t GetMaximumBitRate() const;

private:
  FloatSlider* target_rate_;

  FloatSlider* max_rate_;

};

class H264FileSizeSection : public QWidget
{
  Q_OBJECT
public:
  H264FileSizeSection(QWidget* parent = nullptr);

  /**
   * @brief Returns file size in BITS
   */
  int64_t GetFileSize() const;

private:
  FloatSlider* file_size_;

};

class H264Section : public CodecSection
{
  Q_OBJECT
public:
  enum CompressionMethod {
    kConstantRateFactor,
    kTargetBitRate,
    kTargetFileSize
  };

  H264Section(QWidget* parent = nullptr);

  virtual void AddOpts(EncodingParams* params) override;

private:
  QStackedWidget* compression_method_stack_;

  H264CRFSection* crf_section_;

  H264BitRateSection* bitrate_section_;

  H264FileSizeSection* filesize_section_;

};

OLIVE_NAMESPACE_EXIT

#endif // H264SECTION_H
