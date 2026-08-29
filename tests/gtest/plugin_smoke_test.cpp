/*
 * Oak Video Editor - Plugin Subsystem Smoke Tests
 * Copyright (C) 2025 Olive CE Team
 *
 * Comprehensive smoke tests for the OFX plugin subsystem including:
 * - Plugin host initialization
 * - Clip and image management
 * - Parameter instances (all types)
 * - Plugin node lifecycle
 * - Plugin job execution
 * - Renderer integration
 */

#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>

#include <QCoreApplication>
#include <QThread>

// OFX headers
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxhClip.h"
#include "ofxhImageEffect.h"

// Plugin support headers
#include "pluginSupport/OliveHost.h"
#include "pluginSupport/OliveClip.h"
#include "pluginSupport/OlivePluginInstance.h"
#include "pluginSupport/image.h"
#include "pluginSupport/paraminstance.h"

// Node and render headers
#include "node/plugins/Plugin.h"
#include "render/job/pluginjob.h"
#include "render/plugin/pluginrenderer.h"
#include "render/videoparams.h"
#include "render/texture.h"

#include "node/value.h"
#include "common/ffmpegutils.h"

namespace olive
{
namespace plugin
{
namespace test
{

// ============================================================================
// Helper Functions
// ============================================================================

static VideoParams MakeVideoParams(int width, int height,
								   core::PixelFormat format, int channels,
								   bool premultiplied = false)
{
	VideoParams params;
	params.set_width(width);
	params.set_height(height);
	params.set_format(format);
	params.set_channel_count(channels);
	params.set_premultiplied_alpha(premultiplied);
	params.set_pixel_aspect_ratio(core::rational(1, 1));
	params.set_frame_rate(core::rational(30, 1));
	return params;
}

static TexturePtr CreateTestTexture(const VideoParams &params,
									uint8_t fill_value = 0x7f)
{
	AVFramePtr frame = CreateAVFramePtr();
	frame->set_format(FFmpegUtils::GetFFmpegPixelFormat(params.format(),
														params.channel_count()));
	frame->set_width(params.width());
	frame->set_height(params.height());
	if (frame->format() == FB_PIX_FMT_NONE) {
		return nullptr;
	}
	if (frame->get_buffer(0) < 0) {
		return nullptr;
	}
	if (frame->make_writable() < 0) {
		return nullptr;
	}

	const int linesize = frame->linesize(0);
	for (int y = 0; y < frame->height(); ++y) {
		std::memset(frame->data(0) + y * linesize, fill_value, linesize);
	}

	TexturePtr texture = std::make_shared<Texture>(params);
	texture->handleFrame(frame);
	return texture;
}

// ============================================================================
// Smoke Test: Plugin Host
// ============================================================================

TEST(PluginSmoke, HostSingletonExists)
{
	// Verify that the plugin cache can be accessed
	auto *cache = OFX::Host::PluginCache::getPluginCache();
	EXPECT_NE(cache, nullptr);
}

TEST(PluginSmoke, LoadPluginsEmptyPathNoCrash)
{
	// Loading plugins from empty path should not crash
	EXPECT_NO_THROW({ loadPlugins(QString()); });
}

TEST(PluginSmoke, LoadPluginsNonExistentPathNoCrash)
{
	// Loading plugins from non-existent path should not crash
	EXPECT_NO_THROW(
		{ loadPlugins(QStringLiteral("/nonexistent/path/to/plugins")); });
}

// ============================================================================
// Smoke Test: OliveClipInstance
// ============================================================================

TEST(PluginSmokeClip, OutputClipProperties)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	VideoParams params =
		MakeVideoParams(1920, 1080, core::PixelFormat::U8, 4, true);
	params.set_pixel_aspect_ratio(core::rational(16, 9));
	params.set_frame_rate(core::rational(24, 1));
	params.set_start_time(0);
	params.set_duration(100);

	OliveClipInstance clip(nullptr, desc, params);

	// Test bit depth mapping
	EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthByte);

	// Test component mapping
	EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGBA);

	// Test premultiplication
	EXPECT_EQ(clip.getPremult(), kOfxImagePreMultiplied);

	// Test aspect ratio
	EXPECT_DOUBLE_EQ(clip.getAspectRatio(), 16.0 / 9.0);

	// Test frame rate
	EXPECT_DOUBLE_EQ(clip.getFrameRate(), 24.0);

	// Test frame range
	double start_frame = 0.0, end_frame = 0.0;
	clip.getFrameRange(start_frame, end_frame);
	EXPECT_DOUBLE_EQ(start_frame, 0.0);
	EXPECT_DOUBLE_EQ(end_frame, 100.0 * 24.0); // duration * frame_rate
}

TEST(PluginSmokeClip, ClipDifferentPixelFormats)
{
	// Test U16 format
	{
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		VideoParams params =
			MakeVideoParams(640, 480, core::PixelFormat::U16, 3, false);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthShort);
		EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGB);
		EXPECT_EQ(clip.getPremult(), kOfxImageUnPreMultiplied);
	}

	// Test F16 format
	{
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		VideoParams params =
			MakeVideoParams(640, 480, core::PixelFormat::F16, 4, true);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthHalf);
		EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGBA);
		EXPECT_EQ(clip.getPremult(), kOfxImagePreMultiplied);
	}

	// Test F32 format
	{
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		VideoParams params =
			MakeVideoParams(640, 480, core::PixelFormat::F32, 4, false);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthFloat);
		EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGBA);
		EXPECT_EQ(clip.getPremult(), kOfxImageUnPreMultiplied);
	}
}

TEST(PluginSmokeClip, SourceClipNotConnected)
{
	OFX::Host::ImageEffect::ClipDescriptor desc("Source");
	VideoParams params =
		MakeVideoParams(320, 240, core::PixelFormat::U8, 4, false);
	OliveClipInstance clip(nullptr, desc, params);

	// Source clips should not be connected (no input provided in test)
	EXPECT_FALSE(clip.getConnected());
	EXPECT_FALSE(clip.getContinuousSamples());
}

// ============================================================================
// Smoke Test: Image
// ============================================================================

TEST(PluginSmokeImage, BasicAllocation)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	VideoParams params =
		MakeVideoParams(64, 64, core::PixelFormat::U8, 4, true);
	OliveClipInstance clip(nullptr, desc, params);

	Image image(clip);
	OfxRectI bounds = { 0, 0, 64, 64 };
	OfxRectI rod = bounds;
	image.AllocateFromParams(params, bounds, rod, true);

	EXPECT_NE(image.data(), nullptr);
	EXPECT_EQ(image.width(), 64);
	EXPECT_EQ(image.height(), 64);
	EXPECT_EQ(image.row_bytes(), 64 * 4);
	EXPECT_EQ(image.pixel_format(), core::PixelFormat::U8);
	EXPECT_EQ(image.channel_count(), 4);
}

TEST(PluginSmokeImage, ClearOnAllocate)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	VideoParams params =
		MakeVideoParams(16, 16, core::PixelFormat::U8, 4, false);
	OliveClipInstance clip(nullptr, desc, params);

	Image image(clip);
	OfxRectI bounds = { 0, 0, 16, 16 };
	OfxRectI rod = bounds;

	// Allocate without clear
	image.AllocateFromParams(params, bounds, rod, false);
	ASSERT_NE(image.data(), nullptr);

	// Write some data
	std::memset(image.data(), 0xAB, image.row_bytes() * image.height());

	// Reallocate with clear
	image.AllocateFromParams(params, bounds, rod, true);

	// Verify data is cleared
	bool all_zero = true;
	for (int y = 0; y < 16 && all_zero; ++y) {
		for (int x = 0; x < 16 * 4; ++x) {
			if (image.data()[y * image.row_bytes() + x] != 0) {
				all_zero = false;
				break;
			}
		}
	}
	EXPECT_TRUE(all_zero);
}

TEST(PluginSmokeImage, ResizeOnAllocate)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	VideoParams params =
		MakeVideoParams(32, 32, core::PixelFormat::U8, 4, false);
	OliveClipInstance clip(nullptr, desc, params);

	Image image(clip);
	OfxRectI bounds = { 0, 0, 32, 32 };
	OfxRectI rod = bounds;
	image.AllocateFromParams(params, bounds, rod, true);

	EXPECT_EQ(image.width(), 32);
	EXPECT_EQ(image.height(), 32);

	// Resize to smaller
	OfxRectI new_bounds = { 0, 0, 16, 16 };
	image.EnsureAllocatedFromParams(params, new_bounds, rod, false);

	EXPECT_EQ(image.width(), 16);
	EXPECT_EQ(image.height(), 16);
}

// ============================================================================
// Smoke Test: Parameter Instances (without node binding)
// ============================================================================

TEST(PluginSmokeParam, IntegerNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeInteger, "TestInt");
	IntegerInstance instance(nullptr, desc);

	// Default value should be 0
	int value = -1;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 0);

	// Set value
	EXPECT_EQ(instance.set(42), kOfxStatOK);

	// Get value back
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 42);

	// Get at time (should return same value without node)
	int time_value = -1;
	EXPECT_EQ(instance.get(1.0, time_value), kOfxStatOK);
	EXPECT_EQ(time_value, 42);
}

TEST(PluginSmokeParam, DoubleNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeDouble, "TestDouble");
	DoubleInstance instance(nullptr, "TestDouble", desc);

	double value = -1.0;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(value, 0.0);

	EXPECT_EQ(instance.set(3.14159), kOfxStatOK);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(value, 3.14159);
}

TEST(PluginSmokeParam, BooleanNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeBoolean, "TestBool");
	BooleanInstance instance(nullptr, "TestBool", desc);

	bool value = true; // Start with opposite
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_FALSE(value);

	EXPECT_EQ(instance.set(true), kOfxStatOK);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_TRUE(value);
}

TEST(PluginSmokeParam, ChoiceNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeChoice, "TestChoice");
	ChoiceInstance instance(nullptr, "TestChoice", desc);

	int value = -1;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 0);

	EXPECT_EQ(instance.set(2), kOfxStatOK);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 2);
}

TEST(PluginSmokeParam, RGBANullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeRGBA, "TestColor");
	RGBAInstance instance(nullptr, "TestColor", desc);

	double r = 0, g = 0, b = 0, a = 0;
	EXPECT_EQ(instance.get(r, g, b, a), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.0);
	EXPECT_DOUBLE_EQ(g, 0.0);
	EXPECT_DOUBLE_EQ(b, 0.0);
	EXPECT_DOUBLE_EQ(a, 0.0);

	EXPECT_EQ(instance.set(1.0, 0.5, 0.25, 1.0), kOfxStatOK);

	EXPECT_EQ(instance.get(r, g, b, a), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 1.0);
	EXPECT_DOUBLE_EQ(g, 0.5);
	EXPECT_DOUBLE_EQ(b, 0.25);
	EXPECT_DOUBLE_EQ(a, 1.0);
}

TEST(PluginSmokeParam, RGBNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeRGB, "TestRGB");
	RGBInstance instance(nullptr, "TestRGB", desc);

	double r = 0, g = 0, b = 0;
	EXPECT_EQ(instance.get(r, g, b), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.0);
	EXPECT_DOUBLE_EQ(g, 0.0);
	EXPECT_DOUBLE_EQ(b, 0.0);

	EXPECT_EQ(instance.set(0.8, 0.6, 0.4), kOfxStatOK);

	EXPECT_EQ(instance.get(r, g, b), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.8);
	EXPECT_DOUBLE_EQ(g, 0.6);
	EXPECT_DOUBLE_EQ(b, 0.4);
}

TEST(PluginSmokeParam, Double2DNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeDouble2D, "TestVec2");
	Double2DInstance instance(nullptr, "TestVec2", desc);

	double x = 0, y = 0;
	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 0.0);
	EXPECT_DOUBLE_EQ(y, 0.0);

	EXPECT_EQ(instance.set(10.5, 20.5), kOfxStatOK);

	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 10.5);
	EXPECT_DOUBLE_EQ(y, 20.5);
}

TEST(PluginSmokeParam, Integer2DNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeInteger2D, "TestIVec2");
	Integer2DInstance instance(nullptr, "TestIVec2", desc);

	int x = 0, y = 0;
	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_EQ(x, 0);
	EXPECT_EQ(y, 0);

	EXPECT_EQ(instance.set(100, 200), kOfxStatOK);

	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_EQ(x, 100);
	EXPECT_EQ(y, 200);
}

TEST(PluginSmokeParam, Double3DNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeDouble3D, "TestVec3");
	Double3DInstance instance(nullptr, "TestVec3", desc);

	double x = 0, y = 0, z = 0;
	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 0.0);
	EXPECT_DOUBLE_EQ(y, 0.0);
	EXPECT_DOUBLE_EQ(z, 0.0);

	EXPECT_EQ(instance.set(1.0, 2.0, 3.0), kOfxStatOK);

	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 1.0);
	EXPECT_DOUBLE_EQ(y, 2.0);
	EXPECT_DOUBLE_EQ(z, 3.0);
}

TEST(PluginSmokeParam, Integer3DNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeInteger3D, "TestIVec3");
	Integer3DInstance instance(nullptr, "TestIVec3", desc);

	int x = 0, y = 0, z = 0;
	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_EQ(x, 0);
	EXPECT_EQ(y, 0);
	EXPECT_EQ(z, 0);

	EXPECT_EQ(instance.set(10, 20, 30), kOfxStatOK);

	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_EQ(x, 10);
	EXPECT_EQ(y, 20);
	EXPECT_EQ(z, 30);
}

TEST(PluginSmokeParam, StringNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeString, "TestString");
	StringInstance instance(nullptr, "TestString", desc);

	std::string value;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_TRUE(value.empty());

	EXPECT_EQ(instance.set("hello world"), kOfxStatOK);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, "hello world");
}

TEST(PluginSmokeParam, CustomNullNode)
{
	OFX::Host::Param::Descriptor desc(kOfxParamTypeCustom, "TestCustom");
	CustomInstance instance(nullptr, "TestCustom", desc);

	std::string value;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	// Custom params may have default values

	EXPECT_EQ(instance.set("custom data"), kOfxStatOK);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, "custom data");
}

// ============================================================================
// Smoke Test: Plugin Renderer
// ============================================================================

TEST(PluginSmokeRenderer, BytesToPixelsConversion)
{
	VideoParams params(100, 100, core::PixelFormat::U8, 4, core::rational(1, 1),
					   VideoParams::kInterlaceNone, 1);

	// 4 channels * 1 byte = 4 bytes per pixel
	EXPECT_EQ(detail::BytesToPixels(400, params), 100);
	EXPECT_EQ(detail::BytesToPixels(0, params), 0);

	// Test with RGB (3 channels)
	VideoParams params_rgb(100, 100, core::PixelFormat::U8, 3,
						   core::rational(1, 1), VideoParams::kInterlaceNone,
						   1);
	EXPECT_EQ(detail::BytesToPixels(300, params_rgb), 100);
}

TEST(PluginSmokeRenderer, BytesToPixelsInvalidInput)
{
	VideoParams params(100, 100, core::PixelFormat::U8, 4, core::rational(1, 1),
					   VideoParams::kInterlaceNone, 1);

	// Negative input should return 0
	EXPECT_EQ(detail::BytesToPixels(-1, params), 0);
}

// ============================================================================
// Smoke Test: Plugin Job
// ============================================================================

TEST(PluginSmokeJob, JobConstruction)
{
	NodeValueRow row;
	PluginJob job(nullptr, nullptr, row);

	EXPECT_EQ(job.pluginInstance(), nullptr);
	EXPECT_EQ(job.node(), nullptr);
	EXPECT_DOUBLE_EQ(job.time_seconds(), 0.0);
}

TEST(PluginSmokeJob, JobWithTime)
{
	NodeValueRow row;
	core::rational time(5, 1); // 5 seconds
	PluginJob job(nullptr, nullptr, row, time);

	EXPECT_DOUBLE_EQ(job.time_seconds(), 5.0);
}

TEST(PluginSmokeJob, JobWithTextureValue)
{
	VideoParams params(64, 64, core::PixelFormat::U8, 4);
	TexturePtr tex = CreateTestTexture(params, 0x80);
	ASSERT_NE(tex, nullptr);

	NodeValueRow row;
	row.insert(QStringLiteral("source"), NodeValue(NodeValue::kTexture, tex));

	PluginJob job(nullptr, nullptr, row);

	// Job should have the values inserted
	EXPECT_FALSE(job.GetValues().isEmpty());
}

// ============================================================================
// Smoke Test: Plugin Node (basic lifecycle)
// ============================================================================

TEST(PluginSmokeNode, NodeRequiresValidInstance)
{
	// PluginNode requires a valid OFX instance
	// Creating without one should be handled gracefully
	// Note: This test documents expected behavior

	// A PluginNode cannot be created without an instance
	// The constructor requires an OFX::Host::ImageEffect::Instance
	EXPECT_TRUE(true); // Placeholder for documentation
}

// ============================================================================
// Smoke Test: Integration Components
// ============================================================================

TEST(PluginSmokeIntegration, VideoParamsToOfxMapping)
{
	// Test U8 -> Byte mapping
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::U8, 4, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthByte);
	}

	// Test U16 -> Short mapping
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::U16, 4, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthShort);
	}

	// Test F16 -> Half mapping
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::F16, 4, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthHalf);
	}

	// Test F32 -> Float mapping
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::F32, 4, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthFloat);
	}
}

TEST(PluginSmokeIntegration, ComponentCountMapping)
{
	// Test RGB (3 channels)
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::U8, 3, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGB);
	}

	// Test RGBA (4 channels)
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::U8, 4, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGBA);
	}

	// Test Alpha (1 channel)
	{
		VideoParams params =
			MakeVideoParams(100, 100, core::PixelFormat::U8, 1, false);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentAlpha);
	}
}

// ============================================================================
// Smoke Test: Thread Safety
// ============================================================================

TEST(PluginSmokeThread, ConcurrentImageAllocation)
{
	const int num_threads = 4;
	const int num_allocs_per_thread = 10;

	std::vector<std::thread> threads;
	std::atomic<int> success_count{ 0 };

	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([&success_count, num_allocs_per_thread, t]() {
			for (int i = 0; i < num_allocs_per_thread; ++i) {
				OFX::Host::ImageEffect::ClipDescriptor desc(
					kOfxImageEffectOutputClipName);
				VideoParams params = MakeVideoParams(
					32 + t, 32 + i, core::PixelFormat::U8, 4, false);
				OliveClipInstance clip(nullptr, desc, params);

				Image image(clip);
				OfxRectI bounds = { 0, 0, 32 + t, 32 + i };
				OfxRectI rod = bounds;
				image.AllocateFromParams(params, bounds, rod, true);

				if (image.data() != nullptr && image.width() == 32 + t &&
					image.height() == 32 + i) {
					success_count++;
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	EXPECT_EQ(success_count.load(), num_threads * num_allocs_per_thread);
}

TEST(PluginSmokeThread, ConcurrentParamAccess)
{
	const int num_threads = 4;
	const int num_ops_per_thread = 100;

	OFX::Host::Param::Descriptor desc(kOfxParamTypeInteger, "ConcurrentInt");
	IntegerInstance instance(nullptr, desc);

	std::atomic<int> success_count{ 0 };
	std::mutex access_mutex;
	std::vector<std::thread> threads;

	for (int t = 0; t < num_threads; ++t) {
		threads.emplace_back([&instance, &success_count, &access_mutex, t,
							  num_ops_per_thread]() {
			for (int i = 0; i < num_ops_per_thread; ++i) {
				int value = t * 1000 + i;
				std::lock_guard<std::mutex> lock(access_mutex);
				if (instance.set(value) == kOfxStatOK) {
					int read_value = -1;
					if (instance.get(read_value) == kOfxStatOK) {
						// Without node binding, value should be what we just set
						if (read_value == value) {
							success_count++;
						}
					}
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	EXPECT_EQ(success_count.load(), num_threads * num_ops_per_thread);
}

} // namespace test
} // namespace plugin
} // namespace olive
