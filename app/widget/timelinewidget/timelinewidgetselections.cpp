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

#include "timelinewidgetselections.h"

OLIVE_NAMESPACE_ENTER

void TimelineWidgetSelections::ShiftTime(const rational &diff)
{
  for (auto it=this->begin(); it!=this->end(); it++) {
    it.value().shift(diff);
  }
}

void TimelineWidgetSelections::ShiftTracks(Timeline::TrackType type, int diff)
{
  TimelineWidgetSelections cached_selections;

  {
    // Take all selections of this track type
    auto it = this->begin();
    while (it != this->end()) {
      if (it.key().type() == type) {
        cached_selections.insert(it.key(), it.value());
        it = this->erase(it);
      } else {
        it++;
      }
    }
  }

  // Then re-insert them with the diff applied
  for (auto it=cached_selections.cbegin(); it!=cached_selections.cend(); it++) {
    TrackReference ref(it.key().type(), it.key().index() + diff);

    this->insert(ref, it.value());
  }
}

void TimelineWidgetSelections::TrimIn(const rational &diff)
{
  for (auto it=this->begin(); it!=this->end(); it++) {
    it.value().trim_in(diff);
  }
}

void TimelineWidgetSelections::TrimOut(const rational &diff)
{
  for (auto it=this->begin(); it!=this->end(); it++) {
    it.value().trim_out(diff);
  }
}

OLIVE_NAMESPACE_EXIT
