#include "colorvalueswidget.h"

#include <QGridLayout>

ColorValuesWidget::ColorValuesWidget(QWidget *parent) :
  QWidget(parent)
{
  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Preview")), row, 0);

  preview_ = new ColorPreviewBox();
  preview_->setFixedHeight(fontMetrics().height() * 3 / 2);
  layout->addWidget(preview_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Red")), row, 0);

  red_slider_ = CreateColorSlider();
  layout->addWidget(red_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Green")), row, 0);

  green_slider_ = CreateColorSlider();
  layout->addWidget(green_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Blue")), row, 0);

  blue_slider_ = CreateColorSlider();
  layout->addWidget(blue_slider_, row, 1);
}

Color ColorValuesWidget::GetColor() const
{
  return Color(red_slider_->GetValue(),
               green_slider_->GetValue(),
               blue_slider_->GetValue());
}

ColorPreviewBox *ColorValuesWidget::preview_box() const
{
  return preview_;
}

void ColorValuesWidget::SetColor(const Color &c)
{
  red_slider_->SetValue(c.red());
  green_slider_->SetValue(c.green());
  blue_slider_->SetValue(c.blue());
  preview_->SetColor(c);
}

FloatSlider *ColorValuesWidget::CreateColorSlider()
{
  FloatSlider* fs = new FloatSlider();
  fs->SetMinimum(0);
  fs->SetDragMultiplier(0.01);
  fs->SetMaximum(1);
  fs->SetDecimalPlaces(3);
  connect(fs, &FloatSlider::ValueChanged, this, &ColorValuesWidget::SliderChanged);
  return fs;
}

void ColorValuesWidget::SliderChanged()
{
  Color c(red_slider_->GetValue(), green_slider_->GetValue(), blue_slider_->GetValue());

  preview_->SetColor(c);

  emit ColorChanged(c);
}
