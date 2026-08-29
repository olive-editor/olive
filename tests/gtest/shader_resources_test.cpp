#include <gtest/gtest.h>

#include <QFile>

TEST(Shaders, ResourcesAvailable)
{
	const QStringList shader_paths = {
		QStringLiteral(":/shaders/default.frag"),
		QStringLiteral(":/shaders/default.vert"),
		QStringLiteral(":/shaders/yuv2rgb.frag"),
		QStringLiteral(":/shaders/deinterlace2.frag"),
		QStringLiteral(":/shaders/rgbhistogram.frag"),
		QStringLiteral(":/shaders/rgbhistogram.vert"),
		QStringLiteral(":/shaders/rgbhistogram_secondary.frag"),
		QStringLiteral(":/shaders/rgbvectorscope.frag"),
		QStringLiteral(":/shaders/rgbvectorscope.vert"),
		QStringLiteral(":/shaders/rgbwaveform.frag"),
		QStringLiteral(":/shaders/rgbwaveform.vert"),
		QStringLiteral(":/shaders/threewaycolor.frag"),
		QStringLiteral(":/shaders/alphaover.frag"),
		QStringLiteral(":/shaders/blur.frag"),
		QStringLiteral(":/shaders/chromakey.frag"),
		QStringLiteral(":/shaders/colordifferencekey.frag"),
		QStringLiteral(":/shaders/colormanage.frag"),
		QStringLiteral(":/shaders/cornerpin.frag"),
		QStringLiteral(":/shaders/cornerpin.vert"),
		QStringLiteral(":/shaders/crop.frag"),
		QStringLiteral(":/shaders/crossdissolve.frag"),
		QStringLiteral(":/shaders/deinterlace.frag"),
		QStringLiteral(":/shaders/despill.frag"),
		QStringLiteral(":/shaders/diptoblack.frag"),
		QStringLiteral(":/shaders/dropshadow.frag"),
		QStringLiteral(":/shaders/flip.frag"),
		QStringLiteral(":/shaders/interlace.frag"),
		QStringLiteral(":/shaders/invertrgb.frag"),
		QStringLiteral(":/shaders/invertrgba.frag"),
		QStringLiteral(":/shaders/mosaic.frag"),
		QStringLiteral(":/shaders/multiply.frag"),
		QStringLiteral(":/shaders/noise.frag"),
		QStringLiteral(":/shaders/opacity.frag"),
		QStringLiteral(":/shaders/opacity_rgb.frag"),
		QStringLiteral(":/shaders/rgb.frag"),
		QStringLiteral(":/shaders/ripple.frag"),
		QStringLiteral(":/shaders/shape.frag"),
		QStringLiteral(":/shaders/solid.frag"),
		QStringLiteral(":/shaders/stroke.frag"),
		QStringLiteral(":/shaders/swirl.frag"),
		QStringLiteral(":/shaders/tile.frag"),
		QStringLiteral(":/shaders/wave.frag"),
	};

	for (const QString &path : shader_paths) {
		QFile file(path);
		ASSERT_TRUE(file.exists())
			<< "Missing shader resource: " << path.toStdString();
		ASSERT_TRUE(file.open(QIODevice::ReadOnly))
			<< "Failed to open shader resource: " << path.toStdString();
		const QByteArray contents = file.readAll();
		EXPECT_FALSE(contents.isEmpty())
			<< "Shader resource is empty: " << path.toStdString();
	}
}
