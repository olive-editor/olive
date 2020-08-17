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

  stack_ui_ = new QStackedWidget();
  stack_ui_->setContentsMargins(0, 0, 0, 0);
  // Height has to be forced otherwise it uses far too much space
  stack_ui_->setFixedHeight(20);
  toolbar_layout->addWidget(stack_ui_);

  layout->addLayout(toolbar_layout);

  stack_ = new QStackedWidget();
  layout->addWidget(stack_);

  // Create waveform view
  waveform_view_ = new WaveformScope();
  stack_->addWidget(waveform_view_);
  stack_ui_->addWidget(waveform_view_->ControlUI());

  // Create histogram
  histogram_ = new HistogramScope();
  stack_->addWidget(histogram_);
  stack_ui_->addWidget(histogram_->ControlUI());

  connect(scope_type_combobox_, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), stack_, &QStackedWidget::setCurrentIndex);
  connect(scope_type_combobox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), stack_ui_,
          &QStackedWidget::setCurrentIndex);

  Retranslate();
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
