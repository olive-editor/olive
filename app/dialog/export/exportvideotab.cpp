#include "exportvideotab.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

#include "core.h"
#include "render/colormanager.h"

ExportVideoTab::ExportVideoTab(ColorManager* color_manager, QWidget *parent) :
  QWidget(parent),
  color_manager_(color_manager)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  outer_layout->addWidget(SetupResolutionSection());

  outer_layout->addWidget(SetupCodecSection());

  outer_layout->addWidget(SetupColorSection());

  outer_layout->addStretch();
}

QComboBox *ExportVideoTab::codec_combobox() const
{
  return codec_combobox_;
}

IntegerSlider *ExportVideoTab::width_slider() const
{
  return width_slider_;
}

IntegerSlider *ExportVideoTab::height_slider() const
{
  return height_slider_;
}

QCheckBox *ExportVideoTab::maintain_aspect_checkbox() const
{
  return maintain_aspect_checkbox_;
}

QComboBox *ExportVideoTab::scaling_method_combobox() const
{
  return scaling_method_combobox_;
}

QCheckBox *ExportVideoTab::image_sequence_checkbox() const
{
  return image_sequence_checkbox_;
}

void ExportVideoTab::show_image_sequence_section(bool visible)
{
  image_sequence_checkbox_->setVisible(visible);
  image_sequence_label_->setVisible(visible);
}

void ExportVideoTab::set_frame_rate(const rational &frame_rate)
{
  frame_rate_combobox_->setCurrentIndex(frame_rates_.indexOf(frame_rate));
}

QString ExportVideoTab::CurrentOCIODisplay()
{
  return display_combobox_->currentData().toString();
}

QString ExportVideoTab::CurrentOCIOView()
{
  return views_combobox_->currentData().toString();
}

QString ExportVideoTab::CurrentOCIOLook()
{
  return looks_combobox_->currentData().toString();
}

QWidget* ExportVideoTab::SetupResolutionSection()
{
  int row = 0;

  QGroupBox* resolution_group = new QGroupBox();
  resolution_group->setTitle(tr("Basic"));

  QGridLayout* layout = new QGridLayout(resolution_group);

  layout->addWidget(new QLabel(tr("Width:")), row, 0);

  width_slider_ = new IntegerSlider();
  layout->addWidget(width_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Height:")), row, 0);

  height_slider_ = new IntegerSlider();
  layout->addWidget(height_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Maintain Aspect Ratio:")), row, 0);

  maintain_aspect_checkbox_ = new QCheckBox();
  maintain_aspect_checkbox_->setChecked(true);
  layout->addWidget(maintain_aspect_checkbox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Scaling Method:")), row, 0);

  scaling_method_combobox_ = new QComboBox();
  scaling_method_combobox_->setEnabled(false);
  scaling_method_combobox_->addItem(tr("Fit"), kFit);
  scaling_method_combobox_->addItem(tr("Stretch"), kStretch);
  scaling_method_combobox_->addItem(tr("Crop"), kCrop);
  layout->addWidget(scaling_method_combobox_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Frame Rate:")), row, 0);

  frame_rate_combobox_ = new QComboBox();
  frame_rates_ = Core::SupportedFrameRates();
  foreach (const rational& fr, frame_rates_) {
    frame_rate_combobox_->addItem(Core::FrameRateToString(fr));
  }

  layout->addWidget(frame_rate_combobox_, row, 1);

  return resolution_group;
}

QWidget* ExportVideoTab::SetupColorSection()
{
  int row = 0;

  QGroupBox* color_group = new QGroupBox();
  color_group->setTitle(tr("Color Management"));

  QGridLayout* color_layout = new QGridLayout(color_group);

  color_layout->addWidget(new QLabel(tr("Display:")), row, 0);

  QStringList displays = color_manager_->ListAvailableDisplays();
  display_combobox_ = new QComboBox();
  foreach (const QString& display, displays) {
    display_combobox_->addItem(display, display);
  }
  connect(display_combobox_, SIGNAL(currentIndexChanged(int)), this, SLOT(ColorDisplayChanged()));
  color_layout->addWidget(display_combobox_, row, 1);

  row++;

  views_combobox_ = new QComboBox();
  color_layout->addWidget(new QLabel(tr("View:")), row, 0);
  color_layout->addWidget(views_combobox_, row, 1);

  row++;

  QStringList looks = color_manager_->ListAvailableLooks();
  looks_combobox_ = new QComboBox();
  looks_combobox_->addItem(tr("(None)"), QString());
  foreach (const QString& look, looks) {
    looks_combobox_->addItem(look, look);
  }
  color_layout->addWidget(new QLabel(tr("Look:")), row, 0);
  color_layout->addWidget(looks_combobox_, row, 1);

  row++;

  ColorDisplayChanged();

  return color_group;
}

QWidget *ExportVideoTab::SetupCodecSection()
{
  int row = 0;

  QGroupBox* codec_group = new QGroupBox();
  codec_group->setTitle(tr("Codec"));

  QGridLayout* codec_layout = new QGridLayout(codec_group);

  codec_layout->addWidget(new QLabel(tr("Codec:")), row, 0);

  codec_combobox_ = new QComboBox();
  codec_layout->addWidget(codec_combobox_, row, 1);

  row++;

  image_sequence_label_ = new QLabel(tr("Image Sequence:"));
  codec_layout->addWidget(image_sequence_label_, row, 0);

  image_sequence_checkbox_ = new QCheckBox();
  codec_layout->addWidget(image_sequence_checkbox_, row, 1);

  return codec_group;
}

void ExportVideoTab::ColorDisplayChanged()
{
  views_combobox_->clear();

  QStringList views = color_manager_->ListAvailableViews(display_combobox_->currentData().toString());
  foreach (const QString& view, views) {
    views_combobox_->addItem(view, view);
  }

  emit DisplayChanged(display_combobox_->currentData().toString());
}

void ExportVideoTab::ColorViewChanged()
{
  emit ViewChanged(views_combobox_->currentData().toString());
}

void ExportVideoTab::ColorLookChanged()
{
  emit LookChanged(looks_combobox_->currentData().toString());
}
