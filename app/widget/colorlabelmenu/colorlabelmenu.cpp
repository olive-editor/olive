/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "colorlabelmenu.h"

#include <QEvent>
#include <QWidgetAction>

#include "ui/colorcoding.h"

namespace olive {

ColorLabelMenu::ColorLabelMenu(QWidget *parent) :
  Menu(parent)
{
  for (int i=0; i<ColorCoding::standard_colors().size(); i++) {
    ColorLabelMenuItem* item = new ColorLabelMenuItem();
    item->SetColor(ColorCoding::standard_colors().at(i));
    color_items_.append(item);

    QWidgetAction* a = new QWidgetAction(this);
    Menu::ConformItem(a, QStringLiteral("colorlabel%1").arg(i), this, &ColorLabelMenu::ActionTriggered);
    a->setData(i);
    a->setDefaultWidget(item);

    this->addAction(a);
  }

  Retranslate();
}

void ColorLabelMenu::changeEvent(QEvent *event)
{
  if (event->type() == QEvent::LanguageChange) {
    Retranslate();
  }

  Menu::changeEvent(event);
}

void ColorLabelMenu::Retranslate()
{
  this->setTitle(tr("Color"));

  for (int i=0; i<color_items_.size(); i++) {
    color_items_.at(i)->SetText(ColorCoding::GetColorName(i));
  }
}

void ColorLabelMenu::ActionTriggered()
{
  QAction* a = static_cast<QAction*>(sender());
  emit ColorSelected(a->data().toInt());
}

}
