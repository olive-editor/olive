#include "colorbutton.h"

#include "dialog/color/colordialog.h"

ColorButton::ColorButton(ColorManager* color_manager, QWidget *parent) :
  QPushButton(parent),
  color_manager_(color_manager),
  color_processor_(nullptr)
{
  color_ = Color(1.0f, 1.0f, 1.0f);

  connect(this, &ColorButton::clicked, this, &ColorButton::ShowColorDialog);

  UpdateColor();
}

const Color &ColorButton::GetColor() const
{
  return color_;
}

void ColorButton::SetColor(const Color &c)
{
  color_ = c;

  UpdateColor();
}

void ColorButton::ShowColorDialog()
{
  ColorDialog cd(color_manager_, color_, cm_input_, this);

  if (cd.exec() == QDialog::Accepted) {
    color_ = cd.GetSelectedColor();

    cm_input_ = cd.GetColorSpaceInput();

    color_processor_ = ColorProcessor::Create(color_manager_->GetConfig(),
                                              cd.GetColorSpaceInput(),
                                              cd.GetColorSpaceDisplay(),
                                              cd.GetColorSpaceView(),
                                              cd.GetColorSpaceLook());

    UpdateColor();

    emit ColorChanged(color_);
  }
}

void ColorButton::UpdateColor()
{
  QColor managed;

  if (color_processor_) {
    managed = color_processor_->ConvertColor(color_).toQColor();
  } else {
    managed = color_.toQColor();
  }

  setStyleSheet(QStringLiteral("ColorButton {background: %1;}").arg(managed.name()));
}
