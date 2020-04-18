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

#include "colordialog.h"

#include <QDialogButtonBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "common/qtutils.h"

OLIVE_NAMESPACE_ENTER

ColorDialog::ColorDialog(ColorManager* color_manager, const ManagedColor& start, QWidget *parent) :
  QDialog(parent),
  color_manager_(color_manager)
{
  setWindowTitle(tr("Select Color"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  QWidget* wheel_area = new QWidget();
  QHBoxLayout* wheel_layout = new QHBoxLayout(wheel_area);
  splitter->addWidget(wheel_area);

  color_wheel_ = new ColorWheelWidget();
  wheel_layout->addWidget(color_wheel_);

  hsv_value_gradient_ = new ColorGradientWidget(Qt::Vertical);
  hsv_value_gradient_->setFixedWidth(QFontMetricsWidth(fontMetrics(), QStringLiteral("HHH")));
  wheel_layout->addWidget(hsv_value_gradient_);

  QWidget* value_area = new QWidget();
  QVBoxLayout* value_layout = new QVBoxLayout(value_area);
  value_layout->setSpacing(0);
  splitter->addWidget(value_area);

  color_values_widget_ = new ColorValuesWidget(color_manager_);
  value_layout->addWidget(color_values_widget_);

  chooser_ = new ColorSpaceChooser(color_manager_);
  chooser_->set_input(start.color_input());
  chooser_->set_display(start.color_display());
  chooser_->set_view(start.color_view());
  chooser_->set_look(start.color_look());

  value_layout->addWidget(chooser_);

  // Split window 50/50
  splitter->setSizes({INT_MAX, INT_MAX});

  connect(color_wheel_, &ColorWheelWidget::SelectedColorChanged, color_values_widget_, &ColorValuesWidget::SetColor);
  connect(color_wheel_, &ColorWheelWidget::SelectedColorChanged, hsv_value_gradient_, &ColorGradientWidget::SetSelectedColor);
  connect(hsv_value_gradient_, &ColorGradientWidget::SelectedColorChanged, color_values_widget_, &ColorValuesWidget::SetColor);
  connect(hsv_value_gradient_, &ColorGradientWidget::SelectedColorChanged, color_wheel_, &ColorWheelWidget::SetSelectedColor);
  connect(color_values_widget_, &ColorValuesWidget::ColorChanged, hsv_value_gradient_, &ColorGradientWidget::SetSelectedColor);
  connect(color_values_widget_, &ColorValuesWidget::ColorChanged, color_wheel_, &ColorWheelWidget::SetSelectedColor);

  connect(color_wheel_, &ColorWheelWidget::DiameterChanged, hsv_value_gradient_, &ColorGradientWidget::setFixedHeight);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  Color managed_start;

  if (start.color_input().isEmpty()) {

    managed_start = start;

  } else {

    // Convert reference color to the input space
    ColorProcessorPtr linear_to_input = ColorProcessor::Create(color_manager_,
                                                               color_manager_->GetReferenceColorSpace(),
                                                               start.color_input());

    managed_start = linear_to_input->ConvertColor(start);

  }

  color_wheel_->SetSelectedColor(managed_start);
  hsv_value_gradient_->SetSelectedColor(managed_start);
  color_values_widget_->SetColor(managed_start);

  connect(chooser_, &ColorSpaceChooser::ColorSpaceChanged, this, &ColorDialog::ColorSpaceChanged);
  ColorSpaceChanged(chooser_->input(), chooser_->display(), chooser_->view(), chooser_->look());

  // Set default size ratio to 2:1
  resize(sizeHint().height() * 2, sizeHint().height());
}

ManagedColor ColorDialog::GetSelectedColor() const
{
  ManagedColor selected = color_wheel_->GetSelectedColor();

  // Convert to linear and return a linear color
  if (input_to_ref_processor_) {
    selected = input_to_ref_processor_->ConvertColor(selected);
  }

  selected.set_color_input(GetColorSpaceInput());
  selected.set_color_display(GetColorSpaceDisplay());
  selected.set_color_view(GetColorSpaceView());
  selected.set_color_look(GetColorSpaceLook());

  return selected;
}

QString ColorDialog::GetColorSpaceInput() const
{
  return chooser_->input();
}

QString ColorDialog::GetColorSpaceDisplay() const
{
  return chooser_->display();
}

QString ColorDialog::GetColorSpaceView() const
{
  return chooser_->view();
}

QString ColorDialog::GetColorSpaceLook() const
{
  return chooser_->look();
}

void ColorDialog::ColorSpaceChanged(const QString &input, const QString &display, const QString &view, const QString &look)
{
  input_to_ref_processor_ = ColorProcessor::Create(color_manager_, input, color_manager_->GetReferenceColorSpace());

  ColorProcessorPtr ref_to_display = ColorProcessor::Create(color_manager_,
                                                            color_manager_->GetReferenceColorSpace(),
                                                            display,
                                                            view,
                                                            look);

  ColorProcessorPtr ref_to_input = ColorProcessor::Create(color_manager_, color_manager_->GetReferenceColorSpace(), input);

  // FIXME: For some reason, using OCIO::TRANSFORM_DIR_INVERSE (wrapped by ColorProcessor::kInverse) causes OCIO to
  //        crash. We've disabled that functionality for now (also disabling display_tab_ in ColorValuesWidget)

  /*ColorProcessorPtr display_to_ref = ColorProcessor::Create(color_manager_->GetConfig(),
                                                            color_manager_->GetReferenceColorSpace(),
                                                            display,
                                                            view,
                                                            look,
                                                            ColorProcessor::kInverse);*/

  color_wheel_->SetColorProcessor(input_to_ref_processor_, ref_to_display);
  hsv_value_gradient_->SetColorProcessor(input_to_ref_processor_, ref_to_display);
  color_values_widget_->SetColorProcessor(input_to_ref_processor_, ref_to_display, nullptr, ref_to_input);
}

OLIVE_NAMESPACE_EXIT
