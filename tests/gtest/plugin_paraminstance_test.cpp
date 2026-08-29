#include <gtest/gtest.h>

#include <limits>

#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "common/avframeptr.h"
#include "common/ffmpegutils.h"
#include "pluginSupport/OliveClip.h"
#include "pluginSupport/image.h"
#include "pluginSupport/paraminstance.h"
#include "render/texture.h"

namespace
{
olive::VideoParams MakeParams(int width, int height,
							  olive::core::PixelFormat format, int channels,
							  bool premultiplied)
{
	olive::VideoParams params;
	params.set_width(width);
	params.set_height(height);
	params.set_format(format);
	params.set_channel_count(channels);
	params.set_premultiplied_alpha(premultiplied);
	return params;
}

olive::AVFramePtr CreateFrame(const olive::VideoParams &params)
{
	olive::AVFramePtr frame = olive::CreateAVFramePtr();
	frame->set_format(olive::FFmpegUtils::GetFFmpegPixelFormat(
		params.format(), params.channel_count()));
	frame->set_width(params.width());
	frame->set_height(params.height());
	if (frame->get_buffer(0) < 0) {
		return nullptr;
	}
	return frame;
}
}

// ============================================================================
// app/pluginSupport/paraminstance.h
//
// The node-bound code paths require a PluginNode, which dereferences a real
// OFX ImageEffect::Instance in its constructor and therefore cannot be built
// without a plugin bundle. These tests cover the host-side fallback surface:
// descriptor defaults and the cached-value behavior when no node is bound.
// ============================================================================

TEST(PluginParamInstance, CoordinateSystemHelpers)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeDouble,
											"TestCoordinate");
	// A bare descriptor has no coordinate-system property; the OFX property
	// suite reports an empty string which is treated as canonical.
	EXPECT_FALSE(olive::plugin::IsNormalisedCoordinateSystem(descriptor));

	descriptor.addStandardParamProps(kOfxParamTypeDouble);
	EXPECT_FALSE(olive::plugin::IsNormalisedCoordinateSystem(descriptor));

	descriptor.getProperties().setStringProperty(
		kOfxParamPropDefaultCoordinateSystem, kOfxParamCoordinatesNormalised);
	EXPECT_TRUE(olive::plugin::IsNormalisedCoordinateSystem(descriptor));

	EXPECT_DOUBLE_EQ(olive::plugin::ToNormalised(960.0, 1920.0), 0.5);
	EXPECT_DOUBLE_EQ(olive::plugin::ToCanonical(0.5, 1920.0), 960.0);
	// A non-positive extent passes the value through unchanged.
	EXPECT_DOUBLE_EQ(olive::plugin::ToNormalised(7.5, 0.0), 7.5);
	EXPECT_DOUBLE_EQ(olive::plugin::ToCanonical(7.5, 0.0), 7.5);
}

TEST(PluginParamInstance, ParamChangeLabelContainsParamName)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeDouble, "Gain");
	EXPECT_EQ(olive::plugin::ParamChangeLabel(descriptor),
			  QStringLiteral("Change Gain"));
}

TEST(PluginParamInstance, SubmitUndoCommandIgnoresNullCommand)
{
	EXPECT_NO_THROW(olive::plugin::SubmitUndoCommand(
		nullptr, nullptr, QStringLiteral("Ignored")));
}

TEST(PluginParamInstance, IntegerInstanceUsesDescriptorDefault)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeInteger,
											"TestIntegerDefault");
	descriptor.addStandardParamProps(kOfxParamTypeInteger);
	descriptor.getProperties().setIntProperty(kOfxParamPropDefault, 42);

	olive::plugin::IntegerInstance instance(nullptr, descriptor);

	int value = -1;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 42);

	int time_value = -1;
	EXPECT_EQ(instance.get(1.5, time_value), kOfxStatOK);
	EXPECT_EQ(time_value, 42);

	// Rebinding to the same (null) node keeps the cached value.
	instance.SetNode(nullptr);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 42);
}

TEST(PluginParamInstance, DoubleInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor bare(kOfxParamTypeDouble, "TestDoubleBare");
	olive::plugin::DoubleInstance bare_instance(nullptr, "TestDoubleBare",
												bare);
	double value = -1.0;
	EXPECT_EQ(bare_instance.get(value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(value, 0.0);

	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeDouble, "TestDouble");
	descriptor.addStandardParamProps(kOfxParamTypeDouble);
	descriptor.getProperties().setDoubleProperty(kOfxParamPropDefault, 3.5);
	olive::plugin::DoubleInstance instance(nullptr, "TestDouble", descriptor);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(value, 3.5);

	EXPECT_EQ(instance.set(7.25), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(value, 7.25);

	double time_value = 0.0;
	EXPECT_EQ(instance.get(1.5, time_value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(time_value, 7.25);

	EXPECT_EQ(instance.set(0.5, -2.5), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_DOUBLE_EQ(value, -2.5);

	double derived = 1.0;
	EXPECT_EQ(instance.derive(0.0, derived), kOfxStatErrUnsupported);
	double integrated = 1.0;
	EXPECT_EQ(instance.integrate(0.0, 1.0, integrated),
			  kOfxStatErrUnsupported);
}

TEST(PluginParamInstance, BooleanInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor bare(kOfxParamTypeBoolean, "TestBooleanBare");
	olive::plugin::BooleanInstance bare_instance(nullptr, "TestBooleanBare",
												 bare);
	bool value = true;
	EXPECT_EQ(bare_instance.get(value), kOfxStatOK);
	EXPECT_FALSE(value);

	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeBoolean,
											"TestBoolean");
	descriptor.addStandardParamProps(kOfxParamTypeBoolean);
	descriptor.getProperties().setIntProperty(kOfxParamPropDefault, 1);
	olive::plugin::BooleanInstance instance(nullptr, "TestBoolean", descriptor);

	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_TRUE(value);

	EXPECT_EQ(instance.set(false), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_FALSE(value);

	EXPECT_EQ(instance.set(1.0, true), kOfxStatOK);
	bool time_value = false;
	EXPECT_EQ(instance.get(1.0, time_value), kOfxStatOK);
	EXPECT_TRUE(time_value);
}

TEST(PluginParamInstance, ChoiceInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeChoice, "TestChoice");
	descriptor.addStandardParamProps(kOfxParamTypeChoice);
	descriptor.getProperties().setIntProperty(kOfxParamPropDefault, 2);
	olive::plugin::ChoiceInstance instance(nullptr, "TestChoice", descriptor);

	int value = -1;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 2);

	EXPECT_EQ(instance.set(4), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, 4);

	int time_value = -1;
	EXPECT_EQ(instance.get(2.0, time_value), kOfxStatOK);
	EXPECT_EQ(time_value, 4);
}

TEST(PluginParamInstance, StringInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeString, "TestString");
	descriptor.addStandardParamProps(kOfxParamTypeString);
	descriptor.getProperties().setStringProperty(kOfxParamPropDefault,
												 "default_text");
	olive::plugin::StringInstance instance(nullptr, "TestString", descriptor);

	std::string value;
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, "default_text");

	EXPECT_EQ(instance.set("hello world"), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, "hello world");

	std::string time_value;
	EXPECT_EQ(instance.get(3.0, time_value), kOfxStatOK);
	EXPECT_EQ(time_value, "hello world");

	// A null C-string is stored as an empty string.
	EXPECT_EQ(instance.set(nullptr), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_TRUE(value.empty());
}

TEST(PluginParamInstance, CustomInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeCustom, "TestCustom");
	olive::plugin::CustomInstance instance(nullptr, "TestCustom", descriptor);

	std::string value = "sentinel";
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_TRUE(value.empty());

	EXPECT_EQ(instance.set("binary_blob"), kOfxStatOK);
	EXPECT_EQ(instance.get(value), kOfxStatOK);
	EXPECT_EQ(value, "binary_blob");

	std::string time_value;
	EXPECT_EQ(instance.get(0.25, time_value), kOfxStatOK);
	EXPECT_EQ(time_value, "binary_blob");
}

TEST(PluginParamInstance, RGBAInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeRGBA, "TestRGBA");
	olive::plugin::RGBAInstance instance(nullptr, "TestRGBA", descriptor);

	double r = -1.0, g = -1.0, b = -1.0, a = -1.0;
	EXPECT_EQ(instance.get(r, g, b, a), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.0);
	EXPECT_DOUBLE_EQ(g, 0.0);
	EXPECT_DOUBLE_EQ(b, 0.0);
	EXPECT_DOUBLE_EQ(a, 0.0);

	EXPECT_EQ(instance.set(0.1, 0.2, 0.3, 0.4), kOfxStatOK);
	EXPECT_EQ(instance.get(r, g, b, a), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.1);
	EXPECT_DOUBLE_EQ(g, 0.2);
	EXPECT_DOUBLE_EQ(b, 0.3);
	EXPECT_DOUBLE_EQ(a, 0.4);

	EXPECT_EQ(instance.set(2.0, 0.5, 0.6, 0.7, 0.8), kOfxStatOK);
	EXPECT_EQ(instance.get(2.0, r, g, b, a), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.5);
	EXPECT_DOUBLE_EQ(g, 0.6);
	EXPECT_DOUBLE_EQ(b, 0.7);
	EXPECT_DOUBLE_EQ(a, 0.8);
}

TEST(PluginParamInstance, RGBInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeRGB, "TestRGB");
	olive::plugin::RGBInstance instance(nullptr, "TestRGB", descriptor);

	double r = -1.0, g = -1.0, b = -1.0;
	EXPECT_EQ(instance.get(r, g, b), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.0);
	EXPECT_DOUBLE_EQ(g, 0.0);
	EXPECT_DOUBLE_EQ(b, 0.0);

	EXPECT_EQ(instance.set(1.0, 0.5, 0.25), kOfxStatOK);
	EXPECT_EQ(instance.get(r, g, b), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 1.0);
	EXPECT_DOUBLE_EQ(g, 0.5);
	EXPECT_DOUBLE_EQ(b, 0.25);

	EXPECT_EQ(instance.set(1.5, 0.75, 0.5, 0.125), kOfxStatOK);
	EXPECT_EQ(instance.get(1.5, r, g, b), kOfxStatOK);
	EXPECT_DOUBLE_EQ(r, 0.75);
	EXPECT_DOUBLE_EQ(g, 0.5);
	EXPECT_DOUBLE_EQ(b, 0.125);
}

TEST(PluginParamInstance, Double2DInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeDouble2D,
											"TestDouble2D");
	olive::plugin::Double2DInstance instance(nullptr, "TestDouble2D",
											 descriptor);

	double x = -1.0, y = -1.0;
	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 0.0);
	EXPECT_DOUBLE_EQ(y, 0.0);

	EXPECT_EQ(instance.set(1.5, -2.5), kOfxStatOK);
	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 1.5);
	EXPECT_DOUBLE_EQ(y, -2.5);

	EXPECT_EQ(instance.set(0.5, 3.0, 4.0), kOfxStatOK);
	EXPECT_EQ(instance.get(0.5, x, y), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 3.0);
	EXPECT_DOUBLE_EQ(y, 4.0);
}

TEST(PluginParamInstance, Integer2DInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeInteger2D,
											"TestInteger2D");
	olive::plugin::Integer2DInstance instance(nullptr, "TestInteger2D",
											  descriptor);

	int x = -1, y = -1;
	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_EQ(x, 0);
	EXPECT_EQ(y, 0);

	EXPECT_EQ(instance.set(3, -7), kOfxStatOK);
	EXPECT_EQ(instance.get(x, y), kOfxStatOK);
	EXPECT_EQ(x, 3);
	EXPECT_EQ(y, -7);

	EXPECT_EQ(instance.set(2.0, 10, 20), kOfxStatOK);
	EXPECT_EQ(instance.get(2.0, x, y), kOfxStatOK);
	EXPECT_EQ(x, 10);
	EXPECT_EQ(y, 20);
}

TEST(PluginParamInstance, Double3DInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeDouble3D,
											"TestDouble3D");
	olive::plugin::Double3DInstance instance(nullptr, "TestDouble3D",
											 descriptor);

	double x = -1.0, y = -1.0, z = -1.0;
	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 0.0);
	EXPECT_DOUBLE_EQ(y, 0.0);
	EXPECT_DOUBLE_EQ(z, 0.0);

	EXPECT_EQ(instance.set(1.0, 2.0, 3.0), kOfxStatOK);
	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, 1.0);
	EXPECT_DOUBLE_EQ(y, 2.0);
	EXPECT_DOUBLE_EQ(z, 3.0);

	EXPECT_EQ(instance.set(4.0, -1.0, -2.0, -3.0), kOfxStatOK);
	EXPECT_EQ(instance.get(4.0, x, y, z), kOfxStatOK);
	EXPECT_DOUBLE_EQ(x, -1.0);
	EXPECT_DOUBLE_EQ(y, -2.0);
	EXPECT_DOUBLE_EQ(z, -3.0);
}

TEST(PluginParamInstance, Integer3DInstanceNullNodeRoundTrip)
{
	OFX::Host::Param::Descriptor descriptor(kOfxParamTypeInteger3D,
											"TestInteger3D");
	olive::plugin::Integer3DInstance instance(nullptr, "TestInteger3D",
											  descriptor);

	int x = -1, y = -1, z = -1;
	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_EQ(x, 0);
	EXPECT_EQ(y, 0);
	EXPECT_EQ(z, 0);

	EXPECT_EQ(instance.set(-1, 0, 5), kOfxStatOK);
	EXPECT_EQ(instance.get(x, y, z), kOfxStatOK);
	EXPECT_EQ(x, -1);
	EXPECT_EQ(y, 0);
	EXPECT_EQ(z, 5);

	EXPECT_EQ(instance.set(3.0, 7, 8, 9), kOfxStatOK);
	EXPECT_EQ(instance.get(3.0, x, y, z), kOfxStatOK);
	EXPECT_EQ(x, 7);
	EXPECT_EQ(y, 8);
	EXPECT_EQ(z, 9);
}

TEST(PluginParamInstance, PushbuttonGroupAndPageInstancesExposeNames)
{
	OFX::Host::Param::Descriptor button_desc(kOfxParamTypePushButton,
											 "TestButton");
	olive::plugin::PushbuttonInstance button(nullptr, "TestButton",
											 button_desc);
	button.SetNode(nullptr);
	EXPECT_EQ(button.getName(), "TestButton");

	OFX::Host::Param::Descriptor group_desc(kOfxParamTypeGroup, "TestGroup");
	olive::plugin::GroupInstance group(group_desc);
	EXPECT_EQ(group.getName(), "TestGroup");

	OFX::Host::Param::Descriptor page_desc(kOfxParamTypePage, "TestPage");
	olive::plugin::PageInstance page(page_desc);
	EXPECT_EQ(page.getName(), "TestPage");
}

// ============================================================================
// app/pluginSupport/OliveClip.cpp
// ============================================================================

TEST(PluginClipInstance, UnmappedBitDepthFallsBackToParams)
{
	// U16 -> kOfxBitDepthShort is covered by PluginSupportClip.PropertyGetters.
	struct Case {
		olive::core::PixelFormat format;
		const char *expected;
	};
	const Case cases[] = {
		{ olive::core::PixelFormat::U8, kOfxBitDepthByte },
		{ olive::core::PixelFormat::U10, kOfxBitDepthNone },
		{ olive::core::PixelFormat::F16, kOfxBitDepthHalf },
		{ olive::core::PixelFormat::F32, kOfxBitDepthFloat },
		{ olive::core::PixelFormat::INVALID, kOfxBitDepthNone },
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		SCOPED_TRACE(i);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		olive::VideoParams params =
			MakeParams(16, 16, cases[i].format, 4, false);
		olive::plugin::OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedBitDepth(), cases[i].expected);
	}
}

TEST(PluginClipInstance, UnmappedBitDepthPrefersPluginChoice)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	clip.setPixelDepth(kOfxBitDepthFloat);
	EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthFloat);

	// kOfxBitDepthNone means "no preference" and falls back to the params.
	clip.setPixelDepth(kOfxBitDepthNone);
	EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthByte);
}

TEST(PluginClipInstance, UnmappedComponentsFallsBackToParams)
{
	// 3 channels -> kOfxImageComponentRGB is covered by
	// PluginSupportClip.PropertyGetters.
	struct Case {
		int channels;
		const char *expected;
	};
	const Case cases[] = {
		{ 1, kOfxImageComponentAlpha },
		{ 4, kOfxImageComponentRGBA },
		{ 2, kOfxImageComponentNone },
		{ 0, kOfxImageComponentNone },
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		SCOPED_TRACE(i);
		OFX::Host::ImageEffect::ClipDescriptor desc(
			kOfxImageEffectOutputClipName);
		olive::VideoParams params = MakeParams(
			16, 16, olive::core::PixelFormat::U8, cases[i].channels, false);
		olive::plugin::OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getUnmappedComponents(), cases[i].expected);
	}
}

TEST(PluginClipInstance, UnmappedComponentsPrefersPluginChoice)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	clip.setComponents(kOfxImageComponentAlpha);
	EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentAlpha);

	clip.setComponents(kOfxImageComponentNone);
	EXPECT_EQ(clip.getUnmappedComponents(), kOfxImageComponentRGBA);
}

TEST(PluginClipInstance, PremultReflectsPremultipliedParams)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, true);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	EXPECT_EQ(clip.getPremult(), kOfxImagePreMultiplied);
}

TEST(PluginClipInstance, AspectRatioDefaultsToOneForZeroPar)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	params.set_pixel_aspect_ratio(olive::core::rational(0, 1));
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	EXPECT_DOUBLE_EQ(clip.getAspectRatio(), 1.0);
}

TEST(PluginClipInstance, FieldOrderNoneAndLower)
{
	// kInterlacedTopFirst -> kOfxImageFieldUpper is covered by
	// PluginSupportClip.PropertyGetters.
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams progressive =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	progressive.set_interlacing(olive::VideoParams::kInterlaceNone);
	olive::plugin::OliveClipInstance progressive_clip(nullptr, desc,
													  progressive);
	EXPECT_EQ(progressive_clip.getFieldOrder(), kOfxImageFieldNone);

	olive::VideoParams lower =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	lower.set_interlacing(olive::VideoParams::kInterlacedBottomFirst);
	olive::plugin::OliveClipInstance lower_clip(nullptr, desc, lower);
	EXPECT_EQ(lower_clip.getFieldOrder(), kOfxImageFieldLower);
}

TEST(PluginClipInstance, RegionOfDefinitionDefaultsToScaledFrame)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(100, 80, olive::core::PixelFormat::U8, 4, false);
	params.set_pixel_aspect_ratio(olive::core::rational(2, 1));
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	OfxRectD rod = clip.getRegionOfDefinition(0.0);
	EXPECT_DOUBLE_EQ(rod.x1, 0.0);
	EXPECT_DOUBLE_EQ(rod.y1, 0.0);
	EXPECT_DOUBLE_EQ(rod.x2, 200.0);
	EXPECT_DOUBLE_EQ(rod.y2, 80.0);
}

TEST(PluginClipInstance, RegionOfDefinitionPerTimeOverride)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(100, 80, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	OfxRectD custom = { 1.5, 2.5, 51.5, 21.0 };
	clip.setRegionOfDefinition(custom, 3.0);

	OfxRectD at_three = clip.getRegionOfDefinition(3.0);
	EXPECT_DOUBLE_EQ(at_three.x1, 1.5);
	EXPECT_DOUBLE_EQ(at_three.y1, 2.5);
	EXPECT_DOUBLE_EQ(at_three.x2, 51.5);
	EXPECT_DOUBLE_EQ(at_three.y2, 21.0);

	// Other times still fall back to the params-derived region.
	OfxRectD at_four = clip.getRegionOfDefinition(4.0);
	EXPECT_DOUBLE_EQ(at_four.x1, 0.0);
	EXPECT_DOUBLE_EQ(at_four.y1, 0.0);
	EXPECT_DOUBLE_EQ(at_four.x2, 100.0);
	EXPECT_DOUBLE_EQ(at_four.y2, 80.0);
}

TEST(PluginClipInstance, RegionOfDefinitionFallsBackToStoredDefault)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);

	// Zero-sized params yield no usable params-derived region, so the stored
	// default is used.
	olive::VideoParams empty_params =
		MakeParams(0, 0, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance empty_clip(nullptr, desc, empty_params);

	OfxRectD stored = { 2.0, 4.0, 202.0, 104.0 };
	empty_clip.setDefaultRegionOfDefinition(stored);

	OfxRectD fallback = empty_clip.getRegionOfDefinition(0.0);
	EXPECT_DOUBLE_EQ(fallback.x1, 2.0);
	EXPECT_DOUBLE_EQ(fallback.y1, 4.0);
	EXPECT_DOUBLE_EQ(fallback.x2, 202.0);
	EXPECT_DOUBLE_EQ(fallback.y2, 104.0);

	// A params-derived region still takes precedence over the stored default.
	olive::VideoParams params =
		MakeParams(100, 80, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);
	clip.setDefaultRegionOfDefinition(stored);

	OfxRectD derived = clip.getRegionOfDefinition(0.0);
	EXPECT_DOUBLE_EQ(derived.x1, 0.0);
	EXPECT_DOUBLE_EQ(derived.y1, 0.0);
	EXPECT_DOUBLE_EQ(derived.x2, 100.0);
	EXPECT_DOUBLE_EQ(derived.y2, 80.0);
}

TEST(PluginClipInstance, OutputImageBoundsFollowRegionOfDefinition)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(100, 80, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	OfxRectD custom = { 1.5, 2.5, 51.5, 21.0 };
	clip.setRegionOfDefinition(custom, 3.0);

	auto *image =
		static_cast<olive::plugin::Image *>(clip.getImage(3.0, nullptr));
	ASSERT_NE(image, nullptr);
	// Integer bounds are floor(min) to ceil(max).
	EXPECT_EQ(image->width(), 51);
	EXPECT_EQ(image->height(), 19);
}

TEST(PluginClipInstance, OutputImageCacheIsPerTime)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	OFX::Host::ImageEffect::Image *first = clip.getImage(1.0, nullptr);
	OFX::Host::ImageEffect::Image *second = clip.getImage(2.0, nullptr);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);
}

TEST(PluginClipInstance, GetOutputImageUsesCache)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	OFX::Host::ImageEffect::Image *created = clip.getOutputImage(1.0);
	ASSERT_NE(created, nullptr);
	EXPECT_EQ(clip.getOutputImage(1.0), created);

	OFX::Host::ImageEffect::Image *fetched = clip.getImage(2.0, nullptr);
	ASSERT_NE(fetched, nullptr);
	EXPECT_EQ(clip.getOutputImage(2.0), fetched);
}

TEST(PluginClipInstance, InputImageNullForInvalidParams)
{
	struct Case {
		int width;
		int height;
		olive::core::PixelFormat format;
		int channels;
	};
	const Case cases[] = {
		{ 0, 10, olive::core::PixelFormat::U8, 4 },
		{ 10, 0, olive::core::PixelFormat::U8, 4 },
		{ 10, 10, olive::core::PixelFormat::INVALID, 4 },
		{ 10, 10, olive::core::PixelFormat::U8, 0 },
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
		SCOPED_TRACE(i);
		OFX::Host::ImageEffect::ClipDescriptor desc("Source");
		olive::VideoParams params = MakeParams(cases[i].width, cases[i].height,
											   cases[i].format,
											   cases[i].channels, false);
		olive::plugin::OliveClipInstance clip(nullptr, desc, params);
		EXPECT_EQ(clip.getImage(0.0, nullptr), nullptr);
	}
}

TEST(PluginClipInstance, ConnectedAfterImageBecomesAvailable)
{
	OFX::Host::ImageEffect::ClipDescriptor out_desc(
		kOfxImageEffectOutputClipName);
	olive::VideoParams out_params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance out_clip(nullptr, out_desc, out_params);
	EXPECT_FALSE(out_clip.getConnected());
	ASSERT_NE(out_clip.getImage(0.0, nullptr), nullptr);
	EXPECT_TRUE(out_clip.getConnected());

	OFX::Host::ImageEffect::ClipDescriptor src_desc("Source");
	olive::VideoParams src_params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance src_clip(nullptr, src_desc, src_params);
	EXPECT_FALSE(src_clip.getConnected());
	auto texture = std::make_shared<olive::Texture>(src_params);
	src_clip.setInputTexture(texture, 1.0, true);
	EXPECT_TRUE(src_clip.getConnected());
}

TEST(PluginClipInstance, SetInputTextureCopiesPixels)
{
	OFX::Host::ImageEffect::ClipDescriptor desc("Source");
	olive::VideoParams params =
		MakeParams(4, 2, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	olive::AVFramePtr frame = CreateFrame(params);
	ASSERT_NE(frame, nullptr);
	ASSERT_NE(frame->data(0), nullptr);

	const int values_per_row = params.width() * params.channel_count();
	for (int y = 0; y < params.height(); ++y) {
		uint8_t *row = frame->data(0) + y * frame->linesize(0);
		for (int x = 0; x < values_per_row; ++x) {
			row[x] = static_cast<uint8_t>(y * values_per_row + x + 1);
		}
	}

	auto texture = std::make_shared<olive::Texture>(params);
	texture->handleFrame(frame);
	clip.setInputTexture(texture, 1.0, true);

	auto *image =
		static_cast<olive::plugin::Image *>(clip.getImage(1.0, nullptr));
	ASSERT_NE(image, nullptr);
	ASSERT_NE(image->data(), nullptr);
	ASSERT_EQ(image->width(), params.width());
	ASSERT_EQ(image->height(), params.height());
	for (int y = 0; y < params.height(); ++y) {
		for (int x = 0; x < values_per_row; ++x) {
			EXPECT_EQ(image->data()[y * image->row_bytes() + x],
					  static_cast<uint8_t>(y * values_per_row + x + 1));
		}
	}
	image->releaseReference();
}

TEST(PluginClipInstance, SetInputTextureCopiesFloatPixels)
{
	OFX::Host::ImageEffect::ClipDescriptor desc("Source");
	olive::VideoParams params =
		MakeParams(2, 1, olive::core::PixelFormat::F32, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	olive::AVFramePtr frame = CreateFrame(params);
	ASSERT_NE(frame, nullptr);
	ASSERT_NE(frame->data(0), nullptr);

	const float expected[] = { 0.125f, 0.25f, 0.375f, 0.5f,
							   0.625f, 0.75f, 0.875f, 1.0f };
	auto *dst = reinterpret_cast<float *>(frame->data(0));
	for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
		dst[i] = expected[i];
	}

	auto texture = std::make_shared<olive::Texture>(params);
	texture->handleFrame(frame);
	clip.setInputTexture(texture, 1.0, true);

	auto *image =
		static_cast<olive::plugin::Image *>(clip.getImage(1.0, nullptr));
	ASSERT_NE(image, nullptr);
	ASSERT_NE(image->data(), nullptr);
	const auto *pixels = reinterpret_cast<const float *>(image->data());
	for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
		EXPECT_FLOAT_EQ(pixels[i], expected[i]);
	}
	image->releaseReference();
}

TEST(PluginClipInstance, SetInputTextureScrubsNaNToBlack)
{
	OFX::Host::ImageEffect::ClipDescriptor desc("Source");
	olive::VideoParams params =
		MakeParams(2, 2, olive::core::PixelFormat::F32, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	olive::AVFramePtr frame = CreateFrame(params);
	ASSERT_NE(frame, nullptr);
	ASSERT_NE(frame->data(0), nullptr);

	const int stride =
		frame->linesize(0) / static_cast<int>(sizeof(float));
	for (int y = 0; y < params.height(); ++y) {
		float *row = reinterpret_cast<float *>(frame->data(0)) + y * stride;
		for (int x = 0; x < params.width() * params.channel_count(); ++x) {
			row[x] = 0.5f;
		}
	}
	reinterpret_cast<float *>(frame->data(0))[3] =
		std::numeric_limits<float>::quiet_NaN();

	auto texture = std::make_shared<olive::Texture>(params);
	texture->handleFrame(frame);
	clip.setInputTexture(texture, 1.0, true);
	ASSERT_TRUE(clip.getConnected());

	// A frame containing NaN/Inf is replaced with black rather than being
	// passed to the plugin.
	auto *image =
		static_cast<olive::plugin::Image *>(clip.getImage(1.0, nullptr));
	ASSERT_NE(image, nullptr);
	ASSERT_NE(image->data(), nullptr);
	const auto *pixels = reinterpret_cast<const float *>(image->data());
	const int float_count =
		params.width() * params.height() * params.channel_count();
	for (int i = 0; i < float_count; ++i) {
		EXPECT_FLOAT_EQ(pixels[i], 0.0f);
	}
	image->releaseReference();
}

TEST(PluginClipInstance, PruneImagesCacheEvictsOldestInputImages)
{
	OFX::Host::ImageEffect::ClipDescriptor desc("Source");
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	for (int t = 1;
		 t <= olive::plugin::OliveClipInstance::kMaxInputImageCache + 1; ++t) {
		auto texture = std::make_shared<olive::Texture>(params);
		clip.setInputTexture(texture, static_cast<OfxTime>(t), true);
	}
	clip.pruneImagesCache();

	// The oldest entry (time 1) was evicted; its on-demand recreation is
	// cached again, while later entries remained cached throughout.
	OFX::Host::ImageEffect::Image *evicted_a = clip.getImage(1.0, nullptr);
	OFX::Host::ImageEffect::Image *evicted_b = clip.getImage(1.0, nullptr);
	EXPECT_EQ(evicted_a, evicted_b);

	OFX::Host::ImageEffect::Image *cached_a = clip.getImage(2.0, nullptr);
	OFX::Host::ImageEffect::Image *cached_b = clip.getImage(2.0, nullptr);
	EXPECT_EQ(cached_a, cached_b);

	evicted_a->releaseReference();
	evicted_b->releaseReference();
	cached_a->releaseReference();
	cached_b->releaseReference();
}

TEST(PluginClipInstance, PruneImagesCacheKeepsOutputImages)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	for (int t = 1;
		 t <= olive::plugin::OliveClipInstance::kMaxInputImageCache + 1; ++t) {
		clip.getImage(static_cast<OfxTime>(t), nullptr);
	}
	OFX::Host::ImageEffect::Image *before = clip.getImage(1.0, nullptr);
	ASSERT_NE(before, nullptr);

	// Pruning is a no-op for the output clip.
	clip.pruneImagesCache();
	EXPECT_EQ(clip.getImage(1.0, nullptr), before);
}

#ifdef OFX_SUPPORTS_OPENGLRENDER
TEST(PluginClipInstance, LoadTextureReturnsNullWithoutGpuTexture)
{
	OFX::Host::ImageEffect::ClipDescriptor src_desc("Source");
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance src_clip(nullptr, src_desc, params);

	// No texture was supplied at all.
	EXPECT_EQ(src_clip.loadTexture(1.0, nullptr, nullptr), nullptr);

	// A dummy (CPU-only) texture has no GL id, so no OFX texture is made.
	auto texture = std::make_shared<olive::Texture>(params);
	src_clip.setInputTexture(texture, 1.0, false);
	EXPECT_EQ(src_clip.loadTexture(1.0, nullptr, nullptr), nullptr);

	OFX::Host::ImageEffect::ClipDescriptor out_desc(
		kOfxImageEffectOutputClipName);
	olive::plugin::OliveClipInstance out_clip(nullptr, out_desc, params);
	out_clip.setOutputTexture(texture, 2.0);
	EXPECT_TRUE(out_clip.getConnected());
	EXPECT_EQ(out_clip.loadTexture(2.0, nullptr, nullptr), nullptr);
}
#endif

TEST(PluginClipInstance, SetParamsUpdatesClipAndPluginPreferences)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(16, 16, olive::core::PixelFormat::U8, 4, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	olive::VideoParams updated =
		MakeParams(32, 24, olive::core::PixelFormat::F32, 4, false);
	updated.set_frame_rate(olive::core::rational(60, 1));
	clip.setParams(updated);

	EXPECT_DOUBLE_EQ(clip.getFrameRate(), 60.0);
	EXPECT_EQ(clip.getUnmappedBitDepth(), kOfxBitDepthFloat);

	olive::VideoParams preferred = clip.getPluginPreferredParams();
	EXPECT_EQ(preferred.format(), olive::core::PixelFormat::F32);
	EXPECT_EQ(preferred.channel_count(), 4);
}

TEST(PluginClipInstance, PluginPreferredParamsDefaultsToClipParams)
{
	OFX::Host::ImageEffect::ClipDescriptor desc(kOfxImageEffectOutputClipName);
	olive::VideoParams params =
		MakeParams(64, 32, olive::core::PixelFormat::U16, 3, false);
	olive::plugin::OliveClipInstance clip(nullptr, desc, params);

	olive::VideoParams preferred = clip.getPluginPreferredParams();
	EXPECT_EQ(preferred.format(), olive::core::PixelFormat::U16);
	EXPECT_EQ(preferred.channel_count(), 3);
	EXPECT_EQ(preferred.width(), 64);
	EXPECT_EQ(preferred.height(), 32);

	clip.setPixelDepth(kOfxBitDepthByte);
	clip.setComponents(kOfxImageComponentRGBA);
	preferred = clip.getPluginPreferredParams();
	EXPECT_EQ(preferred.format(), olive::core::PixelFormat::U8);
	EXPECT_EQ(preferred.channel_count(), 4);
	EXPECT_EQ(preferred.width(), 64);
}
