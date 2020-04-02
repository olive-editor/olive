#include "colordialog.h"

#include <QDialogButtonBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "common/qtutils.h"

ColorDialog::ColorDialog(ColorManager* color_manager, Color start, QWidget *parent) :
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

  color_values_widget_ = new ColorValuesWidget();
  value_layout->addWidget(color_values_widget_);

  ColorSpaceChooser* chooser = new ColorSpaceChooser(color_manager_);
  value_layout->addWidget(chooser);

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

  color_wheel_->SetSelectedColor(start);
  hsv_value_gradient_->SetSelectedColor(start);
  color_values_widget_->SetColor(start);

  connect(chooser, &ColorSpaceChooser::ColorSpaceChanged, this, &ColorDialog::ColorSpaceChanged);
  ColorSpaceChanged(chooser->input(), chooser->display(), chooser->view(), chooser->look());
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

void ColorDialog::ColorSpaceChanged(const QString &input, const QString &display, const QString &view, const QString &look)
{
  ColorProcessorPtr to_linear_processor_ = ColorProcessor::Create(color_manager_->GetConfig(), input, OCIO::ROLE_SCENE_LINEAR);

  ColorProcessorPtr to_display = ColorProcessor::Create(color_manager_->GetConfig(),
                                                        OCIO::ROLE_SCENE_LINEAR,
                                                        display,
                                                        view,
                                                        look);

  color_wheel_->SetColorProcessor(to_linear_processor_, to_display);
  hsv_value_gradient_->SetColorProcessor(to_linear_processor_, to_display);
  color_values_widget_->preview_box()->SetColorProcessor(to_linear_processor_, to_display);
}
