#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>

#include "codec/decoder.h"
#include "codec/ffmpeg/ffmpegdecoder.h"
#include "node/project/footage/footage.h"
#include "node/output/track/track.h"

using namespace olive;

TEST(FFmpegDecoderHW, H264_422_10bit_CPUFrame_IsNotBlack)
{
	const QString path =
		QStringLiteral("/home/mikesolar/Videos/dual_system_video.MOV");

	if (!QFileInfo::exists(path)) {
		GTEST_SKIP() << "Test footage not available: " << path.toStdString();
	}

	DecoderPtr decoder = Decoder::CreateFromID(QStringLiteral("ffmpeg"));
	ASSERT_TRUE(decoder);

	Footage footage(path);
	ASSERT_TRUE(footage.IsValid());

	Decoder::CodecStream stream(path, footage.GetStreamIndex(Track::kVideo, 0),
								nullptr);
	ASSERT_TRUE(decoder->Open(stream));

	Decoder::RetrieveVideoParams params;
	params.time = rational(0);
	params.maximum_format = PixelFormat::U16;
	FramePtr frame = decoder->RetrieveVideoFrame(params);
	ASSERT_TRUE(frame);
	ASSERT_TRUE(frame->is_allocated());

	const int width = frame->width();
	const int height = frame->height();
	ASSERT_GT(width, 0);
	ASSERT_GT(height, 0);

	double avg = 0.0;
	int samples = 0;
	const int bpc = VideoParams::GetBytesPerChannel(frame->format());
	const int stride = frame->linesize_bytes();
	for (int y = 0; y < height && y < 1080; y += 120) {
		for (int x = 0; x < width && x < 1920; x += 240) {
			const uint8_t *p = reinterpret_cast<const uint8_t *>(
				frame->const_data() + y * stride + x * 4 * bpc);
			if (bpc == 1) {
				for (int c = 0; c < 3; ++c) {
					avg += p[c] / 255.0;
				}
			} else {
				const uint16_t *p16 = reinterpret_cast<const uint16_t *>(p);
				for (int c = 0; c < 3; ++c) {
					avg += p16[c] / 65535.0;
				}
			}
			samples += 3;
		}
	}

	const double brightness = samples ? avg / samples : 0.0;
	std::cerr << "Frame size: " << width << "x" << height
			  << " format: " << static_cast<int>(frame->format())
			  << " brightness: " << brightness << std::endl;

	EXPECT_GT(brightness, 0.01);
}
