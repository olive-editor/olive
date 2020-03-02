#ifndef H264SECTION_H
#define H264SECTION_H

#include <QSlider>
#include <QStackedWidget>

#include "codecsection.h"
#include "widget/slider/floatslider.h"

class H264CRFSection : public QWidget
{
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

#endif // H264SECTION_H
