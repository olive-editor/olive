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

  ColorValuesWidget* cvw = new ColorValuesWidget();
  splitter->addWidget(cvw);

  splitter->setSizes({INT_MAX, 0});

  connect(color_wheel_, &ColorWheelWidget::SelectedColorChanged, cvw, &ColorValuesWidget::SetColor);
  connect(color_wheel_, &ColorWheelWidget::SelectedColorChanged, hsv_value_gradient_, &ColorGradientWidget::SetSelectedColor);
  connect(hsv_value_gradient_, &ColorGradientWidget::SelectedColorChanged, cvw, &ColorValuesWidget::SetColor);
  connect(hsv_value_gradient_, &ColorGradientWidget::SelectedColorChanged, color_wheel_, &ColorWheelWidget::SetSelectedColor);

  connect(color_wheel_, &ColorWheelWidget::DiameterChanged, hsv_value_gradient_, &ColorGradientWidget::setFixedHeight);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  color_wheel_->SetSelectedColor(start);
  hsv_value_gradient_->SetSelectedColor(start);
  cvw->SetColor(start);
}

const Color &ColorDialog::GetSelectedColor() const
{
  return color_wheel_->GetSelectedColor();
}
