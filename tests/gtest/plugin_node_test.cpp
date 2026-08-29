/*
 * Oak Video Editor - Plugin Node / OFX Host Tests
 * Copyright (C) 2025 Olive CE Team
 *
 * CPU-only tests for app/pluginSupport/OliveHost.cpp and the constructible
 * surface of app/node/plugins/Plugin.h.
 *
 * PluginNode and OlivePluginInstance cannot be instantiated in a unit test:
 * PluginNode's constructor dereferences an OFX::Host::ImageEffect::Instance,
 * and the HostSupport Instance constructor requires a plugin binary that
 * dlopen()s successfully (ofxhImageEffect.cpp calls
 * plugin->getPluginHandle()->getOfxPlugin(), which needs the OfxGetPlugin
 * symbol of a real .ofx bundle). The host-side surface (pluginSupported,
 * descriptor factory, message routing, loadPlugins) is exercised here with a
 * fake ImageEffectPlugin backed by a deliberately invalid PluginBinary.
 */

#include <gtest/gtest.h>

#include <cstdarg>
#include <memory>
#include <string>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxMessage.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginCache.h"

#include "common/Current.h"
#include "node/plugins/Plugin.h"
#include "pluginSupport/OliveHost.h"
#include "version.h"

namespace
{

// Paths that never exist on disk: the PluginBinary stats the file, marks
// itself invalid, and every code path used below tolerates that without ever
// calling dlopen().
constexpr char kFakeBundlePath[] = "/nonexistent/Fake.ofx.bundle";
constexpr char kFakeBinaryPath[] =
	"/nonexistent/Fake.ofx.bundle/Contents/Linux-x86-64/Fake.ofx";
constexpr char kFakePluginId[] = "com.oak.test.FakePlugin";

// Builds an OliveHost, an ImageEffect::PluginCache bound to it (which sets
// the global gImageEffectHost), an invalid PluginBinary, and a fake
// ImageEffectPlugin whose construction runs through OliveHost::makeDescriptor.
// The saver member restores gImageEffectHost on destruction so other tests
// keep observing the global state they set up themselves.
struct FakePluginHarness {
	struct HostGlobalSaver {
		HostGlobalSaver()
			: previous(OFX::Host::ImageEffect::gImageEffectHost)
		{
		}
		~HostGlobalSaver()
		{
			OFX::Host::ImageEffect::gImageEffectHost = previous;
		}
		OFX::Host::ImageEffect::Host *previous;
	};

	HostGlobalSaver saver;
	olive::plugin::OliveHost host;
	OFX::Host::ImageEffect::PluginCache cache{ host };
	OFX::Host::PluginBinary binary{ kFakeBinaryPath, kFakeBundlePath, 0, 0 };
	OFX::Host::ImageEffect::ImageEffectPlugin plugin{ cache,	 &binary, 0,
													  kOfxImageEffectPluginApi,
													  1,
													  kFakePluginId,
													  kFakePluginId,
													  1,
													  0 };
};

OfxStatus CallVMessage(olive::plugin::OliveHost &host, const char *type,
					   const char *id, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	const OfxStatus status = host.vmessage(type, id, format, args);
	va_end(args);
	return status;
}

OfxStatus CallSetPersistentMessage(olive::plugin::OliveHost &host,
								   const char *type, const char *id,
								   const char *format, ...)
{
	va_list args;
	va_start(args, format);
	const OfxStatus status = host.setPersistentMessage(type, id, format, args);
	va_end(args);
	return status;
}

} // namespace

// ============================================================================
// OliveHost::pluginSupported
// ============================================================================

TEST(OliveHost, PluginSupportedRejectsNullPlugin)
{
	FakePluginHarness harness;

	std::string reason;
	EXPECT_FALSE(harness.host.pluginSupported(nullptr, reason));
	EXPECT_EQ(reason, "null plugin");
}

TEST(OliveHost, PluginSupportedRejectsPluginWithoutContexts)
{
	FakePluginHarness harness;

	// The fake plugin was never described, so it supports no contexts.
	std::string reason;
	EXPECT_FALSE(harness.host.pluginSupported(&harness.plugin, reason));
	EXPECT_EQ(reason, "no supported contexts (describe failed)");
}

TEST(OliveHost, PluginSupportedAcceptsPluginWithKnownContext)
{
	FakePluginHarness harness;
	harness.plugin.addContext(kOfxImageEffectContextFilter);

	std::string reason;
	EXPECT_TRUE(harness.host.pluginSupported(&harness.plugin, reason));
	EXPECT_TRUE(reason.empty());
}

TEST(OliveHost, FakePluginExposesConstructionMetadata)
{
	FakePluginHarness harness;

	EXPECT_EQ(harness.plugin.getIdentifier(), kFakePluginId);
	EXPECT_EQ(harness.plugin.getVersionMajor(), 1);
	EXPECT_EQ(harness.plugin.getVersionMinor(), 0);
	// Construction went through OliveHost::makeDescriptor(plugin), which
	// stamps the descriptor with the binary's bundle path.
	EXPECT_EQ(harness.plugin.getDescriptor().getProps().getStringProperty(
				  kOfxPluginPropFilePath),
			  kFakeBundlePath);
}

// ============================================================================
// OliveHost::makeDescriptor (descriptor store)
// ============================================================================

TEST(OliveHost, MakeDescriptorFromBundlePathRecordsFilePath)
{
	FakePluginHarness harness;

	auto first = harness.host.makeDescriptor(
		std::string("/nonexistent/One.ofx.bundle"), nullptr);
	auto second = harness.host.makeDescriptor(
		std::string("/nonexistent/Two.ofx.bundle"), nullptr);

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_NE(first, second);
	EXPECT_EQ(first->getProps().getStringProperty(kOfxPluginPropFilePath),
			  "/nonexistent/One.ofx.bundle");
	EXPECT_EQ(second->getProps().getStringProperty(kOfxPluginPropFilePath),
			  "/nonexistent/Two.ofx.bundle");
	EXPECT_EQ(first->getProps().getStringProperty(kOfxPropType),
			  kOfxTypeImageEffect);
}

TEST(OliveHost, MakeDescriptorFromRootContextCopiesProperties)
{
	FakePluginHarness harness;

	auto root = harness.host.makeDescriptor(
		std::string("/nonexistent/Root.ofx.bundle"), nullptr);
	ASSERT_NE(root, nullptr);
	root->getProps().setStringProperty(kOfxPropLabel, "Root Label");

	auto desc = harness.host.makeDescriptor(*root, &harness.plugin);
	ASSERT_NE(desc, nullptr);

	// Properties are inherited from the root context ...
	EXPECT_EQ(desc->getProps().getStringProperty(kOfxPropLabel),
			  "Root Label");
	// ... while the file path is stamped from the plugin's own binary.
	EXPECT_EQ(desc->getProps().getStringProperty(kOfxPluginPropFilePath),
			  kFakeBundlePath);
}

// ============================================================================
// OliveHost::destroyInstance
// ============================================================================

TEST(OliveHost, DestroyInstanceIgnoresNull)
{
	olive::plugin::OliveHost host;
	EXPECT_NO_THROW(host.destroyInstance(nullptr));
}

// ============================================================================
// OliveHost message routing
//
// With a QApplication on a non-offscreen platform the successful
// vmessage/setPersistentMessage paths pop modal QMessageBox dialogs, which a
// headless test cannot dismiss; on the offscreen platform they log to stderr
// instead, so those paths are exercised here behind a platform check.
// ============================================================================

TEST(OliveHost, VMessageRejectsNullArguments)
{
	olive::plugin::OliveHost host;
	EXPECT_EQ(CallVMessage(host, nullptr, "id", "%s", "x"), kOfxStatFailed);
	EXPECT_EQ(CallVMessage(host, kOfxMessageError, "id", nullptr),
			  kOfxStatFailed);
}

TEST(OliveHost, SetPersistentMessageRejectsNullArguments)
{
	olive::plugin::OliveHost host;
	EXPECT_EQ(CallSetPersistentMessage(host, nullptr, "id", "%s", "x"),
			  kOfxStatFailed);
	EXPECT_EQ(CallSetPersistentMessage(host, kOfxMessageError, "id", nullptr),
			  kOfxStatFailed);
}

TEST(OliveHost, SetPersistentMessageRejectsUnknownType)
{
	olive::plugin::OliveHost host;
	// A type that is neither error, warning, nor message fails before any
	// dialog would be shown.
	EXPECT_EQ(CallSetPersistentMessage(host, "OfxMessageBogus", "id", "%s",
									   "hello"),
			  kOfxStatFailed);
}

TEST(OliveHost, VMessageOffscreenLogsInsteadOfDialog)
{
	if (QGuiApplication::platformName() != QLatin1String("offscreen")) {
		GTEST_SKIP() << "requires the offscreen QPA platform";
	}

	olive::plugin::OliveHost host;
	// No modal dialog is shown on the offscreen platform; the message is
	// logged to stderr and acknowledged.
	EXPECT_EQ(CallVMessage(host, kOfxMessageError, "id", "%s", "boom"),
			  kOfxStatOK);
	EXPECT_EQ(CallVMessage(host, kOfxMessageWarning, "id", "%s", "boom"),
			  kOfxStatOK);
	EXPECT_EQ(CallVMessage(host, kOfxMessageMessage, "id", "%s", "boom"),
			  kOfxStatOK);
	// A question cannot be answered headlessly, so it is a "no".
	EXPECT_EQ(CallVMessage(host, kOfxMessageQuestion, "id", "%s", "boom"),
			  kOfxStatReplyNo);
}

TEST(OliveHost, SetPersistentMessageOffscreenSucceeds)
{
	if (QGuiApplication::platformName() != QLatin1String("offscreen")) {
		GTEST_SKIP() << "requires the offscreen QPA platform";
	}

	olive::plugin::OliveHost host;
	EXPECT_EQ(CallSetPersistentMessage(host, kOfxMessageError, "id", "%s",
									   "boom"),
			  kOfxStatOK);
	EXPECT_EQ(CallSetPersistentMessage(host, kOfxMessageWarning, "id", "%s",
									   "boom"),
			  kOfxStatOK);
	EXPECT_EQ(CallSetPersistentMessage(host, kOfxMessageMessage, "id", "%s",
									   "boom"),
			  kOfxStatOK);
}

TEST(OliveHost, ClearPersistentMessageSucceeds)
{
	olive::plugin::OliveHost host;
	EXPECT_EQ(host.clearPersistentMessage(), kOfxStatOK);
}

// ============================================================================
// OliveHost suite/property surface (inherited from OFX::Host::ImageEffect::Host)
// ============================================================================

TEST(OliveHost, FetchSuiteProvidesExpectedSuites)
{
	olive::plugin::OliveHost host;

	EXPECT_NE(host.fetchSuite(kOfxImageEffectSuite, 1), nullptr);
	EXPECT_EQ(host.fetchSuite(kOfxImageEffectSuite, 2), nullptr);
	EXPECT_NE(host.fetchSuite(kOfxPropertySuite, 1), nullptr);
	EXPECT_NE(host.fetchSuite(kOfxMemorySuite, 1), nullptr);
	EXPECT_NE(host.fetchSuite(kOfxMessageSuite, 1), nullptr);
	EXPECT_NE(host.fetchSuite(kOfxMessageSuite, 2), nullptr);
	EXPECT_NE(host.fetchSuite(kOfxParameterSuite, 1), nullptr);
	EXPECT_EQ(host.fetchSuite("com.oak.BogusSuite", 1), nullptr);
}

TEST(OliveHost, HostPropertiesIdentifyAsOfxHost)
{
	// HostSupport seeds the host property set with the literal "Host"
	// (ofxhHost.cpp hostStuffs), not kOfxTypeImageEffectHost.
	olive::plugin::OliveHost host;
	EXPECT_EQ(host.getProperties().getStringProperty(kOfxPropType), "Host");
}

TEST(OliveHost, HostPropertiesIdentifyApplication)
{
	// The ctor stamps the app identity over HostSupport's "UNKNOWN" defaults
	// so plugins querying the host description see real values.
	olive::plugin::OliveHost host;
	const auto &props = host.getProperties();

	EXPECT_EQ(props.getStringProperty(kOfxPropName), "Oak Video Editor");
	EXPECT_EQ(props.getStringProperty(kOfxPropLabel), "Oak Video Editor");
	EXPECT_EQ(props.getStringProperty(kOfxPropVersionLabel),
			  olive::kAppVersion.toStdString());

	const QStringList version_parts =
		olive::kAppVersion.section(QLatin1Char('-'), 0, 0)
			.split(QLatin1Char('.'));
	EXPECT_EQ(props.getIntProperty(kOfxPropVersion, 0),
			  version_parts.value(0).toInt());
	EXPECT_EQ(props.getIntProperty(kOfxPropVersion, 1),
			  version_parts.value(1).toInt());
	EXPECT_EQ(props.getIntProperty(kOfxPropVersion, 2),
			  version_parts.value(2).toInt());
}

#ifdef OFX_SUPPORTS_OPENGLRENDER
TEST(OliveHost, FlushOpenGLResourcesReportsFailure)
{
	// No GL context exists in a headless test; the host must report failure
	// rather than crash.
	olive::plugin::OliveHost host;
	EXPECT_EQ(host.flushOpenGLResources(), kOfxStatFailed);
}
#endif

// ============================================================================
// olive::plugin::loadPlugins
// ============================================================================

TEST(OliveHost, LoadPluginsInitializesAndReusesCurrentHost)
{
	olive::plugin::loadPlugins(QString());

	std::shared_ptr<olive::plugin::OliveHost> host =
		Current::getInstance().pluginHost();
	std::shared_ptr<OFX::Host::ImageEffect::PluginCache> cache =
		Current::getInstance().pluginCache();
	ASSERT_NE(host, nullptr);
	ASSERT_NE(cache, nullptr);

	// A second call must reuse the already-created host and cache.
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	olive::plugin::loadPlugins(dir.path());

	EXPECT_EQ(Current::getInstance().pluginHost(), host);
	EXPECT_EQ(Current::getInstance().pluginCache(), cache);
}

TEST(OliveHost, LoadPluginsScansBundleWithoutValidBinary)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	// Minimal .ofx.bundle layout with a file that is not a loadable shared
	// object: the scanner must mark it invalid and move on.
	const QString arch_dir = dir.filePath(QStringLiteral(
		"Fake.ofx.bundle/Contents/Linux-x86-64"));
	ASSERT_TRUE(QDir().mkpath(arch_dir));
	QFile fake_binary(dir.filePath(QStringLiteral(
		"Fake.ofx.bundle/Contents/Linux-x86-64/Fake.ofx")));
	ASSERT_TRUE(fake_binary.open(QIODevice::WriteOnly));
	fake_binary.write("not a shared object");
	fake_binary.close();

	EXPECT_NO_THROW(olive::plugin::loadPlugins(dir.path()));

	EXPECT_NE(OFX::Host::PluginCache::getPluginCache(), nullptr);
	// The invalid bundle must not have registered any plugin.
	for (auto *plug : OFX::Host::PluginCache::getPluginCache()->getPlugins()) {
		ASSERT_NE(plug, nullptr);
		EXPECT_NE(plug->getIdentifier(), "Fake");
	}
}

// ============================================================================
// PluginNode
//
// See the file header: PluginNode needs a real OFX instance and cannot be
// built without a plugin bundle. What remains constructible is the fallback
// input id the node uses when an effect declares no usable source clip.
// ============================================================================

TEST(PluginNode, TextureInputFallbackIdIsStable)
{
	EXPECT_EQ(olive::plugin::kTextureInput, QStringLiteral("tex_in"));
}
