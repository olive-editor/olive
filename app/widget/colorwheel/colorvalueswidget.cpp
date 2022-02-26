/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QMouseEvent>
#include <QTabWidget>

#include "config/config.h"
#include "core.h"

namespace olive {

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

    color_picker_btn_ = new QPushButton(tr("Pick"));
    color_picker_btn_->setCheckable(true);
    connect(color_picker_btn_, &QPushButton::toggled, this, &ColorValuesWidget::ColorPickedBtnToggled);
    connect(Core::instance(), &Core::ColorPickerColorEmitted, this, &ColorValuesWidget::SetReferenceColor);
    preview_layout->addWidget(color_picker_btn_);

    layout->addLayout(preview_layout);
  }

  // Create value tabs
  {
    QTabWidget* tabs = new QTabWidget();

    input_tab_ = new ColorValuesTab(true);
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

bool ColorValuesWidget::eventFilter(QObject *watcher, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress) {
    // Should signal to Core to stop pixel sampling and to us to remove our event filter
    bool use_this_color = true;

    foreach (QWidget *w, ignore_pick_from_) {
      if (w->underMouse()) {
        use_this_color = false;
        break;
      }
    }

    if (use_this_color) {
      picker_end_color_ = GetColor();
    }
    color_picker_btn_->setChecked(false);
    return true;
  } else if (event->type() == QEvent::KeyPress) {
    QKeyEvent *key_ev = static_cast<QKeyEvent*>(event);

    if (key_ev->key() == Qt::Key_Escape) {
      color_picker_btn_->setChecked(false);
      return true;
    }
  }

  return QWidget::eventFilter(watcher, event);
}

void ColorValuesWidget::SetColor(const Color &c)
{
  input_tab_->SetColor(c);
  preview_->SetColor(c);

  UpdateValuesFromInput();
}

void ColorValuesWidget::SetReferenceColor(const Color &c)
{
  reference_tab_->SetColor(c);

  UpdateValuesFromRef();
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

void ColorValuesWidget::ColorPickedBtnToggled(bool e)
{
  Core::instance()->RequestPixelSamplingInViewers(e);

  if (e) {
    qApp->installEventFilter(this);

    // Store current color in case it needs to be restored
    picker_end_color_ = GetColor();
  } else {
    qApp->removeEventFilter(this);

    // Restore original color (or use overridden color from eventFilter)
    SetReferenceColor(picker_end_color_);
    emit ColorChanged(picker_end_color_);
  }
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

const double ColorValuesTab::kLegacyMultiplier = 255.0;

ColorValuesTab::ColorValuesTab(bool with_legacy_option, QWidget *parent) :
  QWidget(parent)
{
  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  if (with_legacy_option) {
    legacy_box_ = new QCheckBox(tr("Use legacy (8-bit) values"));
    legacy_box_->setChecked(Config::Current()[QStringLiteral("UseLegacyColorInInputTab")].toBool());
    connect(legacy_box_, &QCheckBox::clicked, this, &ColorValuesTab::LegacyChanged);
    layout->addWidget(legacy_box_, row, 0, 1, 2);
    row++;
  } else {
    legacy_box_ = nullptr;
  }

  sliders_.resize(3);

  layout->addWidget(new QLabel(tr("Red")), row, 0);

  red_slider_ = CreateColorSlider();
  sliders_[0] = red_slider_;
  layout->addWidget(red_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Green")), row, 0);

  green_slider_ = CreateColorSlider();
  sliders_[1] = green_slider_;
  layout->addWidget(green_slider_, row, 1);

  row++;

  layout->addWidget(new QLabel(tr("Blue")), row, 0);

  blue_slider_ = CreateColorSlider();
  sliders_[2] = blue_slider_;
  layout->addWidget(blue_slider_, row, 1);

  row++;

  hex_lbl_ = new QLabel(tr("Hex"));
  layout->addWidget(hex_lbl_, row, 0);

  hex_slider_ = new StringSlider();
  connect(hex_slider_, &StringSlider::ValueChanged, this, &ColorValuesTab::HexChanged);
  layout->addWidget(hex_slider_, row, 1);

  LegacyChanged(AreSlidersLegacyValues());
}

Color ColorValuesTab::GetColor() const
{
  return Color(GetRed(), GetGreen(), GetBlue());
}

void ColorValuesTab::SetColor(const Color &c)
{
  SetRed(c.red());
  SetGreen(c.green());
  SetBlue(c.blue());
}

double ColorValuesTab::GetRed() const
{
  return GetValueInternal(red_slider_);
}

double ColorValuesTab::GetGreen() const
{
  return GetValueInternal(green_slider_);
}

double ColorValuesTab::GetBlue() const
{
  return GetValueInternal(blue_slider_);
}

void ColorValuesTab::SetRed(double r)
{
  SetValueInternal(red_slider_, r);
}

void ColorValuesTab::SetGreen(double g)
{
  SetValueInternal(green_slider_, g);
}

void ColorValuesTab::SetBlue(double b)
{
  SetValueInternal(blue_slider_, b);
}

double ColorValuesTab::GetValueInternal(FloatSlider *slider) const
{
  double d = slider->GetValue();

  if (AreSlidersLegacyValues()) {
    d /= kLegacyMultiplier;
  }

  return d;
}

void ColorValuesTab::SetValueInternal(FloatSlider *slider, double v)
{
  if (AreSlidersLegacyValues()) {
    v *= kLegacyMultiplier;
  }

  slider->SetValue(v);
  UpdateHex();
}

FloatSlider *ColorValuesTab::CreateColorSlider()
{
  FloatSlider* fs = new FloatSlider();
  fs->SetLadderElementCount(1);
  connect(fs, &FloatSlider::ValueChanged, this, &ColorValuesTab::SliderChanged);
  return fs;
}

void ColorValuesTab::SliderChanged()
{
  emit ColorChanged(GetColor());
  UpdateHex();
}

void ColorValuesTab::LegacyChanged(bool legacy)
{
  Config::Current()[QStringLiteral("UseLegacyColorInInputTab")] = legacy;

  double legacy_multiplier = legacy ? kLegacyMultiplier : 1.0/kLegacyMultiplier;
  int decimal_places = legacy ? 0 : 5;
  double drag_multiplier = legacy ? 1.0 : 0.01;

  foreach (FloatSlider *s, sliders_) {
    s->SetValue(s->GetValue() * legacy_multiplier);
    s->SetDecimalPlaces(decimal_places);
    s->SetDragMultiplier(drag_multiplier);
  }

  hex_lbl_->setVisible(legacy);
  hex_slider_->setVisible(legacy);
  UpdateHex();
}

void ColorValuesTab::UpdateHex()
{
  if (AreSlidersLegacyValues()) {
    double r = red_slider_->GetValue();
    double g = green_slider_->GetValue();
    double b = blue_slider_->GetValue();

    if (r > kLegacyMultiplier || g > kLegacyMultiplier || b > kLegacyMultiplier) {
      hex_slider_->SetValue(tr("(Invalid)"));
    } else {
      uint32_t rgb = (uint8_t(r) << 16) | (uint8_t(g) << 8) | uint8_t(b);

      hex_slider_->SetValue(QStringLiteral("%1").arg(rgb, 6, 16, QLatin1Char('0')).toUpper());
    }
  }
}

void ColorValuesTab::HexChanged(const QString &s)
{
  bool ok;
  uint32_t hex = s.toULong(&ok, 16);

  if (ok) {
    uint32_t r = (hex & 0xFF0000) >> 16;
    uint32_t g = (hex & 0x00FF00) >> 8;
    uint32_t b = (hex & 0x0000FF);

    red_slider_->SetValue(r);
    green_slider_->SetValue(g);
    blue_slider_->SetValue(b);

    emit ColorChanged(GetColor());
  } else {
    // Return to original value
    UpdateHex();
  }
}

bool ColorValuesTab::AreSlidersLegacyValues() const
{
  return legacy_box_ && legacy_box_->isChecked();
}

}
