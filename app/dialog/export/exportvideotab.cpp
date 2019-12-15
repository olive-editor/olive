#include "exportvideotab.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

#include "core.h"
#include "common/rational.h"
#include "widget/slider/integerslider.h"

ExportVideoTab::ExportVideoTab(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  outer_layout->addWidget(SetupResolutionSection());

  outer_layout->addWidget(SetupColorSection());

  outer_layout->addWidget(SetupCodecSection());

  outer_layout->addStretch();
}

QComboBox *ExportVideoTab::codec_combobox() const
{
  return codec_combobox_;
}

QWidget* ExportVideoTab::SetupResolutionSection()
{
  int row = 0;

  QGroupBox* resolution_group = new QGroupBox();
  resolution_group->setTitle(tr("Basic"));

  QGridLayout* layout = new QGridLayout(resolution_group);

  layout->addWidget(new QLabel(tr("Width:")), row, 0);

  IntegerSlider* width_slider = new IntegerSlider();
  layout->addWidget(width_slider, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Height:")), row, 0);

  IntegerSlider* height_slider = new IntegerSlider();
  layout->addWidget(height_slider, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Maintain Aspect Ratio:")), row, 0);
  layout->addWidget(new QCheckBox(), row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Scaling Method:")), row, 0);
  layout->addWidget(new QComboBox(), row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Frame Rate:")), row, 0);

  frame_rate_combobox_ = new QComboBox();
  QList<rational> frame_rates = Core::SupportedFrameRates();
  foreach (const rational& fr, frame_rates) {
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
  color_layout->addWidget(new QComboBox(), row, 1);

  row++;

  color_layout->addWidget(new QLabel(tr("View:")), row, 0);
  color_layout->addWidget(new QComboBox(), row, 1);

  row++;

  color_layout->addWidget(new QLabel(tr("Look:")), row, 0);
  color_layout->addWidget(new QComboBox(), row, 1);

  row++;

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

  return codec_group;
}
