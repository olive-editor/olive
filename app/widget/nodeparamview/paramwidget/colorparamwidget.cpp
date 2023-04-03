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

#include "widget/colorbutton/colorbutton.h"

namespace olive {

ColorParamWidget::ColorParamWidget(QObject *parent)
  : AbstractParamWidget{parent}
{

}

void ColorParamWidget::Initialize(QWidget *parent, size_t channels)
{
  qDebug() << "CPW::Initialize - stub";
  /*
  for (size_t i = 0; i < channels; i++) {
    ColorButton* color_button = new ColorButton(project()->color_manager(), parent);
    AddWidget(color_button);
    connect(color_button, &ColorButton::ColorChanged, this, &ColorParamWidget::Arbitrate);
  }
  */
}

void ColorParamWidget::SetValue(const value_t &val)
{
  qDebug() << "CPW::SetValue - stub";
  /*
  ManagedColor mc = val.value<Color>();

  mc.set_color_input(GetInnerInput().GetProperty("col_input").toString());

  QString d = GetInnerInput().GetProperty("col_display").toString();
  QString v = GetInnerInput().GetProperty("col_view").toString();
  QString l = GetInnerInput().GetProperty("col_look").toString();

  mc.set_color_output(ColorTransform(d, v, l));

  static_cast<ColorButton*>(GetWidgets().at(0))->SetColor(mc);
  */
}

void ColorParamWidget::Arbitrate()
{
  qDebug() << "CPW::Arbitrate - stub";
  /*
  // Sender is a ColorButton
  ManagedColor c = static_cast<ColorButton*>(sender())->GetColor();

  MultiUndoCommand* command = new MultiUndoCommand();

  SetInputValueInternal(c.red(), 0, command, false);
  SetInputValueInternal(c.green(), 1, command, false);
  SetInputValueInternal(c.blue(), 2, command, false);
  SetInputValueInternal(c.alpha(), 3, command, false);

  Node* n = GetInnerInput().node();
  n->blockSignals(true);
  n->SetInputProperty(GetInnerInput().input(), QStringLiteral("col_input"), c.color_input());
  n->SetInputProperty(GetInnerInput().input(), QStringLiteral("col_display"), c.color_output().display());
  n->SetInputProperty(GetInnerInput().input(), QStringLiteral("col_view"), c.color_output().view());
  n->SetInputProperty(GetInnerInput().input(), QStringLiteral("col_look"), c.color_output().look());
  n->blockSignals(false);

  Core::instance()->undo_stack()->push(command, GetCommandName());
  */
}

}
