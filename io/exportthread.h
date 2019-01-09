/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QOffscreenSurface>

class ExportDialog;
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVStream;
struct AVCodec;
struct SwsContext;
struct SwrContext;

extern "C" {
	#include <libavcodec/avcodec.h>
}

#define COMPRESSION_TYPE_CBR 0
#define COMPRESSION_TYPE_CFR 1
#define COMPRESSION_TYPE_TARGETSIZE 2
#define COMPRESSION_TYPE_TARGETBR 3

class ExportThread : public QThread {
	Q_OBJECT
public:
	ExportThread();
	void run();

	// export parameters
	QString filename;
	bool video_enabled;
	int video_codec;
	int video_width;
	int video_height;
	double video_frame_rate;
	int video_compression_type;
	double video_bitrate;
	bool audio_enabled;
	int audio_codec;
	int audio_sampling_rate;
	int audio_bitrate;
	long start_frame;
	long end_frame;

	QOffscreenSurface surface;

	ExportDialog* ed;

	bool continueEncode;
signals:
	void progress_changed(int value, qint64 remaining_ms);
private:
	bool encode(AVFormatContext* ofmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream, bool rescale);
	bool setupVideo();
	bool setupAudio();
	bool setupContainer();

    AVFormatContext* fmt_ctx;
	AVStream* video_stream;
	AVCodec* vcodec;
	AVCodecContext* vcodec_ctx;
	AVFrame* video_frame;
	AVFrame* sws_frame;
    SwsContext* sws_ctx;
	AVStream* audio_stream;
	AVCodec* acodec;
	AVFrame* audio_frame;
	AVFrame* swr_frame;
	AVCodecContext* acodec_ctx;
	AVPacket video_pkt;
	AVPacket audio_pkt;
    SwrContext* swr_ctx;

    bool vpkt_alloc;
    bool apkt_alloc;

	int aframe_bytes;
	int ret;
	char* c_filename;
};

#endif // EXPORTTHREAD_H
