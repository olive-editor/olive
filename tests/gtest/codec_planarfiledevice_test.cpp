#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>
#include <QVector>

#include "codec/planarfiledevice.h"

TEST(CodecPlanarFileDevice, WriteThenReadRoundTrip)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QVector<QString> names = {
		dir.filePath(QStringLiteral("ch0.bin")),
		dir.filePath(QStringLiteral("ch1.bin"))
	};

	const qint64 bytes = 1024;
	QVector<QByteArray> src(2);
	src[0].resize(int(bytes));
	src[1].resize(int(bytes));
	for (int i = 0; i < bytes; i++) {
		src[0][i] = char(i & 0xFF);
		src[1][i] = char(255 - (i & 0xFF));
	}

	olive::PlanarFileDevice dev;
	EXPECT_FALSE(dev.isOpen());

	const char *in[2] = { src[0].constData(), src[1].constData() };
	ASSERT_TRUE(dev.open(names, QIODevice::WriteOnly));
	EXPECT_TRUE(dev.isOpen());
	EXPECT_EQ(dev.write(in, bytes), bytes);
	dev.close();
	EXPECT_FALSE(dev.isOpen());

	// Each channel must have landed in its own file
	for (int ch = 0; ch < 2; ch++) {
		QFile f(names[ch]);
		ASSERT_TRUE(f.open(QIODevice::ReadOnly));
		EXPECT_EQ(f.size(), bytes);
		EXPECT_EQ(f.readAll(), src[ch]);
	}

	// Read back through the device
	QVector<QByteArray> dst(2);
	dst[0].resize(int(bytes));
	dst[1].resize(int(bytes));
	char *out[2] = { dst[0].data(), dst[1].data() };

	ASSERT_TRUE(dev.open(names, QIODevice::ReadOnly));
	EXPECT_EQ(dev.size(), bytes);
	EXPECT_EQ(dev.read(out, bytes), bytes);
	dev.close();

	EXPECT_EQ(dst[0], src[0]);
	EXPECT_EQ(dst[1], src[1]);
}

TEST(CodecPlanarFileDevice, OpenWhileOpenFails)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QVector<QString> names = { dir.filePath(QStringLiteral("ch.bin")) };

	olive::PlanarFileDevice dev;
	ASSERT_TRUE(dev.open(names, QIODevice::WriteOnly));
	EXPECT_FALSE(dev.open(names, QIODevice::WriteOnly));
	EXPECT_TRUE(dev.isOpen());
	dev.close();
	EXPECT_FALSE(dev.isOpen());

	// After closing, the device can be opened again
	ASSERT_TRUE(dev.open(names, QIODevice::ReadOnly));
	dev.close();
}

TEST(CodecPlanarFileDevice, OpenMissingFileForReadFails)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QVector<QString> names = { dir.filePath(QStringLiteral("missing.bin")) };

	olive::PlanarFileDevice dev;
	EXPECT_FALSE(dev.open(names, QIODevice::ReadOnly));
	EXPECT_FALSE(dev.isOpen());
	EXPECT_EQ(dev.size(), 0);
}

TEST(CodecPlanarFileDevice, SeekAndReadWithOffset)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QVector<QString> names = { dir.filePath(QStringLiteral("ch.bin")) };

	const qint64 bytes = 256;
	QByteArray src;
	src.resize(int(bytes));
	for (int i = 0; i < bytes; i++) {
		src[i] = char(i);
	}

	olive::PlanarFileDevice dev;
	const char *in = src.constData();
	ASSERT_TRUE(dev.open(names, QIODevice::WriteOnly));
	EXPECT_EQ(dev.write(&in, bytes), bytes);
	dev.close();

	ASSERT_TRUE(dev.open(names, QIODevice::ReadOnly));

	// seek() moves the file position used by subsequent reads
	QByteArray slice;
	slice.resize(4);
	char *slice_data = slice.data();
	EXPECT_TRUE(dev.seek(100));
	EXPECT_EQ(dev.read(&slice_data, 4), 4);
	EXPECT_EQ(slice, src.mid(100, 4));

	// offset applies to the destination pointer, not the file position
	QByteArray buf;
	buf.resize(16);
	buf.fill('\0');
	char *buf_data = buf.data();
	EXPECT_TRUE(dev.seek(200));
	EXPECT_EQ(dev.read(&buf_data, 4, 8), 4);
	EXPECT_EQ(buf.mid(0, 8), QByteArray(8, '\0'));
	EXPECT_EQ(buf.mid(8, 4), src.mid(200, 4));

	dev.close();
}

TEST(CodecPlanarFileDevice, ClosedDeviceOperationsAreSafe)
{
	olive::PlanarFileDevice dev;

	char *read_buf = nullptr;
	const char *write_buf = nullptr;
	EXPECT_EQ(dev.read(&read_buf, 10), -1);
	EXPECT_EQ(dev.write(&write_buf, 10), -1);
	EXPECT_EQ(dev.size(), 0);

	// Closing a device that was never opened must not crash
	dev.close();
}
