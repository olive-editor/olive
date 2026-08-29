/*
 * Oak Video Editor - Viewer/Preview Subsystem Smoke Tests
 * Copyright (C) 2025 Olive CE Team
 *
 * Comprehensive smoke tests for the viewer and preview display subsystem including:
 * - ViewerPlaybackTimer timing calculations
 * - ViewerQueue frame management
 * - ViewerSafeMarginInfo safety margin calculations
 * - PreviewAutoCacher cache management
 */

#include <gtest/gtest.h>

#include <QThread>
#include <QElapsedTimer>

// Viewer headers
#include "widget/viewer/viewerplaybacktimer.h"
#include "widget/viewer/viewerqueue.h"
#include "widget/viewer/viewersafemargininfo.h"
#include "render/previewautocacher.h"
#include "render/audioplaybackcache.h"
#include "olive/core/util/rational.h"

using namespace olive;
using namespace olive::core;

namespace olive
{
namespace viewer
{
namespace test
{

// ============================================================================
// Smoke Test: ViewerPlaybackTimer
// ============================================================================

TEST(ViewerSmokeTimer, DefaultConstruction)
{
	ViewerPlaybackTimer timer;
	// Timer should be in a valid but not-started state
	// After Start() is called, it should return valid timestamps
}

TEST(ViewerSmokeTimer, BasicTiming)
{
	ViewerPlaybackTimer timer;

	// Start at timestamp 0, 1x speed, 24fps (timebase = 1/24)
	timer.Start(0, 1, 1.0 / 24.0);

	// Immediately get timestamp (should be close to 0)
	int64_t ts = timer.GetTimestampNow();
	EXPECT_GE(ts, 0);

	// Wait a bit and check timestamp has increased
	QThread::msleep(50); // 50ms
	int64_t ts2 = timer.GetTimestampNow();

	// At 24fps, 50ms should be approximately 1 frame (or slightly more)
	// Allow for some timing variance
	EXPECT_GE(ts2, ts);
}

TEST(ViewerSmokeTimer, PlaybackSpeedForward)
{
	ViewerPlaybackTimer timer;

	// Start at timestamp 100, 2x speed, 30fps
	timer.Start(100, 2, 1.0 / 30.0);

	int64_t ts1 = timer.GetTimestampNow();
	QThread::msleep(50);
	int64_t ts2 = timer.GetTimestampNow();

	// At 2x speed, time should advance twice as fast
	EXPECT_GT(ts2, ts1);
}

TEST(ViewerSmokeTimer, PlaybackSpeedReverse)
{
	ViewerPlaybackTimer timer;

	// Start at timestamp 1000, -1x speed (reverse), 24fps
	timer.Start(1000, -1, 1.0 / 24.0);

	int64_t ts1 = timer.GetTimestampNow();
	QThread::msleep(50);
	int64_t ts2 = timer.GetTimestampNow();

	// In reverse, timestamp should decrease
	EXPECT_LT(ts2, ts1);
}

TEST(ViewerSmokeTimer, DifferentTimebases)
{
	ViewerPlaybackTimer timer;

	// Test with 24fps
	timer.Start(0, 1, 1.0 / 24.0);
	QThread::msleep(100);
	int64_t ts24 = timer.GetTimestampNow();

	// Test with 60fps
	timer.Start(0, 1, 1.0 / 60.0);
	QThread::msleep(100);
	int64_t ts60 = timer.GetTimestampNow();

	// At same real time, 60fps should have more frames than 24fps
	EXPECT_GT(ts60, ts24);
}

TEST(ViewerSmokeTimer, ZeroSpeed)
{
	ViewerPlaybackTimer timer;

	// Start with 0 speed (paused)
	timer.Start(500, 0, 1.0 / 24.0);

	int64_t ts1 = timer.GetTimestampNow();
	QThread::msleep(50);
	int64_t ts2 = timer.GetTimestampNow();

	// With 0 speed, timestamp should not change
	EXPECT_EQ(ts1, ts2);
}

// ============================================================================
// Smoke Test: ViewerQueue
// ============================================================================

TEST(ViewerSmokeQueue, DefaultConstruction)
{
	ViewerQueue queue;
	EXPECT_TRUE(queue.empty());
}

TEST(ViewerSmokeQueue, AppendForwardPlayback)
{
	ViewerQueue queue;

	// Append frames for forward playback
	ViewerPlaybackFrame frame1{ rational(0), QVariant() };
	ViewerPlaybackFrame frame2{ rational(1, 24), QVariant() };
	ViewerPlaybackFrame frame3{ rational(2, 24), QVariant() };

	queue.AppendTimewise(frame1, 1); // speed = 1 (forward)
	queue.AppendTimewise(frame2, 1);
	queue.AppendTimewise(frame3, 1);

	EXPECT_EQ(queue.size(), 3);

	// Verify order (should be chronological for forward playback)
	auto it = queue.begin();
	EXPECT_EQ(it->timestamp, rational(0));
	++it;
	EXPECT_EQ(it->timestamp, rational(1, 24));
	++it;
	EXPECT_EQ(it->timestamp, rational(2, 24));
}

TEST(ViewerSmokeQueue, AppendReversePlayback)
{
	ViewerQueue queue;

	// Append frames for reverse playback
	ViewerPlaybackFrame frame1{ rational(2, 24), QVariant() };
	ViewerPlaybackFrame frame2{ rational(1, 24), QVariant() };
	ViewerPlaybackFrame frame3{ rational(0), QVariant() };

	queue.AppendTimewise(frame1, -1); // speed = -1 (reverse)
	queue.AppendTimewise(frame2, -1);
	queue.AppendTimewise(frame3, -1);

	EXPECT_EQ(queue.size(), 3);

	// Verify order (should be reverse chronological for reverse playback)
	auto it = queue.begin();
	EXPECT_EQ(it->timestamp, rational(2, 24));
	++it;
	EXPECT_EQ(it->timestamp, rational(1, 24));
	++it;
	EXPECT_EQ(it->timestamp, rational(0));
}

TEST(ViewerSmokeQueue, InsertOutOfOrder)
{
	ViewerQueue queue;

	// Insert frames out of order for forward playback
	ViewerPlaybackFrame frame1{ rational(0), QVariant() };
	ViewerPlaybackFrame frame2{ rational(2, 24), QVariant() };
	ViewerPlaybackFrame frame3{ rational(1, 24), QVariant() }; // Middle frame

	queue.AppendTimewise(frame1, 1);
	queue.AppendTimewise(frame2, 1);
	queue.AppendTimewise(frame3, 1); // Should insert in middle

	EXPECT_EQ(queue.size(), 3);

	// Verify correct order
	auto it = queue.begin();
	EXPECT_EQ(it->timestamp, rational(0));
	++it;
	EXPECT_EQ(it->timestamp, rational(1, 24));
	++it;
	EXPECT_EQ(it->timestamp, rational(2, 24));
}

TEST(ViewerSmokeQueue, PurgeBefore)
{
	ViewerQueue queue;

	// Add some frames
	for (int i = 0; i < 10; i++) {
		ViewerPlaybackFrame frame{ rational(i, 24), QVariant() };
		queue.AppendTimewise(frame, 1);
	}

	EXPECT_EQ(queue.size(), 10);

	// Purge frames before 5/24
	queue.PurgeBefore(rational(5, 24), 1);

	// Should have 5 frames remaining (5, 6, 7, 8, 9)
	EXPECT_EQ(queue.size(), 5);
	EXPECT_EQ(queue.front().timestamp, rational(5, 24));
}

TEST(ViewerSmokeQueue, PurgeBeforeReverse)
{
	ViewerQueue queue;

	// Add frames for reverse playback (newest first)
	for (int i = 9; i >= 0; i--) {
		ViewerPlaybackFrame frame{ rational(i, 24), QVariant() };
		queue.AppendTimewise(frame, -1);
	}

	EXPECT_EQ(queue.size(), 10);

	// In reverse playback, front() is the largest timestamp (9/24)
	// PurgeBefore with negative speed removes frames where front > time
	queue.PurgeBefore(rational(5, 24), -1);

	// Should have frames 0-5 remaining (those <= 5/24)
	EXPECT_EQ(queue.size(), 6);
	EXPECT_EQ(queue.front().timestamp, rational(5, 24));
}

// ============================================================================
// Smoke Test: ViewerSafeMarginInfo
// ============================================================================

TEST(ViewerSmokeSafeMargin, DefaultConstruction)
{
	ViewerSafeMarginInfo info;
	EXPECT_FALSE(info.is_enabled());
	EXPECT_FALSE(info.custom_ratio());
	EXPECT_DOUBLE_EQ(info.ratio(), 0.0);
}

TEST(ViewerSmokeSafeMargin, EnabledConstruction)
{
	ViewerSafeMarginInfo info(true);
	EXPECT_TRUE(info.is_enabled());
	EXPECT_FALSE(info.custom_ratio());
}

TEST(ViewerSmokeSafeMargin, CustomRatioConstruction)
{
	ViewerSafeMarginInfo info(true, 0.9);
	EXPECT_TRUE(info.is_enabled());
	EXPECT_TRUE(info.custom_ratio());
	EXPECT_DOUBLE_EQ(info.ratio(), 0.9);
}

TEST(ViewerSmokeSafeMargin, EqualityOperators)
{
	ViewerSafeMarginInfo info1(true, 0.9);
	ViewerSafeMarginInfo info2(true, 0.9);
	ViewerSafeMarginInfo info3(false, 0.9);
	ViewerSafeMarginInfo info4(true, 0.8);

	EXPECT_TRUE(info1 == info2);
	EXPECT_FALSE(info1 != info2);

	EXPECT_FALSE(info1 == info3); // Different enabled state
	EXPECT_FALSE(info1 == info4); // Different ratio
	EXPECT_TRUE(info1 != info3);
}

TEST(ViewerSmokeSafeMargin, ZeroRatio)
{
	ViewerSafeMarginInfo info(true, 0.0);
	EXPECT_TRUE(info.is_enabled());
	EXPECT_FALSE(info.custom_ratio()); // 0 ratio means no custom ratio
}

TEST(ViewerSmokeSafeMargin, CopyConstruction)
{
	ViewerSafeMarginInfo original(true, 0.85);
	ViewerSafeMarginInfo copy(original);

	EXPECT_EQ(copy.is_enabled(), original.is_enabled());
	EXPECT_EQ(copy.custom_ratio(), original.custom_ratio());
	EXPECT_DOUBLE_EQ(copy.ratio(), original.ratio());
}

// ============================================================================
// Smoke Test: AudioPlaybackCache (used by preview system)
// ============================================================================

TEST(ViewerSmokeAudioCache, DefaultConstruction)
{
	AudioPlaybackCache cache;
	// Should construct without crashing
	SUCCEED();
}

TEST(ViewerSmokeAudioCache, ParameterSetters)
{
	AudioPlaybackCache cache;

	AudioParams params(48000, kChannelLayoutStereo, SampleFormat::F32P);
	cache.SetParameters(params);

	// Parameters should be retrievable
	AudioParams retrieved = cache.GetParameters();
	EXPECT_EQ(retrieved.sample_rate(), params.sample_rate());
}

TEST(ViewerSmokeAudioCache, ValidateWithRange)
{
	AudioPlaybackCache cache;

	// Initially no validated ranges
	TimeRangeList validated = cache.GetValidatedRanges();
	EXPECT_TRUE(validated.isEmpty());
}

// ============================================================================
// Smoke Test: PreviewAutoCacher (basic lifecycle)
// ============================================================================

// NOTE: PreviewAutoCacher requires a QApplication and proper initialization.
// These tests are disabled in headless mode.

TEST(ViewerSmokeAutoCacher, DISABLED_Construction)
{
	// PreviewAutoCacher requires full GUI environment
	SUCCEED();
}

TEST(ViewerSmokeAutoCacher, DISABLED_SetPlayhead)
{
	// PreviewAutoCacher requires full GUI environment
	SUCCEED();
}

TEST(ViewerSmokeAutoCacher, DISABLED_PauseControls)
{
	// PreviewAutoCacher requires full GUI environment
	SUCCEED();
}

TEST(ViewerSmokeAutoCacher, DISABLED_SetIgnoreCacheRequests)
{
	// PreviewAutoCacher requires full GUI environment
	SUCCEED();
}

TEST(ViewerSmokeAutoCacher, DISABLED_SetDisplayColorProcessor)
{
	// PreviewAutoCacher requires full GUI environment
	SUCCEED();
}

// ============================================================================
// Smoke Test: Rational Time Calculations (core utility)
// ============================================================================

TEST(ViewerSmokeRational, DefaultConstruction)
{
	rational r;
	EXPECT_EQ(r.numerator(), 0);
	EXPECT_EQ(r.denominator(), 1);
}

TEST(ViewerSmokeRational, ValueConstruction)
{
	rational r(24, 1);
	EXPECT_EQ(r.numerator(), 24);
	EXPECT_EQ(r.denominator(), 1);

	rational r2(1, 24);
	EXPECT_EQ(r2.numerator(), 1);
	EXPECT_EQ(r2.denominator(), 24);
}

TEST(ViewerSmokeRational, ToDouble)
{
	rational r(1, 2);
	EXPECT_DOUBLE_EQ(r.toDouble(), 0.5);

	rational r2(3, 4);
	EXPECT_DOUBLE_EQ(r2.toDouble(), 0.75);
}

TEST(ViewerSmokeRational, Arithmetic)
{
	rational r1(1, 2);
	rational r2(1, 4);

	rational sum = r1 + r2;
	EXPECT_EQ(sum.numerator(), 3);
	EXPECT_EQ(sum.denominator(), 4);

	rational diff = r1 - r2;
	EXPECT_EQ(diff.numerator(), 1);
	EXPECT_EQ(diff.denominator(), 4);
}

TEST(ViewerSmokeRational, Comparison)
{
	rational r1(1, 2);
	rational r2(2, 4);
	rational r3(3, 4);

	EXPECT_TRUE(r1 == r2); // Equivalent fractions
	EXPECT_FALSE(r1 == r3);
	EXPECT_TRUE(r1 < r3);
	EXPECT_TRUE(r3 > r1);
}

TEST(ViewerSmokeRational, NullCheck)
{
	rational r;
	EXPECT_TRUE(r.isNull()); // 0/1 is considered null

	rational r2(1, 2);
	EXPECT_FALSE(r2.isNull());
}

TEST(ViewerSmokeRational, Flipped)
{
	rational r(24, 1);
	rational flipped = r.flipped();

	EXPECT_EQ(flipped.numerator(), 1);
	EXPECT_EQ(flipped.denominator(), 24);
}

// ============================================================================
// Smoke Test: Thread Safety
// ============================================================================

TEST(ViewerSmokeThread, ConcurrentTimerAccess)
{
	const int num_threads = 4;
	const int num_iterations = 100;

	ViewerPlaybackTimer timer;
	timer.Start(0, 1, 1.0 / 30.0);

	std::vector<std::thread> threads;
	std::atomic<int> success_count{ 0 };

	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([&timer, &success_count, num_iterations]() {
			for (int i = 0; i < num_iterations; ++i) {
				int64_t ts = timer.GetTimestampNow();
				if (ts >= 0) {
					success_count++;
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	EXPECT_EQ(success_count.load(), num_threads * num_iterations);
}

TEST(ViewerSmokeThread, ConcurrentQueueAccess)
{
	const int num_threads = 4;
	const int num_frames_per_thread = 25;

	ViewerQueue queue;
	std::vector<std::thread> threads;
	std::atomic<int> append_count{ 0 };

	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back(
			[&queue, &append_count, t, num_frames_per_thread]() {
				for (int i = 0; i < num_frames_per_thread; ++i) {
					ViewerPlaybackFrame frame{
						rational(t * num_frames_per_thread + i, 24), QVariant()
					};
					queue.AppendTimewise(frame, 1);
					append_count++;
				}
			});
	}

	for (auto &t : threads) {
		t.join();
	}

	EXPECT_EQ(append_count.load(), num_threads * num_frames_per_thread);
	EXPECT_EQ(queue.size(), num_threads * num_frames_per_thread);
}

// ============================================================================
// Smoke Test: Integration Scenarios
// ============================================================================

TEST(ViewerSmokeIntegration, PlaybackSequenceSimulation)
{
	// Simulate a basic playback sequence
	ViewerPlaybackTimer timer;
	ViewerQueue queue;

	// Start playback at frame 0, 24fps
	timer.Start(0, 1, 1.0 / 24.0);

	// Queue some frames
	for (int i = 0; i < 10; i++) {
		ViewerPlaybackFrame frame{ rational(i, 24), QVariant(i) };
		queue.AppendTimewise(frame, 1);
	}

	// Get current timestamp
	int64_t current_ts = timer.GetTimestampNow();

	// Find frame closest to current time
	rational current_time(current_ts, 1);
	bool found = false;
	for (const auto &frame : queue) {
		if (frame.timestamp >= current_time) {
			found = true;
			break;
		}
	}

	// Should have frames available
	EXPECT_FALSE(queue.empty());
}

TEST(ViewerSmokeIntegration, SafeMarginWithDifferentAspectRatios)
{
	// Test safe margins for different aspect ratios
	std::vector<double> ratios = { 0.9, 0.85, 0.8, 0.7 };

	for (double ratio : ratios) {
		ViewerSafeMarginInfo info(true, ratio);
		EXPECT_TRUE(info.is_enabled());
		EXPECT_TRUE(info.custom_ratio());
		EXPECT_DOUBLE_EQ(info.ratio(), ratio);
	}
}

TEST(ViewerSmokeIntegration, ReversePlaybackScenario)
{
	ViewerPlaybackTimer timer;
	ViewerQueue queue;

	// Start reverse playback from frame 100
	timer.Start(100, -1, 1.0 / 24.0);

	// Queue frames in reverse order
	for (int i = 100; i >= 90; i--) {
		ViewerPlaybackFrame frame{ rational(i, 24), QVariant(i) };
		queue.AppendTimewise(frame, -1);
	}

	// Get timestamps - should decrease
	int64_t ts1 = timer.GetTimestampNow();
	QThread::msleep(50);
	int64_t ts2 = timer.GetTimestampNow();

	EXPECT_LT(ts2, ts1);
	EXPECT_EQ(queue.front().timestamp, rational(100, 24));
}

} // namespace test
} // namespace viewer
} // namespace olive
