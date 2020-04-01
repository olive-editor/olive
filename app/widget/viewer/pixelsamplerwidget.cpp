#include "pixelsamplerwidget.h"

#include <QVBoxLayout>

PixelSamplerWidget::PixelSamplerWidget(QWidget *parent) :
  QGroupBox(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  label_ = new QLabel();
  layout->addWidget(label_);

  setTitle(tr("Color"));

  UpdateLabelInternal();
}

void PixelSamplerWidget::SetValues(const Color &color)
{
  color_ = color;
  UpdateLabelInternal();
}

void PixelSamplerWidget::UpdateLabelInternal()
{
  label_->setText(tr("<html>"
                     "<font color='#FF8080'>R: %1</font><br>"
                     "<font color='#80FF80'>G: %2</font><br>"
                     "<font color='#8080FF'>B: %3</font><br>"
                     "A: %4"
                     "</html>").arg(QString::number(color_.red()),
                                    QString::number(color_.green()),
                                    QString::number(color_.blue()),
                                    QString::number(color_.alpha())));
}

ManagedPixelSamplerWidget::ManagedPixelSamplerWidget(QWidget *parent) :
  QWidget(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  display_view_ = new PixelSamplerWidget();
  display_view_->setTitle(tr("Display"));
  layout->addWidget(display_view_);

  reference_view_ = new PixelSamplerWidget();
  reference_view_->setTitle(tr("Reference"));
  layout->addWidget(reference_view_);
}

void ManagedPixelSamplerWidget::SetValues(const Color &reference, const Color &display)
{
  reference_view_->SetValues(reference);
  display_view_->SetValues(display);
}
