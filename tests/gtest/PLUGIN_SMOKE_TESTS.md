# Plugin Subsystem Smoke Tests

This document describes the comprehensive smoke tests for the Oak Video Editor plugin subsystem.

## Overview

The plugin smoke tests verify the core functionality of the OFX plugin integration without requiring actual plugin binaries. These tests ensure that the plugin host, clip management, image handling, parameter instances, and rendering pipeline work correctly.

## Test Categories

### 1. PluginSmoke (Host Tests)
- **HostSingletonExists**: Verifies the OFX plugin cache is accessible
- **LoadPluginsEmptyPathNoCrash**: Ensures empty plugin paths are handled gracefully
- **LoadPluginsNonExistentPathNoCrash**: Ensures non-existent paths don't crash

### 2. PluginSmokeClip (Clip Tests)
- **OutputClipProperties**: Tests property getters for output clips (bit depth, components, premult, aspect ratio, frame rate)
- **ClipDifferentPixelFormats**: Tests U8, U16, F16, F32 format mappings to OFX
- **SourceClipNotConnected**: Tests source clip connection state

### 3. PluginSmokeImage (Image Tests)
- **BasicAllocation**: Tests image allocation with various parameters
- **ClearOnAllocate**: Tests memory clearing on allocation
- **ResizeOnAllocate**: Tests image resizing during reallocation

### 4. PluginSmokeParam (Parameter Tests)
Tests all parameter types without node binding:
- **IntegerNullNode**: Integer parameter get/set
- **DoubleNullNode**: Double parameter get/set
- **BooleanNullNode**: Boolean parameter get/set
- **ChoiceNullNode**: Choice parameter get/set
- **RGBANullNode**: RGBA color parameter get/set
- **RGBNullNode**: RGB color parameter get/set
- **Double2DNullNode**: 2D double vector get/set
- **Integer2DNullNode**: 2D integer vector get/set
- **Double3DNullNode**: 3D double vector get/set
- **Integer3DNullNode**: 3D integer vector get/set
- **StringNullNode**: String parameter get/set
- **CustomNullNode**: Custom parameter get/set

### 5. PluginSmokeRenderer (Renderer Tests)
- **BytesToPixelsConversion**: Tests byte-to-pixel conversion for texture I/O
- **BytesToPixelsInvalidInput**: Tests handling of invalid inputs

### 6. PluginSmokeJob (Job Tests)
- **JobConstruction**: Tests basic PluginJob creation
- **JobWithTime**: Tests job creation with time parameter
- **JobWithTextureValue**: Tests job with texture input values

### 7. PluginSmokeNode (Node Tests)
- **NodeRequiresValidInstance**: Documents that PluginNode requires valid OFX instance

### 8. PluginSmokeIntegration (Integration Tests)
- **VideoParamsToOfxMapping**: Tests pixel format to OFX bit depth mapping
- **ComponentCountMapping**: Tests channel count to OFX component mapping

### 9. PluginSmokeThread (Thread Safety Tests)
- **ConcurrentImageAllocation**: Tests thread-safe image allocation (4 threads × 10 allocs)
- **ConcurrentParamAccess**: Tests thread-safe parameter access (4 threads × 100 ops)

## Running the Tests

### Run all plugin smoke tests:
```bash
./build/tests/gtest/olive-gtest --gtest_filter="PluginSmoke*"
```

### Run specific test category:
```bash
./build/tests/gtest/olive-gtest --gtest_filter="PluginSmokeParam*"
./build/tests/gtest/olive-gtest --gtest_filter="PluginSmokeClip*"
./build/tests/gtest/olive-gtest --gtest_filter="PluginSmokeThread*"
```

### Run all tests:
```bash
./build/tests/gtest/olive-gtest
```

## CI Integration

The plugin smoke tests are integrated into GitHub Actions CI and run on:
- Ubuntu Latest
- macOS Latest
- Windows 2022

The CI configuration includes a dedicated "Plugin Smoke Tests" step that runs these tests separately for clarity.

## Adding New Tests

When adding new plugin smoke tests:

1. Follow the naming convention: `PluginSmoke<Category>.<TestName>`
2. Use the helper functions in the `olive::plugin::test` namespace
3. Test both success and failure paths
4. Include thread safety tests if applicable
5. Document the test purpose in this README

## Dependencies

These tests depend on:
- OpenFX Host Support library
- Qt6 Core
- Google Test
- FFmpeg (for texture creation utilities)

No actual OFX plugins are required for smoke tests.
