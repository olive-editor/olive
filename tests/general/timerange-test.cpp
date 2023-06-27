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

#include <cstring>

#include "util/timerange.h"
#include "util/tests.h"

using namespace olive::core;

bool timerangelist_remove_test()
{
  TimeRangeList t;

  t.insert(TimeRange(0, 30));
  t.remove(TimeRange(2, 5));

  return true;
}

bool timerangelist_mergeadjacent_test()
{
  TimeRangeList t;

  // TimeRangeList should merge 1 and 3 together since they're adjacent
  t.insert(TimeRange(0, 6));
  t.insert(TimeRange(20, 30));
  t.insert(TimeRange(6, 10));

  if (!(t.size() == 2)) { return false; }
  if (!(t.first() == TimeRange(20, 30))) { return false; }
  if (!(t.at(1) == TimeRange(0, 10))) { return false; }

  // TimeRangeList should ignore these because it's already contained
  TimeRangeList noop_test = t;

  noop_test.insert(TimeRange(4, 7));
  if (!(noop_test == t)) { return false; }

  noop_test.insert(TimeRange(0, 3));
  if (!(noop_test == t)) { return false; }

  noop_test.insert(TimeRange(25, 30));
  if (!(noop_test == t)) { return false; }

  // TimeRangeList should combine all these together
  TimeRangeList combine_test_no_overlap = t;
  combine_test_no_overlap.insert(TimeRange(10, 20));
  if (!(combine_test_no_overlap.size() == 1)) { return false; }
  if (!(combine_test_no_overlap.first() == TimeRange(0, 30))) { return false; }

  TimeRangeList combine_test_in_overlap = t;
  combine_test_in_overlap.insert(TimeRange(9, 20));
  if (!(combine_test_in_overlap.size() == 1)) { return false; }
  if (!(combine_test_in_overlap.first() == TimeRange(0, 30))) { return false; }

  TimeRangeList combine_test_out_overlap = t;
  combine_test_out_overlap.insert(TimeRange(10, 21));
  if (!(combine_test_out_overlap.size() == 1)) { return false; }
  if (!(combine_test_out_overlap.first() == TimeRange(0, 30))) { return false; }

  TimeRangeList combine_test_both_overlap = t;
  combine_test_both_overlap.insert(TimeRange(9, 21));
  if (!(combine_test_both_overlap.size() == 1)) { return false; }
  if (!(combine_test_both_overlap.first() == TimeRange(0, 30))) { return false; }

  return true;
}

int main()
{
  Tester t;

  t.add("TimeRangeList::remove", timerangelist_remove_test);
  t.add("TimeRangeList::merge_adjacent", timerangelist_mergeadjacent_test);

  return t.exec();
}
