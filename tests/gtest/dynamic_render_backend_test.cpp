#include <gtest/gtest.h>

#include <QByteArray>
#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QThread>

#include "node/value.h"
#include "render/backend/dynamicrenderer.h"
#include "render/job/shaderjob.h"
#include "render/shadercode.h"
#include "render/texture.h"
#include "render/videoparams.h"

// Verifies that the dynamic adapter can load the private OpenGL backend and
// query its advertised C ABI capabilities without creating a viewer context.
TEST(DynamicRenderBackend, LoadsExperimentalOpenGLBackend)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("opengl"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "opengl backend library could not be loaded in this environment";
	}

	EXPECT_EQ(renderer.OpenGLContext(), nullptr);
	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	EXPECT_EQ(info.abi_version, 1U);
	EXPECT_EQ(info.kind, OAK_RENDER_BACKEND_OPENGL);
	EXPECT_STREQ(info.name, "opengl");
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_TEXTURES);
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_SHADERS);
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_BLIT);
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_READBACK);
#endif
}

// Regression test: the backend renderer must follow DynamicRenderer when it is
// moved to a background thread. If it stays in the thread where Load() was
// called, GL operations are rejected as "wrong thread" and texture creation
// returns null, which manifests as a black screen.
TEST(DynamicRenderBackend, OpenGLBackendFollowsAdapterToRenderThread)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("opengl"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "opengl backend library could not be loaded in this environment";
	}

	if (!renderer.Init()) {
		GTEST_SKIP()
			<< "OpenGL backend could not be initialized on this system";
	}

	QThread render_thread;
	renderer.moveToThread(&render_thread);
	render_thread.start();

	QOpenGLContext *ctx = renderer.OpenGLContext();
	ASSERT_NE(ctx, nullptr);
	EXPECT_EQ(ctx->thread(), &render_thread)
		<< "Backend OpenGL context did not follow DynamicRenderer to render thread";

	// Exercise the actual GL path in the render thread: PostInit() creates the
	// offscreen surface there, and CreateTexture() must not crash.
	olive::TexturePtr texture;
	QMetaObject::invokeMethod(
		&renderer,
		[&]() {
			renderer.PostInit();
			texture = renderer.CreateTexture(
				olive::VideoParams(64, 64, olive::PixelFormat::U8,
								   olive::VideoParams::kRGBAChannelCount));
		},
		Qt::BlockingQueuedConnection);

	render_thread.quit();
	render_thread.wait();

	ASSERT_NE(texture, nullptr);
	EXPECT_FALSE(texture->IsDummy());
#endif
}

// Verifies Vulkan backend discovery on systems with a working Vulkan ICD. The
// test skips when the runtime correctly reports Vulkan as unavailable.
TEST(DynamicRenderBackend, LoadsExperimentalVulkanBackendWhenAvailable)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("vulkan"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "vulkan backend library could not be loaded in this environment";
	}

	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	if (info.kind != OAK_RENDER_BACKEND_VULKAN) {
		GTEST_SKIP() << "Vulkan backend is not available on this system";
	}
	EXPECT_EQ(renderer.backend_name(), QStringLiteral("vulkan"));
	EXPECT_EQ(renderer.OpenGLContext(), nullptr);
	EXPECT_STREQ(info.name, "vulkan");
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_TEXTURES);
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_SHADERS);
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_BLIT);
	EXPECT_TRUE(info.capabilities & OAK_RENDER_BACKEND_CAP_READBACK);
#endif
}

// Verifies that requesting Vulkan on systems without a usable runtime falls
// back to OpenGL and reports the effective backend name.
TEST(DynamicRenderBackend, FallsBackWhenExperimentalVulkanUnavailable)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("vulkan"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "vulkan backend library could not be loaded in this environment";
	}

	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	if (info.kind == OAK_RENDER_BACKEND_VULKAN) {
		GTEST_SKIP()
			<< "Vulkan backend is available on this system; skip fallback test";
	}
	EXPECT_EQ(renderer.backend_name(), QStringLiteral("opengl"));
	EXPECT_EQ(renderer.OpenGLContext(), nullptr);
	EXPECT_EQ(info.kind, OAK_RENDER_BACKEND_OPENGL);
	EXPECT_STREQ(info.name, "opengl");
#endif
}

// Exercises the minimal Vulkan render loop: upload a texture, run a pass-through
// shader blit, then download the destination and verify pixel data.
TEST(DynamicRenderBackend, VulkanUploadBlitDownload)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("vulkan"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "vulkan backend library could not be loaded in this environment";
	}

	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	if (info.kind != OAK_RENDER_BACKEND_VULKAN) {
		GTEST_SKIP() << "Vulkan backend is not available on this system";
	}

	ASSERT_TRUE(renderer.Init());
	renderer.PostInit();

	const int kSize = 64;
	olive::VideoParams params(kSize, kSize, olive::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);

	olive::TexturePtr src = renderer.CreateTexture(params);
	ASSERT_NE(src, nullptr);
	ASSERT_FALSE(src->IsDummy());

	QByteArray src_data(kSize * kSize * 4, 0);
	for (int i = 0; i < kSize * kSize; ++i) {
		src_data[i * 4 + 0] = static_cast<char>(255); // R
		src_data[i * 4 + 1] = static_cast<char>(0); // G
		src_data[i * 4 + 2] = static_cast<char>(0); // B
		src_data[i * 4 + 3] = static_cast<char>(255); // A
	}
	src->Upload(src_data.data(), kSize);

	olive::TexturePtr dst = renderer.CreateTexture(params);
	ASSERT_NE(dst, nullptr);
	ASSERT_FALSE(dst->IsDummy());

	const QString vert =
		QStringLiteral("uniform mat4 ove_mvpmat;\n"
					   "in vec4 a_position;\n"
					   "in vec2 a_texcoord;\n"
					   "out vec2 ove_texcoord;\n"
					   "void main() {\n"
					   "    gl_Position = ove_mvpmat * a_position;\n"
					   "    ove_texcoord = a_texcoord;\n"
					   "}\n");
	const QString frag =
		QStringLiteral("uniform sampler2D ove_maintex;\n"
					   "in vec2 ove_texcoord;\n"
					   "out vec4 frag_color;\n"
					   "void main() {\n"
					   "    frag_color = texture(ove_maintex, ove_texcoord);\n"
					   "}\n");
	QVariant shader =
		renderer.CreateNativeShader(olive::ShaderCode(frag, vert));
	ASSERT_FALSE(shader.isNull());

	olive::ShaderJob job;
	job.Insert(QStringLiteral("ove_maintex"),
			   olive::NodeValue(olive::NodeValue::kTexture,
								QVariant::fromValue(src)));
	job.Insert(QStringLiteral("ove_mvpmat"),
			   olive::NodeValue(olive::NodeValue::kMatrix, QMatrix4x4()));

	renderer.BlitToTexture(shader, job, dst.get(), true);

	QByteArray dst_data(kSize * kSize * 4, 0);
	dst->Download(dst_data.data(), kSize);

	// The default pass-through shader should reproduce the red source pixel.
	EXPECT_EQ(static_cast<uint8_t>(dst_data[0]), 255u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[1]), 0u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[2]), 0u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[3]), 255u);
#endif
}

// Ensures the Vulkan backend handles null-destination blits by rendering to a
// temporary offscreen target instead of dereferencing a missing framebuffer.
TEST(DynamicRenderBackend, VulkanNullDestinationBlitDoesNotCrash)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("vulkan"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "vulkan backend library could not be loaded in this environment";
	}

	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	if (info.kind != OAK_RENDER_BACKEND_VULKAN) {
		GTEST_SKIP() << "Vulkan backend is not available on this system";
	}

	ASSERT_TRUE(renderer.Init());
	renderer.PostInit();

	const int kSize = 32;
	olive::VideoParams params(kSize, kSize, olive::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);

	olive::TexturePtr src = renderer.CreateTexture(params);
	ASSERT_NE(src, nullptr);
	ASSERT_FALSE(src->IsDummy());

	QByteArray src_data(kSize * kSize * 4, 0);
	for (int i = 0; i < kSize * kSize; ++i) {
		src_data[i * 4 + 0] = static_cast<char>(255);
		src_data[i * 4 + 3] = static_cast<char>(255);
	}
	src->Upload(src_data.data(), kSize);

	const QString vert =
		QStringLiteral("uniform mat4 ove_mvpmat;\n"
					   "in vec4 a_position;\n"
					   "in vec2 a_texcoord;\n"
					   "out vec2 ove_texcoord;\n"
					   "void main() {\n"
					   "    gl_Position = ove_mvpmat * a_position;\n"
					   "    ove_texcoord = a_texcoord;\n"
					   "}\n");
	const QString frag =
		QStringLiteral("uniform sampler2D ove_maintex;\n"
					   "in vec2 ove_texcoord;\n"
					   "out vec4 frag_color;\n"
					   "void main() {\n"
					   "    frag_color = texture(ove_maintex, ove_texcoord);\n"
					   "}\n");
	QVariant shader =
		renderer.CreateNativeShader(olive::ShaderCode(frag, vert));
	ASSERT_FALSE(shader.isNull());

	olive::ShaderJob job;
	job.Insert(QStringLiteral("ove_maintex"),
			   olive::NodeValue(olive::NodeValue::kTexture,
								QVariant::fromValue(src)));
	job.Insert(QStringLiteral("ove_mvpmat"),
			   olive::NodeValue(olive::NodeValue::kMatrix, QMatrix4x4()));

	// Null-destination Blit has no render target; it should simply not crash.
	renderer.Blit(shader, job, params, true);
	SUCCEED();
#endif
}

// Verifies iterative shader support: the first pass writes to a temporary
// texture and the second pass samples it before writing the final destination.
TEST(DynamicRenderBackend, VulkanIterativeBlitPingPong)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("vulkan"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "vulkan backend library could not be loaded in this environment";
	}

	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	if (info.kind != OAK_RENDER_BACKEND_VULKAN) {
		GTEST_SKIP() << "Vulkan backend is not available on this system";
	}

	ASSERT_TRUE(renderer.Init());
	renderer.PostInit();

	const int kSize = 32;
	olive::VideoParams params(kSize, kSize, olive::PixelFormat::U8,
							  olive::VideoParams::kRGBAChannelCount);

	olive::TexturePtr src = renderer.CreateTexture(params);
	ASSERT_NE(src, nullptr);
	ASSERT_FALSE(src->IsDummy());

	// Start with a fully red texture.
	QByteArray src_data(kSize * kSize * 4, 0);
	for (int i = 0; i < kSize * kSize; ++i) {
		src_data[i * 4 + 0] = static_cast<char>(255);
		src_data[i * 4 + 3] = static_cast<char>(255);
	}
	src->Upload(src_data.data(), kSize);

	olive::TexturePtr dst = renderer.CreateTexture(params);
	ASSERT_NE(dst, nullptr);
	ASSERT_FALSE(dst->IsDummy());

	// Shader that samples the iterative input and scales RGB by 0.5 each pass.
	const QString vert =
		QStringLiteral("uniform mat4 ove_mvpmat;\n"
					   "in vec4 a_position;\n"
					   "in vec2 a_texcoord;\n"
					   "out vec2 ove_texcoord;\n"
					   "void main() {\n"
					   "    gl_Position = ove_mvpmat * a_position;\n"
					   "    ove_texcoord = a_texcoord;\n"
					   "}\n");
	const QString frag =
		QStringLiteral("uniform sampler2D ove_maintex;\n"
					   "in vec2 ove_texcoord;\n"
					   "out vec4 frag_color;\n"
					   "void main() {\n"
					   "    vec4 c = texture(ove_maintex, ove_texcoord);\n"
					   "    frag_color = vec4(c.rgb * 0.5, c.a);\n"
					   "}\n");
	QVariant shader =
		renderer.CreateNativeShader(olive::ShaderCode(frag, vert));
	ASSERT_FALSE(shader.isNull());

	olive::ShaderJob job;
	job.Insert(QStringLiteral("ove_maintex"),
			   olive::NodeValue(olive::NodeValue::kTexture,
								QVariant::fromValue(src)));
	job.Insert(QStringLiteral("ove_mvpmat"),
			   olive::NodeValue(olive::NodeValue::kMatrix, QMatrix4x4()));
	job.SetIterations(2, QStringLiteral("ove_maintex"));

	renderer.BlitToTexture(shader, job, dst.get(), true);

	QByteArray dst_data(kSize * kSize * 4, 0);
	dst->Download(dst_data.data(), kSize);

	// After two halving passes, red is 255 * 0.5 * 0.5. UNORM conversion floors
	// the intermediate value, so the result is 63 rather than 64.
	EXPECT_EQ(static_cast<uint8_t>(dst_data[0]), 63u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[1]), 0u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[2]), 0u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[3]), 255u);
#endif
}

// Verifies RGB upload/download when the Vulkan driver stores the texture in a
// wider renderable format such as RGBA.
TEST(DynamicRenderBackend, VulkanUploadDownloadThreeChannel)
{
#ifndef OAK_ENABLE_DYNAMIC_RENDER_BACKEND
	GTEST_SKIP() << "Dynamic render backend is not enabled in this build";
#else
	olive::DynamicRenderer renderer(QStringLiteral("vulkan"));
	if (!renderer.Load()) {
		GTEST_SKIP()
			<< "vulkan backend library could not be loaded in this environment";
	}

	OakRenderBackendInfo info = {};
	ASSERT_TRUE(renderer.GetBackendInfo(&info));
	if (info.kind != OAK_RENDER_BACKEND_VULKAN) {
		GTEST_SKIP() << "Vulkan backend is not available on this system";
	}

	ASSERT_TRUE(renderer.Init());
	renderer.PostInit();

	const int kSize = 16;
	olive::VideoParams params(kSize, kSize, olive::PixelFormat::U8,
							  olive::VideoParams::kRGBChannelCount);

	olive::TexturePtr tex = renderer.CreateTexture(params);
	ASSERT_NE(tex, nullptr);
	ASSERT_FALSE(tex->IsDummy());

	QByteArray src_data(kSize * kSize * 3, 0);
	for (int i = 0; i < kSize * kSize; ++i) {
		src_data[i * 3 + 0] = static_cast<char>(255);
		src_data[i * 3 + 1] = static_cast<char>(128);
		src_data[i * 3 + 2] = static_cast<char>(64);
	}
	tex->Upload(src_data.data(), kSize);

	QByteArray dst_data(kSize * kSize * 3, 0);
	tex->Download(dst_data.data(), kSize);

	EXPECT_EQ(static_cast<uint8_t>(dst_data[0]), 255u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[1]), 128u);
	EXPECT_EQ(static_cast<uint8_t>(dst_data[2]), 64u);
#endif
}
