/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "colorparamwidget.h"

#include "node/project.h"
#include "widget/colorbutton/colorbutton.h"

namespace olive {

ColorParamWidget::ColorParamWidget(const NodeInput &input, QObject *parent)
  : AbstractParamWidget{parent},
    input_(input)
{

}

void ColorParamWidget::Initialize(QWidget *parent, size_t channels)
{
  Q_ASSERT(channels == 4);

  ColorButton* color_button = new ColorButton(input_.node()->project()->color_manager(), parent);
  AddWidget(color_button);
  connect(color_button, &ColorButton::ColorChanged, this, &ColorParamWidget::Arbitrate);
}

void ColorParamWidget::SetValue(const value_t &val)
{
  ManagedColor mc = val.toColor();

  mc.set_color_input(input_.GetProperty("col_input").toString());

  QString d = input_.GetProperty("col_display").toString();
  QString v = input_.GetProperty("col_view").toString();
  QString l = input_.GetProperty("col_look").toString();

  mc.set_color_output(ColorTransform(d, v, l));

  static_cast<ColorButton*>(GetWidgets().at(0))->SetColor(mc);
}

void ColorParamWidget::Arbitrate()
{
  // Sender is a ColorButton
  ManagedColor c = static_cast<ColorButton*>(sender())->GetColor();

  emit ChannelValueChanged(0, double(c.red()));
  emit ChannelValueChanged(1, double(c.green()));
  emit ChannelValueChanged(2, double(c.blue()));
  emit ChannelValueChanged(3, double(c.alpha()));

  Node* n = input_.node();
  n->blockSignals(true);
  n->SetInputProperty(input_.input(), QStringLiteral("col_input"), c.color_input());
  n->SetInputProperty(input_.input(), QStringLiteral("col_display"), c.color_output().display());
  n->SetInputProperty(input_.input(), QStringLiteral("col_view"), c.color_output().view());
  n->SetInputProperty(input_.input(), QStringLiteral("col_look"), c.color_output().look());
  n->blockSignals(false);
}

}
