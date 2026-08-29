#include <gtest/gtest.h>

#include <QOpenGLContext>
#include <QVariant>
#include <cstdlib>

#include "render/opengl/openglrenderer.h"

TEST(OpenGLRenderer, DownloadFromTextureWithoutCurrentContext)
{
	if (std::getenv("CI")) {
		GTEST_SKIP() << "Skipping OpenGL test in CI environment";
	}

	QOpenGLContext context;
	if (!context.create()) {
		GTEST_SKIP()
			<< "Skipping OpenGL test because no context can be created";
	}
	ASSERT_EQ(QOpenGLContext::currentContext(), nullptr);

	olive::OpenGLRenderer renderer;
	renderer.Init(&context);

	olive::VideoParams params(4, 4, olive::core::PixelFormat::U8, 4,
							  olive::core::rational(1, 1),
							  olive::VideoParams::kInterlaceNone, 1);

	unsigned char buffer[4 * 4 * 4] = {};
	renderer.DownloadFromTexture(QVariant::fromValue<GLuint>(0), params, buffer,
								 4 * 4);

	EXPECT_EQ(QOpenGLContext::currentContext(), nullptr);
}
