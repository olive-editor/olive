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
  layout->addWidget(splitter);

  QWidget* wheel_area = new QWidget();
  QHBoxLayout* wheel_layout = new QHBoxLayout(wheel_area);
  splitter->addWidget(wheel_area);

  color_wheel_ = new ColorWheelGLWidget();
  wheel_layout->addWidget(color_wheel_);

  hsv_value_gradient_ = new ColorGradientGLWidget(Qt::Vertical);
  hsv_value_gradient_->setFixedWidth(QFontMetricsWidth(fontMetrics(), QStringLiteral("HHH")));
  wheel_layout->addWidget(hsv_value_gradient_);

  ColorValuesWidget* cvw = new ColorValuesWidget();
  splitter->addWidget(cvw);

  connect(color_wheel_, &ColorWheelGLWidget::ColorChanged, cvw, &ColorValuesWidget::SetColor);
  connect(color_wheel_, &ColorWheelGLWidget::ColorChanged, this, &ColorDialog::UpdateValueGradient);
  connect(color_wheel_, &ColorWheelGLWidget::DiameterChanged, hsv_value_gradient_, &ColorGradientGLWidget::setFixedHeight);
  connect(hsv_value_gradient_, &ColorGradientGLWidget::ColorChanged, cvw, &ColorValuesWidget::SetColor);
  connect(hsv_value_gradient_, &ColorGradientGLWidget::ColorChanged, this, &ColorDialog::UpdateWheelValue);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);

  UpdateValueGradient(start);
  UpdateWheelValue(start);
  cvw->SetColor(start);
}

void ColorDialog::UpdateValueGradient(const Color &c)
{
  float h, s, v;

  c.toHsv(&h, &s, &v);

  Color start = Color::fromHsv(h, s, 1.0f);
  Color end = Color::fromHsv(h, s, 0.0f);

  hsv_value_gradient_->SetColors(start, end);
}

void ColorDialog::UpdateWheelValue(const Color &c)
{
  float h, s, v;

  c.toHsv(&h, &s, &v);

  color_wheel_->SetHSVValue(v);
}

void ColorDialog::UpdateValueGradientSize(int diameter)
{
  hsv_value_gradient_->setFixedHeight(diameter);
}
