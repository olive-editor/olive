/*
 * Oak Video Editor - OFX Plugin Integration Tests
 * Copyright (C) 2025 Olive CE Team
 *
 * Tests for various OpenFX plugins including:
 * - MirrorOFX (net.sf.openfx.Mirror)
 * - Transform (net.sf.openfx.TransformPlugin)
 * - ColorCorrect (net.sf.openfx.ColorCorrectPlugin)
 * - Saturation (net.sf.openfx.SaturationPlugin)
 * - Blur (net.sf.openfx.GaussianBlurPlugin)
 */

#include <gtest/gtest.h>

#include <cstdlib>

#include "common/ffmpegutils.h"
#include "node/value.h"
#include "pluginSupport/OliveHost.h"
#include "pluginSupport/OlivePluginInstance.h"
#include "render/job/pluginjob.h"
#include "render/plugin/pluginrenderer.h"
#include "render/texture.h"
#include "render/videoparams.h"

namespace olive
{
namespace plugin
{
namespace test
{

namespace
{

// Helper to create a test texture with solid color
// For U8: fill_value is 0-255
// For U16: fill_value is 0-65535
// For Float: fill_value is 0.0-1.0 mapped to bytes
template <typename T>
TexturePtr CreateSolidTextureT(const VideoParams &params, T fill_value)
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
		T *row = reinterpret_cast<T *>(frame->data(0) + y * linesize);
		for (int x = 0; x < frame->width() * params.channel_count(); ++x) {
			row[x] = fill_value;
		}
	}

	TexturePtr texture = std::make_shared<Texture>(params);
	texture->handleFrame(frame);
	return texture;
}

TexturePtr CreateSolidTexture(const VideoParams &params,
							  uint32_t fill_value = 0x7f)
{
	// Choose type based on pixel format
	switch (params.format()) {
	case core::PixelFormat::U8:
		return CreateSolidTextureT<uint8_t>(params,
											static_cast<uint8_t>(fill_value));
	case core::PixelFormat::U16:
		return CreateSolidTextureT<uint16_t>(params,
											 static_cast<uint16_t>(fill_value));
	case core::PixelFormat::F16:
	case core::PixelFormat::F32:
		return CreateSolidTextureT<float>(
			params, static_cast<float>(fill_value) / 255.0f);
	default:
		return nullptr;
	}
}

// Helper to create a gradient texture
// For U8: gradient is 0-255 per byte
// For Float: gradient is 0.0-1.0 per component
template <typename T>
TexturePtr CreateGradientTextureT(const VideoParams &params, float scale)
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
		T *row = reinterpret_cast<T *>(frame->data(0) + y * linesize);
		T value = static_cast<T>((y * scale) / frame->height());
		for (int x = 0; x < frame->width() * params.channel_count(); ++x) {
			row[x] = value;
		}
	}

	TexturePtr texture = std::make_shared<Texture>(params);
	texture->handleFrame(frame);
	return texture;
}

TexturePtr CreateGradientTexture(const VideoParams &params)
{
	switch (params.format()) {
	case core::PixelFormat::U8:
		return CreateGradientTextureT<uint8_t>(params, 255.0f);
	case core::PixelFormat::U16:
		return CreateGradientTextureT<uint16_t>(params, 65535.0f);
	case core::PixelFormat::F16:
	case core::PixelFormat::F32:
		return CreateGradientTextureT<float>(params, 1.0f);
	default:
		return nullptr;
	}
}

// Helper function to find and render a plugin
bool RenderPlugin(const std::string &plugin_id, const VideoParams &params,
				  const NodeValueRow &inputs, bool verbose = false)
{
	auto *cache = OFX::Host::PluginCache::getPluginCache();
	if (!cache) {
		if (verbose)
			std::cerr << "Plugin cache not available" << std::endl;
		return false;
	}

	OFX::Host::Plugin *found = nullptr;
	for (auto *plug : cache->getPlugins()) {
		if (plug && plug->getIdentifier() == plugin_id) {
			found = plug;
			break;
		}
	}

	if (!found) {
		if (verbose)
			std::cerr << "Plugin not found: " << plugin_id << std::endl;
		return false;
	}

	auto *image_effect =
		dynamic_cast<OFX::Host::ImageEffect::ImageEffectPlugin *>(found);
	if (!image_effect) {
		if (verbose)
			std::cerr << "Not an image effect plugin" << std::endl;
		return false;
	}

	const auto &contexts = image_effect->getContexts();
	std::string context = kOfxImageEffectContextFilter;
	if (!contexts.empty() &&
		contexts.find(kOfxImageEffectContextFilter) == contexts.end()) {
		context = *contexts.begin();
	}

	OFX::Host::ImageEffect::Instance *instance =
		image_effect->createInstance(context, nullptr);
	if (!instance) {
		if (verbose)
			std::cerr << "Failed to create instance" << std::endl;
		return false;
	}

	auto *olive_instance = dynamic_cast<OlivePluginInstance *>(instance);
	if (!olive_instance) {
		if (verbose)
			std::cerr << "Not an OlivePluginInstance" << std::endl;
		return false;
	}

	olive_instance->setVideoParam(params);

	PluginJob job(instance, nullptr, inputs);
	TexturePtr output = std::make_shared<Texture>(params);

	PluginRenderer renderer(nullptr);
	renderer.RenderPlugin(nullptr, job, output, params, true, false);

	bool has_frame = output->frame() != nullptr;
	if (!has_frame && verbose) {
		std::cerr << "Render produced no output frame" << std::endl;
	}

	return has_frame;
}

// Skip check function
bool ShouldSkipTest()
{
	const char *itest = std::getenv("OAK_OFX_ITEST");
	if (!itest || std::string(itest) != "1") {
		return true;
	}

	const char *path = std::getenv("OAK_OFX_PLUGIN_PATH");
	if (!path || std::string(path).empty()) {
		return true;
	}

	static bool plugins_loaded = false;
	if (!plugins_loaded) {
		QString raw = QString::fromUtf8(path);
		const QChar separator = QDir::listSeparator();
		const QStringList paths = raw.split(separator, Qt::SkipEmptyParts);
		for (const QString &p : paths) {
			loadPlugins(p);
		}
		plugins_loaded = true;
	}

	return false;
}

} // anonymous namespace

// ============================================================================
// Mirror Plugin Tests
// ============================================================================

TEST(PluginMisc, MirrorHorizontal)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	// Mirror plugin typically works with 8-bit
	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateGradientTexture(params);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.Mirror", params, row, true);
	EXPECT_TRUE(result) << "Mirror plugin should produce output";
}

// ============================================================================
// Transform Plugin Tests
// ============================================================================

TEST(PluginMisc, TransformTranslate)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result =
		RenderPlugin("net.sf.openfx.TransformPlugin", params, row, true);
	EXPECT_TRUE(result) << "Transform plugin should produce output";
}

// ============================================================================
// Color Correction Plugin Tests
// ============================================================================

TEST(PluginMisc, ColorCorrect)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result =
		RenderPlugin("net.sf.openfx.ColorCorrectPlugin", params, row, true);
	EXPECT_TRUE(result) << "ColorCorrect plugin should produce output";
}

TEST(PluginMisc, Saturation)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result =
		RenderPlugin("net.sf.openfx.SaturationPlugin", params, row, true);
	EXPECT_TRUE(result) << "Saturation plugin should produce output";
}

// ============================================================================
// Blur Plugin Tests
// ============================================================================

TEST(PluginMisc, GaussianBlur)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	// Use CImgBlur from the available plugin list
	bool result = RenderPlugin("net.sf.cimg.CImgBlur", params, row, true);
	EXPECT_TRUE(result) << "GaussianBlur plugin should produce output";
}

// ============================================================================
// Additional Plugin Tests
// ============================================================================

TEST(PluginMisc, Crop)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.CropPlugin", params, row, true);
	EXPECT_TRUE(result) << "Crop plugin should produce output";
}

TEST(PluginMisc, Grade)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.GradePlugin", params, row, true);
	EXPECT_TRUE(result) << "Grade plugin should produce output";
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST(PluginMisc, NonExistentPlugin)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result =
		RenderPlugin("net.sf.openfx.NonExistentPlugin", params, row, true);
	EXPECT_FALSE(result) << "Non-existent plugin should fail gracefully";
}

// ============================================================================
// Additional CImg-based Plugin Tests
// ============================================================================

TEST(PluginMisc, CImgSharpen)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.cimg.CImgSharpen", params, row, true);
	EXPECT_TRUE(result) << "CImgSharpen plugin should produce output";
}

TEST(PluginMisc, CImgDenoise)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.cimg.CImgDenoise", params, row, true);
	EXPECT_TRUE(result) << "CImgDenoise plugin should produce output";
}

TEST(PluginMisc, CImgBilateral)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.cimg.CImgBilateral", params, row, true);
	EXPECT_TRUE(result) << "CImgBilateral plugin should produce output";
}

// ============================================================================
// Merge Plugin Tests (for transitions/compositing)
// ============================================================================

TEST(PluginMisc, MergeOver)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));
	row.insert(QStringLiteral("Bg"), NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.MergePlugin", params, row, true);
	EXPECT_TRUE(result) << "Merge plugin should produce output";
}

// ============================================================================
// Keyer Plugin Test
// ============================================================================

TEST(PluginMisc, Keyer)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.KeyerPlugin", params, row, true);
	EXPECT_TRUE(result) << "Keyer plugin should produce output";
}

// ============================================================================
// Transform Tests
// ============================================================================

TEST(PluginMisc, CornerPin)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result =
		RenderPlugin("net.sf.openfx.CornerPinPlugin", params, row, true);
	EXPECT_TRUE(result) << "CornerPin plugin should produce output";
}

TEST(PluginMisc, LensDistortion)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result =
		RenderPlugin("net.sf.openfx.LensDistortion", params, row, true);
	EXPECT_TRUE(result) << "LensDistortion plugin should produce output";
}

// ============================================================================
// Color Space Conversion Tests
// ============================================================================

TEST(PluginMisc, Invert)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.Invert", params, row, true);
	EXPECT_TRUE(result) << "Invert plugin should produce output";
}

TEST(PluginMisc, Gamma)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr input = CreateSolidTexture(params, 0x80);
	ASSERT_NE(input, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, input));

	bool result = RenderPlugin("net.sf.openfx.GammaPlugin", params, row, true);
	EXPECT_TRUE(result) << "Gamma plugin should produce output";
}

// ============================================================================
// Utility Test
// ============================================================================

TEST(PluginMisc, ListAvailablePlugins)
{
	if (ShouldSkipTest()) {
		GTEST_SKIP() << "OFX integration test not enabled";
	}

	auto *cache = OFX::Host::PluginCache::getPluginCache();
	if (!cache) {
		GTEST_SKIP() << "Plugin cache not available";
	}

	std::cout << "\nAvailable OFX plugins:\n";
	for (auto *plug : cache->getPlugins()) {
		if (plug) {
			std::cout << "  - " << plug->getIdentifier() << std::endl;
		}
	}
	std::cout << std::endl;

	SUCCEED();
}

TEST(PluginMisc, CImgBilateralGuided_MultiInput)
{
	if (ShouldSkipTest())
		GTEST_SKIP() << "OFX integration test not enabled";
	// CImgBilateralGuided is a multi-input plugin (Source + Guide).
	// This test verifies that connecting both inputs does not trigger
	// the frame-rate mismatch exception in setupClipPreferencesArgs.
	VideoParams params(320, 240, core::PixelFormat::F32, 4);
	TexturePtr source = CreateSolidTexture(params, 0x80);
	TexturePtr guide = CreateSolidTexture(params, 0x40);
	ASSERT_NE(source, nullptr);
	ASSERT_NE(guide, nullptr);

	NodeValueRow row;
	row.insert(QString::fromStdString(kOfxImageEffectSimpleSourceClipName),
			   NodeValue(NodeValue::kTexture, source));
	row.insert(QStringLiteral("Guide"), NodeValue(NodeValue::kTexture, guide));
	bool result =
		RenderPlugin("net.sf.cimg.CImgBilateralGuided", params, row, true);
	EXPECT_TRUE(result)
		<< "CImgBilateralGuided plugin should produce output with both Source and Guide connected";
}

} // namespace test
} // namespace plugin
} // namespace olive
