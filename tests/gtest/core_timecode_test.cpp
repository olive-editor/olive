#include <gtest/gtest.h>

#include "olive/core/util/timecodefunctions.h"

using namespace olive::core;

TEST(CoreTimecode, TimeToTimecodeSeconds)
{
	rational time(5, 1);
	rational tb(1, 25);
	std::string tc =
		Timecode::time_to_timecode(time, tb, Timecode::kTimecodeSeconds);
	EXPECT_EQ(tc, "00:00:05.000");
}

TEST(CoreTimecode, TimeToTimecodeNonDropFrame)
{
	rational time(2, 1);
	rational tb(1, 25);
	std::string tc =
		Timecode::time_to_timecode(time, tb, Timecode::kTimecodeNonDropFrame);
	EXPECT_EQ(tc, "00:00:02:00");
}

TEST(CoreTimecode, TimeToTimecodePlusSign)
{
	rational time(1, 1);
	rational tb(1, 25);
	std::string tc =
		Timecode::time_to_timecode(time, tb, Timecode::kTimecodeSeconds, true);
	EXPECT_EQ(tc.substr(0, 1), "+");
}

TEST(CoreTimecode, TimeToTimecodeInvalidTimebase)
{
	rational time(1, 1);
	EXPECT_EQ(Timecode::time_to_timecode(time, rational(), Timecode::kFrames),
			  "INVALID TIMEBASE");
}

TEST(CoreTimecode, TimecodeToTimeSeconds)
{
	rational tb(1, 25);
	bool ok = false;
	rational t = Timecode::timecode_to_time("00:00:05.500", tb,
											Timecode::kTimecodeSeconds, &ok);
	EXPECT_TRUE(ok);
	EXPECT_EQ(t, rational(11, 2));
}

TEST(CoreTimecode, TimecodeToTimeNonDropFrame)
{
	rational tb(1, 25);
	bool ok = false;
	rational t = Timecode::timecode_to_time(
		"00:00:02:03", tb, Timecode::kTimecodeNonDropFrame, &ok);
	EXPECT_TRUE(ok);
	EXPECT_EQ(t, rational(53, 25));
}

TEST(CoreTimecode, TimecodeToTimeInvalid)
{
	rational tb(1, 25);
	bool ok = true;
	Timecode::timecode_to_time("not a timecode", tb, Timecode::kTimecodeSeconds,
							   &ok);
	EXPECT_FALSE(ok);
}

TEST(CoreTimecode, TimeToString)
{
	EXPECT_EQ(Timecode::time_to_string(3661000), "01:01:01");
}

TEST(CoreTimecode, SnapTimeToTimebase)
{
	rational tb(1, 25);
	rational snapped = Timecode::snap_time_to_timebase(rational(1, 10), tb);
	// 0.1s @ 25fps rounds to frame 3 (0.12s)
	EXPECT_EQ(snapped, rational(3, 25));
}

TEST(CoreTimecode, TimeToTimestamp)
{
	rational tb(1, 25);
	EXPECT_EQ(Timecode::time_to_timestamp(rational(2, 1), tb), 50);
	EXPECT_EQ(Timecode::time_to_timestamp(0.08, tb, Timecode::kFloor), 2);
	EXPECT_EQ(Timecode::time_to_timestamp(0.08, tb, Timecode::kCeil), 2);
}

TEST(CoreTimecode, TimestampToTime)
{
	rational tb(1, 25);
	EXPECT_EQ(Timecode::timestamp_to_time(50, tb), rational(2, 1));
}

TEST(CoreTimecode, RescaleTimestamp)
{
	rational src(1, 25);
	rational dst(1, 30);
	EXPECT_EQ(Timecode::rescale_timestamp(50, src, dst), 60);
	EXPECT_EQ(Timecode::rescale_timestamp(50, src, src), 50);
}

TEST(CoreTimecode, RescaleTimestampCeil)
{
	rational src(1, 25);
	rational dst(1, 30);
	EXPECT_EQ(Timecode::rescale_timestamp_ceil(1, src, dst), 2);
}

TEST(CoreTimecode, TimebaseIsDropFrame)
{
	EXPECT_FALSE(Timecode::timebase_is_drop_frame(rational(1, 25)));
	EXPECT_TRUE(Timecode::timebase_is_drop_frame(rational(1001, 30000)));
}
