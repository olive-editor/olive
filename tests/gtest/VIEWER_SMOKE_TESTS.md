# Viewer/Preview Display Subsystem Smoke Tests

This document describes the comprehensive smoke tests for the Oak Video Editor viewer and preview display subsystem.

## Overview

The viewer smoke tests verify the core functionality of the preview display system including playback timing, frame queue management, safe margin calculations, and audio caching.

## Bug Fixes

### 1. Fixed: ViewerSafeMarginInfo Uninitialized Member

**File**: `app/widget/viewer/viewersafemargininfo.h`

**Problem**: The `ratio_` member was not initialized in the default constructor, causing undefined behavior.

**Fix**:
```cpp
ViewerSafeMarginInfo()
    : enabled_(false)
    , ratio_(0.0)  // Added: Initialize ratio_
{
}
```

### 2. Fixed: Audio Playback Crash on Invalid Params

**File**: `app/widget/viewer/viewer.cpp`

**Problem**: `PlayInternal()` was calling `time_to_bytes()` on `audio_processor_.to()` without checking if the params were valid, causing an assertion failure when audio output configuration was invalid.

**Fix**:
```cpp
// Verify audio processor output params are valid before using them
AudioParams output_params = audio_processor_.to();
if (!output_params.is_valid()) {
    qWarning() << "Audio processor output params are invalid, skipping audio playback";
} else {
    AudioManager::instance()->SetOutputNotifyInterval(
        output_params.time_to_bytes(kAudioPlaybackInterval));
    // ... rest of audio setup
}
```

### 3. Fixed: ViewerPlaybackFrame Metatype Registration

**File**: `app/widget/viewer/viewerqueue.h`

**Problem**: `ViewerPlaybackFrame` structure using `QVariant` was not properly registered with Qt's metatype system.

**Fix**: Added `#include <QVariant>` and `Q_DECLARE_METATYPE(olive::ViewerPlaybackFrame)` at global scope.

## Test Categories

### 1. ViewerSmokeTimer (Playback Timer Tests) - 6 tests
- **DefaultConstruction**: Verifies timer starts in valid state
- **BasicTiming**: Tests basic timestamp calculation at 24fps
- **PlaybackSpeedForward**: Tests 2x forward playback speed
- **PlaybackSpeedReverse**: Tests reverse playback (-1x speed)
- **DifferentTimebases**: Tests timing at different frame rates (24fps, 60fps)
- **ZeroSpeed**: Tests paused state (0 speed)

### 2. ViewerSmokeQueue (Frame Queue Tests) - 6 tests
- **DefaultConstruction**: Verifies empty queue state
- **AppendForwardPlayback**: Tests frame ordering for forward playback
- **AppendReversePlayback**: Tests frame ordering for reverse playback
- **InsertOutOfOrder**: Tests automatic sorting of out-of-order frames
- **PurgeBefore**: Tests removing old frames during forward playback
- **PurgeBeforeReverse**: Tests removing old frames during reverse playback

### 3. ViewerSmokeSafeMargin (Safe Margin Tests) - 6 tests
- **DefaultConstruction**: Tests default disabled state with initialized ratio
- **EnabledConstruction**: Tests enabled state without custom ratio
- **CustomRatioConstruction**: Tests enabled state with custom aspect ratio
- **EqualityOperators**: Tests == and != operators
- **ZeroRatio**: Tests that ratio of 0 means no custom ratio
- **CopyConstruction**: Tests proper copying of margin info

### 4. ViewerSmokeAudioCache (Audio Cache Tests) - 3 tests
- **DefaultConstruction**: Tests cache initialization
- **ParameterSetters**: Tests setting and retrieving audio parameters
- **ValidateWithRange**: Tests validated range tracking

### 5. ViewerSmokeAutoCacher (Auto Cacher Tests) - 5 tests (DISABLED)
These tests require a full GUI environment and are disabled in headless mode:
- **Construction**: Tests basic cacher creation
- **SetPlayhead**: Tests playhead position updates
- **PauseControls**: Tests render/thumbnail pause controls
- **SetIgnoreCacheRequests**: Tests cache request ignoring
- **SetDisplayColorProcessor**: Tests color processor setting

### 6. ViewerSmokeRational (Rational Number Tests) - 7 tests
- **DefaultConstruction**: Tests 0/1 default value
- **ValueConstruction**: Tests fraction construction
- **ToDouble**: Tests conversion to double
- **Arithmetic**: Tests addition and subtraction
- **Comparison**: Tests equality and ordering
- **NullCheck**: Tests null detection (0 numerator)
- **Flipped**: Tests reciprocal calculation

### 7. ViewerSmokeThread (Thread Safety Tests) - 2 tests
- **ConcurrentTimerAccess**: Tests 4 threads reading timer concurrently
- **ConcurrentQueueAccess**: Tests 4 threads modifying queue concurrently

### 8. ViewerSmokeIntegration (Integration Tests) - 3 tests
- **PlaybackSequenceSimulation**: Tests timer + queue interaction
- **SafeMarginWithDifferentAspectRatios**: Tests various aspect ratios
- **ReversePlaybackScenario**: Tests full reverse playback setup

## Running the Tests

### Run all viewer smoke tests:
```bash
./build/tests/gtest/olive-gtest --gtest_filter="ViewerSmoke*"
```

### Run specific test category:
```bash
./build/tests/gtest/olive-gtest --gtest_filter="ViewerSmokeTimer*"
./build/tests/gtest/olive-gtest --gtest_filter="ViewerSmokeQueue*"
./build/tests/gtest/olive-gtest --gtest_filter="ViewerSmokeSafeMargin*"
```

### Run all tests:
```bash
./build/tests/gtest/olive-gtest
```

## Test Statistics

- **Total Tests**: 33 (plus 5 disabled)
- **Test Suites**: 7
- **Coverage Areas**:
  - Playback Timer: 6 tests
  - Frame Queue: 6 tests
  - Safe Margin: 6 tests
  - Audio Cache: 3 tests
  - Auto Cacher: 5 tests (disabled in CI)
  - Rational Math: 7 tests
  - Thread Safety: 2 tests
  - Integration: 3 tests

## Dependencies

These tests depend on:
- Qt6 Core (QTimer, QThread)
- olive::core (rational)
- Google Test
- FFmpeg (for AudioParams channel layouts)

## Thread Safety Notes

The viewer subsystem has the following thread safety characteristics:

1. **ViewerPlaybackTimer**: Thread-safe for concurrent reads, but Start() should not be called concurrently with GetTimestampNow()
2. **ViewerQueue**: NOT thread-safe - requires external synchronization for concurrent modifications
3. **ViewerSafeMarginInfo**: Thread-safe for read-only access after construction
4. **AudioPlaybackCache**: Uses internal locking for thread safety

The thread safety tests verify basic concurrent access patterns, but production code should use proper locking for producer-consumer scenarios.
