/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "proxygenerator.h"

#include "project/footage.h"
#include "io/path.h"
#include "mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QtMath>
#include <QStatusBar>

#include <QDebug>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
}

enum AVCodecID temp_enc_codec = AV_CODEC_ID_PRORES;

ProxyGenerator::ProxyGenerator() : cancelled(false) {}

void ProxyGenerator::transcode(const ProxyInfo& info) {
	// set progress to 0
	current_progress = 0.0;

	// open input file
	AVFormatContext* input_fmt_ctx = nullptr;
	avformat_open_input(&input_fmt_ctx, info.footage->url.toUtf8(), nullptr, nullptr);

	// open output file
	AVFormatContext* output_fmt_ctx = nullptr;
	avformat_alloc_output_context2(&output_fmt_ctx, nullptr, nullptr, info.path.toUtf8());

	// open output file writing handle
	avio_open(&output_fmt_ctx->pb, info.path.toUtf8(), AVIO_FLAG_WRITE);

	// get stream info from input file
	avformat_find_stream_info(input_fmt_ctx, nullptr);

	// create array of input decoders
	QVector<AVCodecContext*> input_streams;
	input_streams.resize(input_fmt_ctx->nb_streams);
	input_streams.fill(nullptr);

	// create array of output encoders
	QVector<AVCodecContext*> output_streams;
	output_streams.resize(input_fmt_ctx->nb_streams);
	output_streams.fill(nullptr);

	// create array of swscale contexts for pixel format conversion
	QVector<SwsContext*> sws_contexts;
	sws_contexts.resize(input_fmt_ctx->nb_streams);
	sws_contexts.fill(nullptr);

	// loop through file to find compatible video streams
	for (int i=0;i<int(input_fmt_ctx->nb_streams);i++) {
		AVStream* in_stream = input_fmt_ctx->streams[i];

		// create new stream in output
		AVStream* out_stream = avformat_new_stream(output_fmt_ctx, nullptr);
		out_stream->id = in_stream->id;

		// find decoder for this codec
		AVCodec* dec_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);

		// find encoder for chosen proxy type
		AVCodec* enc_codec = avcodec_find_encoder(temp_enc_codec);

		// we only transcode video streams, others we just passthrough
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && dec_codec != nullptr) {

			// allocate decoding context for this stream
			AVCodecContext* dec_ctx = avcodec_alloc_context3(dec_codec);

			// copy parameters from stream to decoding context
			avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);

			// open decoder
			avcodec_open2(dec_ctx, dec_codec, nullptr);

			// store decoding context in array
			input_streams[i] = dec_ctx;

			// retrieve more information about this stream
			av_dump_format(input_fmt_ctx, i, info.footage->url.toUtf8(), 0);

			// allocate encoding context for this stream
			AVCodecContext* enc_ctx = avcodec_alloc_context3(enc_codec);

			// copy properties from decoding context to encoding context
			enc_ctx->codec_id = temp_enc_codec;
			enc_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
			enc_ctx->width = qFloor(dec_ctx->width*info.size_multiplier);
			enc_ctx->height = qFloor(dec_ctx->height*info.size_multiplier);
			enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
			enc_ctx->pix_fmt = enc_codec->pix_fmts[0];
			enc_ctx->framerate = dec_ctx->framerate;
			enc_ctx->time_base = in_stream->time_base;

			out_stream->time_base = in_stream->time_base;

			// if format uses global headers, add flag to enc_ctx
			if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
				enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			// set encoder options (mostly just multithreading)
			AVDictionary* opts = nullptr;
			av_dict_set(&opts, "threads", "auto", 0);

			// open encoder
			avcodec_open2(enc_ctx, enc_codec, &opts);

			// copy parameters from encoding context to stream
			avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);

			// store encoding context in array
			output_streams[i] = enc_ctx;

			// create swscontext for this stream
			SwsContext* sws_ctx = sws_getContext(
											in_stream->codecpar->width,
											in_stream->codecpar->height,
											static_cast<enum AVPixelFormat>(in_stream->codecpar->format),
											enc_ctx->width,
											enc_ctx->height,
											enc_ctx->pix_fmt,
											0,
											nullptr,
											nullptr,
											nullptr
										);

			sws_contexts[i] = sws_ctx;
		} else {
			avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		}
	}

	// write video header
	avformat_write_header(output_fmt_ctx, nullptr);

	// packet that av_read_frame will dump file packets into
	AVPacket packet;
	av_init_packet(&packet);

	// frame that decoder will decode into
	AVFrame* dec_frame = av_frame_alloc();

	// main transcoding loop
	while (!skip) {

		// cache stream index
		int stream_index = packet.stream_index;

		// retrieve frame from decoder (this will clear the last frame so we don't have to do that)
		int read_ret = -1;
		int recfr_ret = -1;
		do {
			// read from input file
			read_ret = av_read_frame(input_fmt_ctx, &packet);

			// handle errors
			if (read_ret < 0) {

				// AVERROR_EOF means we've simply reached the end of the file, otherwise this is an error
				if (read_ret != AVERROR_EOF) {
					qWarning() << "Proxy generation for file" << info.footage->url << "ended prematurely";
				}

				// either way, we shall abort reading
				break;
			}

			stream_index = packet.stream_index;

			// determine whether this frame is from a stream we're transcoding
			if (input_streams.at(stream_index) == nullptr) {
				// if we didn't allocate a decoder for this earlier, we just pass it through

				av_packet_rescale_ts(&packet, input_fmt_ctx->streams[stream_index]->time_base, output_fmt_ctx->streams[stream_index]->time_base);

				// write packet to output
				av_interleaved_write_frame(output_fmt_ctx, &packet);

			} else {
				// we're going to transcode this packet.

				// send packet to decoder
				avcodec_send_packet(input_streams.at(stream_index), &packet);

				// use timestamp and stream duration to create a rough estimation of the progress through this file
				current_progress = qCeil((double(packet.pts)/double(input_fmt_ctx->streams[packet.stream_index]->duration))*100);

			}

			// free packet allocated by av_read_frame
			av_packet_unref(&packet);
		} while ((recfr_ret = avcodec_receive_frame(input_streams.at(packet.stream_index), dec_frame)) == AVERROR(EAGAIN) && !skip);

		// error/eof handling - cancel while loop
		if (read_ret < 0 || skip) {
			break;
		}

		// free packet as we're about to use it for encoding
		av_packet_unref(&packet);

		// rescale input frame timestamp to output timestamp
		av_rescale_q(dec_frame->pts, input_fmt_ctx->streams[stream_index]->time_base, output_fmt_ctx->streams[stream_index]->time_base);

        // determine if the pix_fmt, width, and/or height is different, so if we need to convert
        bool convert_pix_fmt = (output_streams.at(stream_index)->pix_fmt != input_streams.at(stream_index)->pix_fmt
                || output_streams.at(stream_index)->width != input_streams.at(stream_index)->width
                || output_streams.at(stream_index)->height != input_streams.at(stream_index)->height);

		// create reference to the frame to be sent to the encoder
		AVFrame* enc_frame = dec_frame;

		if (convert_pix_fmt) {
			// create sws frame for pixel format conversion
			enc_frame = av_frame_alloc();
			enc_frame->width = output_streams.at(stream_index)->width;
			enc_frame->height = output_streams.at(stream_index)->height;
			enc_frame->format = output_streams.at(stream_index)->pix_fmt;
			av_frame_get_buffer(enc_frame, 0);

			// convert pixel format to format expected by the encoder
			sws_scale(sws_contexts.at(stream_index), dec_frame->data, dec_frame->linesize, 0, dec_frame->height, enc_frame->data, enc_frame->linesize);

			// set same pts as dec_frame
			enc_frame->pts = dec_frame->pts;
		}

		// send frame to encoder
		avcodec_send_frame(output_streams.at(stream_index), enc_frame);

		if (convert_pix_fmt) {
			// free sws frame since we made one before
			av_frame_free(&enc_frame);
		}

		// return value for packet receiving
		int recret;

		// loop through receiving packets
		while ((recret = avcodec_receive_packet(output_streams.at(stream_index), &packet)) >= 0 && !skip) {

			// set packet stream index to current stream index
			packet.stream_index = stream_index;

			// write frame to file
			av_interleaved_write_frame(output_fmt_ctx, &packet);

			// unref old packet
			av_packet_unref(&packet);

		}

	}

	// free dec_frame
	av_frame_free(&dec_frame);

	// write video trailer
	av_write_trailer(output_fmt_ctx);

	// free stream contexts
	for (int i=0;i<int(input_fmt_ctx->nb_streams);i++) {
		if (input_streams[i] != nullptr) {
			// free swscale contexts
			sws_freeContext(sws_contexts[i]);

			// free input decoding context
			avcodec_close(input_streams[i]);
			avcodec_free_context(&input_streams[i]);

			// free output encoding context
			avcodec_close(output_streams[i]);
			avcodec_free_context(&output_streams[i]);
		}
	}

	// close output file handle
	avio_closep(&output_fmt_ctx->pb);

	// close output file
	avformat_free_context(output_fmt_ctx);

	// close input file
	avformat_close_input(&input_fmt_ctx);

	// set footage to use newly generated proxy
	info.footage->proxy = true;
	info.footage->proxy_path = info.path;

	qInfo() << "Finished creating proxy for" << info.footage->url;
    QMetaObject::invokeMethod(Olive::MainWindow->statusBar(),
                              "showMessage",
                              Qt::QueuedConnection,
                              Q_ARG(QString, tr("Finished generating proxy for \"%1\"").arg(info.footage->url)));
}

// main proxy generating loop
void ProxyGenerator::run() {
	// mutex used for thread safe signalling
	mutex.lock();

	while (!cancelled) {
		// wait for queue() to be called
		waitCond.wait(&mutex);

		// quit thread if cancel() was called
		if (cancelled) break;

		// loop through queue until the queue is empty
		while (proxy_queue.size() > 0) {

			// grab proxy info
			const ProxyInfo& info = proxy_queue.first();

			// create directory for info
			QFileInfo(info.path).dir().mkpath(".");

			// set skip to false
			skip = false;

			// transcode proxy
			transcode(info);

			// we're finished with this proxy, remove it
			proxy_queue.removeFirst();

			// quit loop if cancel() was called
			if (cancelled) break;

		}
	}

	mutex.unlock();
}

// called to add footage to generate proxies for
void ProxyGenerator::queue(const ProxyInfo &info) {
	// remove any queued proxies with the same footage
	if (!proxy_queue.isEmpty()
			&& proxy_queue.first().footage == info.footage) {
		// if the thread is currently processing a proxy with the same footage, abort it
		skip = true;
	}

	// scan through the rest of the queue for another proxy with the same footage (start with 1 since we already processed first())
	for (int i=1;i<proxy_queue.size();i++) {
		if (proxy_queue.at(i).footage == info.footage) {
			// found a duplicate, assume the one we're queuing now overrides and delete it
			proxy_queue.removeAt(i);
			i--;
		}
	}

	// add proxy info to queue
	proxy_queue.append(info);

	// wake proxy thread loop if sleeping
	waitCond.wakeAll();
}

// to be called from another thread to terminate the proxy generator thread and free it
void ProxyGenerator::cancel() {
	// signal to thread to cancel
	cancelled = true;
	skip = true;

	// if signal is sleeping, wake it to cancel correctly
	waitCond.wakeAll();

	// wait for thread to finish
	wait();
}

double ProxyGenerator::get_proxy_progress(Footage *f) {
	if (proxy_queue.first().footage == f) {
		return current_progress;
	}
	return 0.0;
}

// proxy generator is a global omnipotent entity
ProxyGenerator proxy_generator;
