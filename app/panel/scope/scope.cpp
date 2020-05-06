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

#include "scope.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QGuiApplication>

#include "panel/viewer/viewer.h"

OLIVE_NAMESPACE_ENTER

ScopePanel::ScopePanel(QWidget* parent) :
  PanelWidget(QStringLiteral("ScopePanel"), parent)
{
  QWidget* central = new QWidget();
  setWidget(central);

  QVBoxLayout* layout = new QVBoxLayout(central);

  QHBoxLayout* toolbar_layout = new QHBoxLayout();
  toolbar_layout->setMargin(0);

  scope_type_combobox_ = new QComboBox();

  for (int i=0;i<ScopePanel::kTypeCount;i++) {
    // These strings get filled in later in Retranslate()
    scope_type_combobox_->addItem(QString());
  }

  toolbar_layout->addWidget(scope_type_combobox_);

  toolbar_layout->addStretch();

  swizzle_.reserve(4);
  swizzle_ = {true, true, true, false};

  //check boxes for channels in waveform
  luma_select_ = new QCheckBox(tr("Luma"), this);
  toolbar_layout->addWidget(luma_select_);

  red_select_ = new QCheckBox(tr("R"), this);
  red_select_->setChecked(true);
  toolbar_layout->addWidget(red_select_);

  green_select_ = new QCheckBox(tr("G"), this);
  green_select_->setChecked(true);
  toolbar_layout->addWidget(green_select_);

  blue_select_ = new QCheckBox(tr("B"), this);
  blue_select_->setChecked(true);
  toolbar_layout->addWidget(blue_select_);

  layout->addLayout(toolbar_layout);

  stack_ = new QStackedWidget();
  layout->addWidget(stack_);

  // Create waveform view
  waveform_view_ = new WaveformScope();
  stack_->addWidget(waveform_view_);

  // Create histogram
  histogram_ = new HistogramScope();
  stack_->addWidget(histogram_);

  connect(scope_type_combobox_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      stack_, &QStackedWidget::setCurrentIndex);
  connect(scope_type_combobox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), 
      this, &ScopePanel::SetSwizzleVisibility);

  connect(luma_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(3); });
  connect(red_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(0); });
  connect(green_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(1); });
  connect(blue_select_, &QCheckBox::clicked, this, [this] { SortSwizzleData(2); });

  connect(this, &ScopePanel::SendSwizzleData, waveform_view_, &WaveformScope::SetSwizzleData);

  Retranslate();

  // show channel selectors if showing waveform
  SetSwizzleVisibility();
}

void ScopePanel::SetSwizzleVisibility() 
{
  bool display_swizzle = false;
  if (scope_type_combobox_->currentText() == QString("Waveform")) {
    display_swizzle = true;
  }
  luma_select_->setVisible(display_swizzle);
  red_select_->setVisible(display_swizzle);
  green_select_->setVisible(display_swizzle);
  blue_select_->setVisible(display_swizzle);
}

void ScopePanel::SortSwizzleData(int checkbox) 
{ 
  //If Ctrl is pressed when selecting a channel, display only the 
  //the channel selected
  if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
    for (int i = 0; i < 4; i++) {
      swizzle_[i] = (i == checkbox? true : false);
    }
    red_select_->setChecked(checkbox == 0 ? true : false);
    green_select_->setChecked(checkbox == 1 ? true : false);
    blue_select_->setChecked(checkbox == 2 ? true : false);
    luma_select_->setChecked(checkbox == 3 ? true : false);
  } else { 
    swizzle_[checkbox] = !swizzle_[checkbox];
  }

  emit SendSwizzleData(swizzle_);

}

void ScopePanel::SetType(ScopePanel::Type t)
{
  scope_type_combobox_->setCurrentIndex(t);
}

QString ScopePanel::TypeToName(ScopePanel::Type t)
{
  switch (t) {
  case kTypeWaveform:
    return tr("Waveform");
  case kTypeHistogram:
    return tr("Histogram");
  case kTypeCount:
    break;
  }

  return QString();
}

void ScopePanel::SetReferenceBuffer(Frame *frame)
{
  histogram_->SetBuffer(frame);
  waveform_view_->SetBuffer(frame);
}

void ScopePanel::SetColorManager(ColorManager *manager)
{
  histogram_->ConnectColorManager(manager);
  waveform_view_->ConnectColorManager(manager);
}

void ScopePanel::Retranslate()
{
  SetTitle(tr("Scope"));

  for (int i=0;i<ScopePanel::kTypeCount;i++) {
    scope_type_combobox_->setItemText(i, TypeToName(static_cast<Type>(i)));
  }
}

OLIVE_NAMESPACE_EXIT
