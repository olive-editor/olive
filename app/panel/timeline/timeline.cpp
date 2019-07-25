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

#include "timeline.h"

#include <QVBoxLayout>

#include "widget/timeruler/timeruler.h"

TimelinePanel::TimelinePanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: Test code
  QWidget* main = new QWidget(this);
  setWidget(main);

  QVBoxLayout* layout = new QVBoxLayout(main);
  layout->setSpacing(0);
  layout->setMargin(0);

  TimeRuler* tr = new TimeRuler(true, this);
  layout->addWidget(tr);
  layout->addStretch();
  // End test code

  Retranslate();
}

void TimelinePanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void TimelinePanel::Retranslate()
{
  SetTitle(tr("Timeline"));
  SetSubtitle(tr("(none)"));
}
