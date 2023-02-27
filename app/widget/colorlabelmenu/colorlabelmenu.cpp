/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QPainter>
#include <QWidgetAction>

#include "common/qtutils.h"
#include "ui/colorcoding.h"

namespace olive {

ColorLabelMenu::ColorLabelMenu(QWidget *parent) :
  Menu(parent)
{
  // Used for size calculations
  int box_size = fontMetrics().height();

  color_items_.resize(ColorCoding::standard_colors().size());
  for (int i=0; i<ColorCoding::standard_colors().size(); i++) {
    QPixmap p(box_size, box_size);

    QPainter painter(&p);
    painter.setPen(Qt::black);
    painter.setBrush(QtUtils::toQColor(ColorCoding::standard_colors().at(i)));
    painter.drawRect(p.rect().adjusted(0, 0, -1, -1));

    QAction *a = AddItem(QStringLiteral("colorlabel%1").arg(i), this, &ColorLabelMenu::ActionTriggered);
    a->setIcon(p);
    a->setData(i);
    color_items_.replace(i, a);
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
    color_items_.at(i)->setText(ColorCoding::GetColorName(i));
  }
}

void ColorLabelMenu::ActionTriggered()
{
  QAction* a = static_cast<QAction*>(sender());
  emit ColorSelected(a->data().toInt());
}

}
