#include "colordialog.h"

#include <QDialogButtonBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "common/qtutils.h"

ColorDialog::ColorDialog(ColorManager* color_manager, Color start, QString input_cs, QWidget *parent) :
  QDialog(parent),
  color_manager_(color_manager)
{
  setWindowTitle(tr("Select Color"));

  if (input_cs.isEmpty()) {
    input_cs = color_manager_->GetDefaultInputColorSpace();
  }

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

  color_values_widget_ = new ColorValuesWidget();
  value_layout->addWidget(color_values_widget_);

  chooser_ = new ColorSpaceChooser(color_manager_);
  chooser_->set_input(input_cs);
  value_layout->addWidget(chooser_);

  splitter->setSizes({INT_MAX, 0});

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

  {
    // Convert reference color to the input space
    ColorProcessorPtr linear_to_input = ColorProcessor::Create(color_manager_->GetConfig(),
                                                               color_manager_->GetReferenceColorSpace(),
                                                               input_cs);

    Color managed_start = linear_to_input->ConvertColor(start);

    color_wheel_->SetSelectedColor(managed_start);
    hsv_value_gradient_->SetSelectedColor(managed_start);
    color_values_widget_->SetColor(managed_start);
  }

  connect(chooser_, &ColorSpaceChooser::ColorSpaceChanged, this, &ColorDialog::ColorSpaceChanged);
  ColorSpaceChanged(chooser_->input(), chooser_->display(), chooser_->view(), chooser_->look());

  resize(sizeHint().height() * 2, sizeHint().height());
}

Color ColorDialog::GetSelectedColor() const
{
  Color selected = color_wheel_->GetSelectedColor();

  // Convert to linear and return a linear color
  if (to_linear_processor_) {
    return to_linear_processor_->ConvertColor(selected);
  }

  // Fallback if no processor is available
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
  to_linear_processor_ = ColorProcessor::Create(color_manager_->GetConfig(), input, color_manager_->GetReferenceColorSpace());

  ColorProcessorPtr to_display = ColorProcessor::Create(color_manager_->GetConfig(),
                                                        color_manager_->GetReferenceColorSpace(),
                                                        display,
                                                        view,
                                                        look);

  color_wheel_->SetColorProcessor(to_linear_processor_, to_display);
  hsv_value_gradient_->SetColorProcessor(to_linear_processor_, to_display);
  color_values_widget_->preview_box()->SetColorProcessor(to_linear_processor_, to_display);
}
