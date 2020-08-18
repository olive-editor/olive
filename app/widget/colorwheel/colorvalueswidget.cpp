/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "colorvalueswidget.h"

#include <QGridLayout>
#include <QTabWidget>

OLIVE_NAMESPACE_ENTER

ColorValuesWidget::ColorValuesWidget(ColorManager *manager, QWidget *parent) :
  QWidget(parent),
  manager_(manager),
  input_to_ref_(nullptr),
  ref_to_display_(nullptr),
  display_to_ref_(nullptr),
  ref_to_input_(nullptr)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Create preview box
  {
    QHBoxLayout* preview_layout = new QHBoxLayout();

    preview_layout->setMargin(0);

    preview_layout->addWidget(new QLabel(tr("Preview")));

    preview_ = new ColorPreviewBox();
    preview_->setFixedHeight(fontMetrics().height() * 3 / 2);
    preview_layout->addWidget(preview_);

    layout->addLayout(preview_layout);
  }

  // Create value tabs
  {
    QTabWidget* tabs = new QTabWidget();

    input_tab_ = new ColorValuesTab();
    tabs->addTab(input_tab_, tr("Input"));
    connect(input_tab_, &ColorValuesTab::ColorChanged, this, &ColorValuesWidget::UpdateValuesFromInput);
    connect(input_tab_, &ColorValuesTab::ColorChanged, this, &ColorValuesWidget::ColorChanged);
    connect(input_tab_, &ColorValuesTab::ColorChanged, preview_, &ColorPreviewBox::SetColor);

    reference_tab_ = new ColorValuesTab();
    tabs->addTab(reference_tab_, tr("Reference"));
    connect(reference_tab_, &ColorValuesTab::ColorChanged, this, &ColorValuesWidget::UpdateValuesFromRef);

    display_tab_ = new ColorValuesTab();
    tabs->addTab(display_tab_, tr("Display"));
    connect(display_tab_, &ColorValuesTab::ColorChanged, this, &ColorValuesWidget::UpdateValuesFromDisplay);

    // FIXME: Display -> Ref temporarily disabled due to OCIO crash (see ColorDialog::ColorSpaceChanged for more info)
    display_tab_->setEnabled(false);

    layout->addWidget(tabs);
  }
}

Color ColorValuesWidget::GetColor() const
{
  return reference_tab_->GetColor();
}

void ColorValuesWidget::SetColorProcessor(ColorProcessorPtr input_to_ref, ColorProcessorPtr ref_to_display, ColorProcessorPtr display_to_ref, ColorProcessorPtr ref_to_input)
{
  input_to_ref_ = input_to_ref;
  ref_to_display_ = ref_to_display;
  display_to_ref_ = display_to_ref;
  ref_to_input_ = ref_to_input;

  UpdateValuesFromInput();

  preview_->SetColorProcessor(input_to_ref_, ref_to_display_);
}

void ColorValuesWidget::SetColor(const Color &c)
{
  input_tab_->SetColor(c);
  preview_->SetColor(c);

  UpdateValuesFromInput();
}

void ColorValuesWidget::UpdateValuesFromInput()
{
  UpdateRefFromInput();
  UpdateDisplayFromRef();
}

void ColorValuesWidget::UpdateValuesFromRef()
{
  UpdateInputFromRef();
  UpdateDisplayFromRef();
}

void ColorValuesWidget::UpdateValuesFromDisplay()
{
  UpdateRefFromDisplay();
  UpdateInputFromRef();
}

void ColorValuesWidget::UpdateInputFromRef()
{
  if (ref_to_input_) {
    input_tab_->SetColor(ref_to_input_->ConvertColor(reference_tab_->GetColor()));
  } else {
    input_tab_->SetColor(reference_tab_->GetColor());
  }

  preview_->SetColor(input_tab_->GetColor());
  emit ColorChanged(input_tab_->GetColor());
}

void ColorValuesWidget::UpdateDisplayFromRef()
{
  if (ref_to_display_) {
    display_tab_->SetColor(ref_to_display_->ConvertColor(reference_tab_->GetColor()));
  } else {
    display_tab_->SetColor(reference_tab_->GetColor());
  }
}

void ColorValuesWidget::UpdateRefFromInput()
{
  if (input_to_ref_) {
    reference_tab_->SetColor(input_to_ref_->ConvertColor(input_tab_->GetColor()));
  } else {
    reference_tab_->SetColor(input_tab_->GetColor());
  }
}

void ColorValuesWidget::UpdateRefFromDisplay()
{
  if (display_to_ref_) {
    reference_tab_->SetColor(display_to_ref_->ConvertColor(display_tab_->GetColor()));
  } else {
    reference_tab_->SetColor(display_tab_->GetColor());
  }
}

ColorValuesTab::ColorValuesTab(QWidget *parent) :
  QWidget(parent)
{
  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

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

Color ColorValuesTab::GetColor() const
{
  return Color(red_slider_->GetValue(),
               green_slider_->GetValue(),
               blue_slider_->GetValue());
}

void ColorValuesTab::SetColor(const Color &c)
{
  red_slider_->SetValue(c.red());
  green_slider_->SetValue(c.green());
  blue_slider_->SetValue(c.blue());
}

FloatSlider *ColorValuesTab::CreateColorSlider()
{
  FloatSlider* fs = new FloatSlider();
  fs->SetDragMultiplier(0.01);
  fs->SetDecimalPlaces(5);
  fs->SetLadderElementCount(1);
  connect(fs, &FloatSlider::ValueChanged, this, &ColorValuesTab::SliderChanged);
  return fs;
}

void ColorValuesTab::SliderChanged()
{
  emit ColorChanged(GetColor());
}

OLIVE_NAMESPACE_EXIT
