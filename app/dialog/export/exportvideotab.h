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
  QCheckBox* image_sequence_checkbox() const;

  void show_image_sequence_section(bool visible);
  void set_frame_rate(const rational& frame_rate);

  QString CurrentOCIODisplay();
  QString CurrentOCIOView();
  QString CurrentOCIOLook();

signals:
  void DisplayChanged(const QString& display);
  void ViewChanged(const QString& view);
  void LookChanged(const QString& look);

private:
  QWidget* SetupResolutionSection();
  QWidget* SetupColorSection();
  QWidget* SetupCodecSection();

  QComboBox* codec_combobox_;
  QComboBox* frame_rate_combobox_;
  QCheckBox* maintain_aspect_checkbox_;
  QComboBox* scaling_method_combobox_;
  QCheckBox* image_sequence_checkbox_;
  QLabel* image_sequence_label_;

  IntegerSlider* width_slider_;
  IntegerSlider* height_slider_;

  QComboBox* display_combobox_;
  QComboBox* views_combobox_;
  QComboBox* looks_combobox_;

  QList<rational> frame_rates_;

private slots:
  void ColorDisplayChanged();
  void ColorViewChanged();
  void ColorLookChanged();

};

#endif // EXPORTVIDEOTAB_H
