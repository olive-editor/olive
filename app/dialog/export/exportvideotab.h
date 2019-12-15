#ifndef EXPORTVIDEOTAB_H
#define EXPORTVIDEOTAB_H

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

#include "common/rational.h"
#include "widget/slider/integerslider.h"

class ExportVideoTab : public QWidget
{
  Q_OBJECT
public:
  ExportVideoTab(QWidget* parent = nullptr);

  QComboBox* codec_combobox() const;

  IntegerSlider* width_slider() const;
  IntegerSlider* height_slider() const;
  QCheckBox* maintain_aspect_checkbox() const;
  QComboBox* scaling_method_combobox() const;

  void set_frame_rate(const rational& frame_rate);

private:
  QWidget* SetupResolutionSection();
  QWidget* SetupColorSection();
  QWidget* SetupCodecSection();

  QComboBox* codec_combobox_;
  QComboBox* frame_rate_combobox_;
  QCheckBox* maintain_aspect_checkbox_;
  QComboBox* scaling_method_combobox_;

  IntegerSlider* width_slider_;
  IntegerSlider* height_slider_;

  QList<rational> frame_rates_;

};

#endif // EXPORTVIDEOTAB_H
