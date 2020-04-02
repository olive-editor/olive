#ifndef EXPORTVIDEOTAB_H
#define EXPORTVIDEOTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

#include "common/rational.h"
#include "dialog/color/colorspacechooser.h"
#include "dialog/export/codec/h264section.h"
#include "dialog/export/codec/imagesection.h"
#include "render/colormanager.h"
#include "widget/slider/integerslider.h"

class ExportVideoTab : public QWidget
{
  Q_OBJECT
public:
  ExportVideoTab(ColorManager* color_manager, QWidget* parent = nullptr);

  enum ScalingMethod {
    kFit,
    kStretch,
    kCrop
  };

  QComboBox* codec_combobox() const;

  IntegerSlider* width_slider() const;
  IntegerSlider* height_slider() const;
  QCheckBox* maintain_aspect_checkbox() const;
  QComboBox* scaling_method_combobox() const;

  const rational& frame_rate() const;
  void set_frame_rate(const rational& frame_rate);

  QString CurrentOCIODisplay();
  QString CurrentOCIOView();
  QString CurrentOCIOLook();

  CodecSection* GetCodecSection() const;
  void SetCodecSection(CodecSection* section);
  ImageSection* image_section() const;
  H264Section* h264_section() const;

signals:
  void DisplayColorSpaceChanged(const QString& display, const QString& view, const QString& look);

private:
  QWidget* SetupResolutionSection();
  QWidget* SetupColorSection();
  QWidget* SetupCodecSection();

  QComboBox* codec_combobox_;
  QComboBox* frame_rate_combobox_;
  QCheckBox* maintain_aspect_checkbox_;
  QComboBox* scaling_method_combobox_;

  QStackedWidget* codec_stack_;
  ImageSection* image_section_;
  H264Section* h264_section_;

  ColorSpaceChooser* color_space_chooser_;

  IntegerSlider* width_slider_;
  IntegerSlider* height_slider_;

  QList<rational> frame_rates_;

  ColorManager* color_manager_;

private slots:
  void MaintainAspectRatioChanged(bool val);

};

#endif // EXPORTVIDEOTAB_H
