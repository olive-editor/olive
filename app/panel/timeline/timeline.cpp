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

#include <QScrollBar>
#include <QVBoxLayout>

TimelinePanel::TimelinePanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: Test code
  QWidget* main = new QWidget(this);
  setWidget(main);

  QVBoxLayout* layout = new QVBoxLayout(main);
  layout->setSpacing(0);
  layout->setMargin(0);

  ruler_ = new TimeRuler(true, this);
  layout->addWidget(ruler_);

  view_ = new TimelineView(this);
  layout->addWidget(view_);

  connect(view_->horizontalScrollBar(), SIGNAL(valueChanged(int)), ruler_, SLOT(SetScroll(int)));
  connect(ruler_, SIGNAL(TimeChanged(const int64_t&)), view_, SLOT(SetTime(const int64_t&)));
  // End test code

  Retranslate();

//  ruler_->SetScale(90.0);
//  view_->SetScale(90.0);
}

void TimelinePanel::Clear()
{
  SetTimebase(0);

  view_->Clear();
}

void TimelinePanel::AddClip(ClipBlock *clip)
{
  view_->AddClip(clip);
}

void TimelinePanel::SetTimebase(const rational &timebase)
{
  ruler_->SetTimebase(timebase);
  view_->SetTimebase(timebase);
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
