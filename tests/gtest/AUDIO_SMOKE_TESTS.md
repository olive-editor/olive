# Audio Subsystem Smoke Tests

This document describes the comprehensive smoke tests for the Oak Video Editor audio subsystem.

## Overview

The audio smoke tests verify the core functionality of the audio processing pipeline including audio parameters, sample buffers, visual waveforms, audio processing, and preview device management.

## Test Categories

### 1. AudioSmokeParams (Audio Parameters Tests) - 9 tests
- **DefaultConstruction**: Verifies default AudioParams is invalid
- **ValidConstruction**: Tests construction with valid parameters
- **MonoChannelLayout**: Tests mono (1 channel) configuration
- **SurroundChannelLayout**: Tests 5.1 surround (6 channels) configuration
- **TimeConversions**: Tests time/sample/bytes conversion functions
- **EqualityOperators**: Tests == and != operators
- **CopyConstruction**: Tests proper deep copy via copy constructor
- **CopyAssignment**: Tests proper deep copy via assignment operator
- **ChannelLayoutModification**: Tests dynamic channel layout changes

### 2. AudioSmokeBuffer (Sample Buffer Tests) - 8 tests
- **DefaultConstruction**: Verifies empty buffer state
- **Allocation**: Tests memory allocation with params
- **DataAccess**: Tests data read/write operations
- **Silence**: Tests silence() function
- **VolumeTransform**: Tests volume scaling
- **Clamp**: Tests clamping to [-1, 1] range
- **FastSet**: Tests fast channel copying
- **RipChannel**: Tests extracting single channel

### 3. AudioSmokeWaveform (Visual Waveform Tests) - 11 tests
- **DefaultConstruction**: Verifies empty waveform
- **ChannelCount**: Tests channel configuration
- **OverwriteSamples**: Tests writing sample data
- **OverwriteSilence**: Tests writing silence
- **TrimIn**: Tests trimming from start
- **Resize**: Tests resizing waveform
- **TrimRange**: Tests trimming range [in, length]
- **Mid**: Tests extracting sub-section
- **GetSummaryFromTime**: Tests summary generation for display
- **SumSamples**: Tests sample summarization
- **ReSumSamples**: Tests re-summarization from mipmap

### 4. AudioSmokeProcessor (Audio Processor Tests) - 8 tests
- **DefaultConstruction**: Verifies processor starts closed
- **OpenClose**: Tests basic open/close lifecycle
- **SampleRateConversion**: Tests 48kHz to 44.1kHz conversion
- **ChannelLayoutConversion**: Tests stereo to mono conversion
- **FormatConversion**: Tests F32P to S16P conversion
- **TempoChange**: Tests 2x tempo adjustment
- **InvalidOpen**: Tests double-open prevention
- **ConvertWithoutOpen**: Tests error handling for closed processor

### 5. AudioSmokePreviewDevice (Preview Device Tests) - 4 tests
- **Construction**: Tests QIODevice initialization
- **BytesPerFrame**: Tests bytes per frame getter/setter
- **NotifyInterval**: Tests notification interval setting
- **Clear**: Tests buffer clearing

### 6. AudioSmokeSampleFormat (Sample Format Tests) - 3 tests
- **ByteCount**: Tests bytes per sample for all formats
- **PackedVsPlanar**: Tests packed/planar format detection
- **StringConversion**: Tests format name conversions

### 7. AudioSmokeThread (Thread Safety Tests) - 2 tests
- **ConcurrentWaveformAccess**: Tests 4 threads reading waveform concurrently
- **ConcurrentSampleBufferOperations**: Tests 4 threads performing different buffer operations

## Bug Fixes

### Fixed: PreviewAudioDevice Uninitialized Member

**File**: `app/render/previewaudiodevice.cpp`

**Problem**: The `bytes_per_frame_` member was not initialized in the constructor, leading to undefined behavior.

**Fix**:
```cpp
PreviewAudioDevice::PreviewAudioDevice(QObject *parent)
	: QIODevice(parent)      // Added: Properly initialize QIODevice base
	, bytes_per_frame_(0)     // Added: Initialize member
	, notify_interval_(0)
	, bytes_read_(0)
{
}
```

## Running the Tests

### Run all audio smoke tests:
```bash
./build/tests/gtest/olive-gtest --gtest_filter="AudioSmoke*"
```

### Run specific test category:
```bash
./build/tests/gtest/olive-gtest --gtest_filter="AudioSmokeParams*"
./build/tests/gtest/olive-gtest --gtest_filter="AudioSmokeBuffer*"
./build/tests/gtest/olive-gtest --gtest_filter="AudioSmokeWaveform*"
./build/tests/gtest/olive-gtest --gtest_filter="AudioSmokeProcessor*"
```

### Run all tests:
```bash
./build/tests/gtest/olive-gtest
```

## Test Statistics

- **Total Tests**: 45
- **Test Suites**: 7
- **Coverage Areas**:
  - AudioParams: 9 tests
  - SampleBuffer: 8 tests
  - AudioVisualWaveform: 11 tests
  - AudioProcessor: 8 tests
  - PreviewAudioDevice: 4 tests
  - SampleFormat: 3 tests
  - Thread Safety: 2 tests

## Dependencies

These tests depend on:
- olive::core (AudioParams, SampleBuffer, SampleFormat)
- FFmpeg (libavutil for channel layouts)
- Qt6 Core
- Google Test
- PortAudio (for AudioManager - minimal testing)

## Thread Safety Notes

The audio subsystem has the following thread safety characteristics:

1. **AudioParams**: Thread-safe for read operations after construction
2. **SampleBuffer**: NOT thread-safe - external synchronization required
3. **AudioVisualWaveform**: Read operations are thread-safe, writes need synchronization
4. **AudioProcessor**: NOT thread-safe - single thread access only
5. **PreviewAudioDevice**: Uses QMutex for internal synchronization

The thread safety tests verify that concurrent reads work correctly, but do NOT test concurrent read/write scenarios which require external locking.
