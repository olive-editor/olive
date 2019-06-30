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

#include "platform/theme/themeservice-impl.h"
#include "widget/timeline/timelinewidget.h"
#include "project/sequence/sequence.h"
#include "project/sequence/track.h"
#include "project/sequence/clip.h"

#include <iostream>

TimelinePanel::TimelinePanel(QWidget *parent) :
  PanelWidget(parent)
{
  // Create Sequence mock data
  auto sequence = new olive::Sequence();
  sequence->addTrack();
  auto track1 = sequence->addTrack();
  auto track2 = sequence->addTrack();
  track1->addClip(new olive::Clip(30, 150));
  track2->addClip(new olive::Clip(120, 260));

  auto widget = new olive::TimelineWidget(this, olive::ThemeService::instance());
  widget->setSequence(sequence);

  setWidget(widget);

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
