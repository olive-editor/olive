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

#include "testutil.h"

#include "common/timerange.h"

namespace olive {

OLIVE_ADD_TEST(TimeRangeListMergeAdjacent)
{
  TimeRangeList t;

  // TimeRangeList should merge 1 and 3 together since they're adjacent
  t.insert(TimeRange(0, 6));
  t.insert(TimeRange(20, 30));
  t.insert(TimeRange(6, 10));

  OLIVE_ASSERT(t.size() == 2);
  OLIVE_ASSERT(t.first() == TimeRange(20, 30));
  OLIVE_ASSERT(t.at(1) == TimeRange(0, 10));

  // TimeRangeList should ignore these because it's already contained
  TimeRangeList noop_test = t;

  noop_test.insert(TimeRange(4, 7));
  OLIVE_ASSERT(noop_test == t);

  noop_test.insert(TimeRange(0, 3));
  OLIVE_ASSERT(noop_test == t);

  noop_test.insert(TimeRange(25, 30));
  OLIVE_ASSERT(noop_test == t);

  // TimeRangeList should combine all these together
  TimeRangeList combine_test_no_overlap = t;
  combine_test_no_overlap.insert(TimeRange(10, 20));
  OLIVE_ASSERT(combine_test_no_overlap.size() == 1);
  OLIVE_ASSERT(combine_test_no_overlap.first() == TimeRange(0, 30));

  TimeRangeList combine_test_in_overlap = t;
  combine_test_in_overlap.insert(TimeRange(9, 20));
  OLIVE_ASSERT(combine_test_in_overlap.size() == 1);
  OLIVE_ASSERT(combine_test_in_overlap.first() == TimeRange(0, 30));

  TimeRangeList combine_test_out_overlap = t;
  combine_test_out_overlap.insert(TimeRange(10, 21));
  OLIVE_ASSERT(combine_test_out_overlap.size() == 1);
  OLIVE_ASSERT(combine_test_out_overlap.first() == TimeRange(0, 30));

  TimeRangeList combine_test_both_overlap = t;
  combine_test_both_overlap.insert(TimeRange(9, 21));
  OLIVE_ASSERT(combine_test_both_overlap.size() == 1);
  OLIVE_ASSERT(combine_test_both_overlap.first() == TimeRange(0, 30));

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(TimeRangeListFrameIteratorSize)
{
  const rational timebase(1, 10);

  TimeRangeList ranges;

  ranges.insert(TimeRange(0, 10));                                      // 100
  ranges.insert(TimeRange(25, 30));                                     // 50
  ranges.insert(TimeRange(50, 60));                                     // 100
  ranges.insert(TimeRange(70, rational(1401, 20)));                     // 1
  ranges.insert(TimeRange(rational(1402, 20), rational(1403, 20)));     // 1
  ranges.insert(TimeRange(rational(10001, 40), rational(10002, 40)));   // 0
  ranges.insert(TimeRange(rational(10001, 40), rational(10004, 40)));   // 0
  ranges.insert(TimeRange(rational(10001, 40), rational(10005, 40)));   // 1

  TimeRangeListFrameIterator iterator(ranges, timebase);

  QVector<rational> vec = iterator.ToVector();

  OLIVE_ASSERT_EQUAL(vec.size(), 253);
  OLIVE_ASSERT_EQUAL(iterator.size(), vec.size());

  TimeRangeListFrameIterator empty(TimeRangeList(), timebase);

  QVector<rational> empty_vec = empty.ToVector();

  OLIVE_ASSERT_EQUAL(empty_vec.size(), 0);
  OLIVE_ASSERT_EQUAL(empty_vec.size(), empty.size());

  OLIVE_TEST_END;
}

}
