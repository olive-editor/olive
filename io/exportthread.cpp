#include "exportthread.h"

#include "project/sequence.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "dialogs/exportdialog.h"
#include "debug.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavutil/opt.h>
	#include <libswresample/swresample.h>
	#include <libswscale/swscale.h>
}

#include <QApplication>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QPainter>

ExportThread::ExportThread() : continueEncode(true) {
	surface.create();

	fmt_ctx = NULL;
	video_stream = NULL;
	vcodec = NULL;
	vcodec_ctx = NULL;
	video_frame = NULL;
	sws_frame = NULL;
	sws_ctx = NULL;
	audio_stream = NULL;
	acodec = NULL;
	audio_frame = NULL;
	swr_frame = NULL;
	acodec_ctx = NULL;
	swr_ctx = NULL;

	vpkt_alloc = false;
	apkt_alloc = false;
}

bool ExportThread::encode(AVFormatContext* ofmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream, bool rescale) {
	ret = avcodec_send_frame(codec_ctx, frame);
	if (ret < 0) {
		qCritical() << "Failed to send frame to encoder." << ret;
        ed->export_error = tr("failed to send frame to encoder (%1)").arg(QString::number(ret));
		return false;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(codec_ctx, packet);
		if (ret == AVERROR(EAGAIN)) {
			return true;
		} else if (ret < 0) {
			if (ret != AVERROR_EOF) {
				qCritical() << "Failed to receive packet from encoder." << ret;
                ed->export_error = tr("failed to receive packet from encoder (%1)").arg(QString::number(ret));
			}
			return false;
		}

		packet->stream_index = stream->index;
		if (rescale) av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);
		av_interleaved_write_frame(ofmt_ctx, packet);
		av_packet_unref(packet);
	}
	return true;
}

bool ExportThread::setupVideo() {
	// if video is disabled, no setup necessary
	if (!video_enabled) return true;

	// find video encoder
	vcodec = avcodec_find_encoder((enum AVCodecID) video_codec);
	if (!vcodec) {
		qCritical() << "Could not find video encoder";
        ed->export_error = tr("could not video encoder for %1").arg(QString::number(video_codec));
		return false;
	}

	// create video stream
	video_stream = avformat_new_stream(fmt_ctx, vcodec);
	video_stream->id = 0;
	if (!video_stream) {
		qCritical() << "Could not allocate video stream";
        ed->export_error = tr("could not allocate video stream");
		return false;
	}

	// allocate context
//	vcodec_ctx = video_stream->codec;
	vcodec_ctx = avcodec_alloc_context3(vcodec);
	if (!vcodec_ctx) {
		qCritical() << "Could not allocate video encoding context";
        ed->export_error = tr("could not allocate video encoding context");
		return false;
	}

	// setup context
	vcodec_ctx->codec_id = static_cast<AVCodecID>(video_codec);
	vcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	vcodec_ctx->width = video_width;
	vcodec_ctx->height = video_height;
	vcodec_ctx->sample_aspect_ratio = {1, 1};
	vcodec_ctx->pix_fmt = vcodec->pix_fmts[0]; // maybe be breakable code
	vcodec_ctx->framerate = av_d2q(video_frame_rate, INT_MAX);
	if (video_compression_type == COMPRESSION_TYPE_CBR) vcodec_ctx->bit_rate = video_bitrate * 1000000;
	vcodec_ctx->time_base = av_inv_q(vcodec_ctx->framerate);
	video_stream->time_base = vcodec_ctx->time_base;

	if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
		vcodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	if (vcodec_ctx->codec_id == AV_CODEC_ID_H264) {
		/*char buffer[50];
		itoa(vcodec_ctx, buffer, 10);*/

		//av_opt_set(vcodec_ctx->priv_data, "preset", "fast", AV_OPT_SEARCH_CHILDREN);
		//av_opt_set(vcodec_ctx->priv_data, "x264opts", "opencl", AV_OPT_SEARCH_CHILDREN);

		switch (video_compression_type) {
		case COMPRESSION_TYPE_CFR:
			av_opt_set(vcodec_ctx->priv_data, "crf", QString::number(static_cast<int>(video_bitrate)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
			break;
		}
	}

	ret = avcodec_open2(vcodec_ctx, vcodec, NULL);
	if (ret < 0) {
		qCritical() << "Could not open output video encoder." << ret;
        ed->export_error = tr("could not open output video encoder (%1)").arg(QString::number(ret));
		return false;
	}

	// copy video encoder parameters to output stream
	ret = avcodec_parameters_from_context(video_stream->codecpar, vcodec_ctx);
	if (ret < 0) {
		qCritical() << "Could not copy video encoder parameters to output stream." << ret;
        ed->export_error = tr("could not copy video encoder parameters to output stream (%1)").arg(QString::number(ret));
		return false;
	}

	// create AVFrame
	video_frame = av_frame_alloc();
	av_frame_make_writable(video_frame);
	video_frame->format = AV_PIX_FMT_RGBA;
	video_frame->width = sequence->width;
	video_frame->height = sequence->height;
	av_frame_get_buffer(video_frame, 0);

	av_init_packet(&video_pkt);

	sws_ctx = sws_getContext(
				sequence->width,
				sequence->height,
				AV_PIX_FMT_RGBA,
				video_width,
				video_height,
				vcodec_ctx->pix_fmt,
				SWS_FAST_BILINEAR,
				NULL,
				NULL,
				NULL
			);

	sws_frame = av_frame_alloc();
	sws_frame->format = vcodec_ctx->pix_fmt;
	sws_frame->width = video_width;
	sws_frame->height = video_height;
	av_frame_get_buffer(sws_frame, 0);

	return true;
}

bool ExportThread::setupAudio() {
	// if audio is disabled, no setup necessary
	if (!audio_enabled) return true;

	// find encoder
	acodec = avcodec_find_encoder(static_cast<AVCodecID>(audio_codec));
	if (!acodec) {
		qCritical() << "Could not find audio encoder";
        ed->export_error = tr("could not audio encoder for %1").arg(QString::number(audio_codec));
		return false;
	}

	// allocate audio stream
	audio_stream = avformat_new_stream(fmt_ctx, acodec);
	audio_stream->id = 1;
	if (!audio_stream) {
		qCritical() << "Could not allocate audio stream";
        ed->export_error = tr("could not allocate audio stream");
		return false;
	}

	// allocate context
//	acodec_ctx = audio_stream->codec;
	acodec_ctx = avcodec_alloc_context3(acodec);
	if (!acodec_ctx) {
		qCritical() << "Could not find allocate audio encoding context";
        ed->export_error = tr("could not allocate audio encoding context");
		return false;
	}

	// setup context
	acodec_ctx->codec_id = static_cast<AVCodecID>(audio_codec);
	acodec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	acodec_ctx->sample_rate = audio_sampling_rate;
	acodec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;  // change this to support surround/mono sound in the future (this is what the user sets the output audio to)
	acodec_ctx->channels = av_get_channel_layout_nb_channels(acodec_ctx->channel_layout);
	acodec_ctx->sample_fmt = acodec->sample_fmts[0];
	acodec_ctx->bit_rate = audio_bitrate * 1000;

	acodec_ctx->time_base.num = 1;
	acodec_ctx->time_base.den = audio_sampling_rate;
	audio_stream->time_base = acodec_ctx->time_base;

	if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
		acodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	// open encoder
	ret = avcodec_open2(acodec_ctx, acodec, NULL);
	if (ret < 0) {
		qCritical() << "Could not open output audio encoder." << ret;
        ed->export_error = tr("could not open output audio encoder (%1)").arg(QString::number(ret));
		return false;
	}

	// copy params to output stream
	ret = avcodec_parameters_from_context(audio_stream->codecpar, acodec_ctx);
	if (ret < 0) {
		qCritical() << "Could not copy audio encoder parameters to output stream." << ret;
        ed->export_error = tr("could not copy audio encoder parameters to output stream (%1)").arg(QString::number(ret));
		return false;
	}

	// init audio resampler context
	swr_ctx = swr_alloc_set_opts(
			NULL,
			acodec_ctx->channel_layout,
			acodec_ctx->sample_fmt,
			acodec_ctx->sample_rate,
			sequence->audio_layout,
			AV_SAMPLE_FMT_S16,
			sequence->audio_frequency,
			0,
			NULL
		);
	swr_init(swr_ctx);

	// initialize raw audio frame
	audio_frame = av_frame_alloc();
	audio_frame->sample_rate = sequence->audio_frequency;
	audio_frame->nb_samples = acodec_ctx->frame_size;
	if (audio_frame->nb_samples == 0) audio_frame->nb_samples = 256; // should possibly be smaller?
	audio_frame->format = AV_SAMPLE_FMT_S16;
	audio_frame->channel_layout = AV_CH_LAYOUT_STEREO; // change this to support surround/mono sound in the future (this is whatever format they're held in the internal buffer)
	audio_frame->channels = av_get_channel_layout_nb_channels(audio_frame->channel_layout);
	av_frame_make_writable(audio_frame);
	ret = av_frame_get_buffer(audio_frame, 0);
	if (ret < 0) {
		qCritical() << "Could not allocate audio buffer." << ret;
        ed->export_error = tr("could not allocate audio buffer (%1)").arg(QString::number(ret));
		return false;
	}
	aframe_bytes = av_samples_get_buffer_size(NULL, audio_frame->channels, audio_frame->nb_samples, static_cast<AVSampleFormat>(audio_frame->format), 0);

	// init converted audio frame
	swr_frame = av_frame_alloc();
	swr_frame->channel_layout = acodec_ctx->channel_layout;
	swr_frame->channels = acodec_ctx->channels;
	swr_frame->sample_rate = acodec_ctx->sample_rate;
	swr_frame->format = acodec_ctx->sample_fmt;
	av_frame_make_writable(swr_frame);

	av_init_packet(&audio_pkt);

	return true;
}

bool ExportThread::setupContainer() {
	avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, c_filename);
	if (!fmt_ctx) {
		qCritical() << "Could not create output context";
        ed->export_error = tr("could not create output format context");
		return false;
	}

	//av_dump_format(fmt_ctx, 0, c_filename, 1);

	ret = avio_open(&fmt_ctx->pb, c_filename, AVIO_FLAG_WRITE);
	if (ret < 0) {
		qCritical() << "Could not open output file." << ret;
        ed->export_error = tr("could not open output file (%1)").arg(QString::number(ret));
		return false;
	}

	return true;
}

void ExportThread::run() {
	panel_sequence_viewer->pause();

	if (!panel_sequence_viewer->viewer_widget->context()->makeCurrent(&surface)) {
		qCritical() << "Make current failed";
        ed->export_error = tr("could not make OpenGL context current");
		return;
	}

	// copy filename
	QByteArray ba = filename.toUtf8();
	c_filename = new char[ba.size()+1];
	strcpy(c_filename, ba.data());

	continueEncode = setupContainer();

	if (video_enabled && continueEncode) continueEncode = setupVideo();

	if (audio_enabled && continueEncode) continueEncode = setupAudio();

	if (continueEncode) {
		ret = avformat_write_header(fmt_ctx, NULL);
		if (ret < 0) {
			qCritical() << "Could not write output file header." << ret;
            ed->export_error = tr("could not write output file header (%1)").arg(QString::number(ret));
			continueEncode = false;
		}
	}

	panel_sequence_viewer->seek(start_frame);
	panel_sequence_viewer->reset_all_audio();

	QOpenGLFramebufferObject fbo(sequence->width, sequence->height, QOpenGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_RECTANGLE);
	fbo.bind();

	panel_sequence_viewer->viewer_widget->default_fbo = &fbo;

	long file_audio_samples = 0;
	qint64 start_time, frame_time, avg_time, eta, total_time = 0;
	long remaining_frames, frame_count = 1;

	while (sequence->playhead <= end_frame && continueEncode) {
		start_time = QDateTime::currentMSecsSinceEpoch();

		panel_sequence_viewer->viewer_widget->paintGL();

		double timecode_secs = (double) (sequence->playhead-start_frame) / sequence->frame_rate;
		if (video_enabled) {
			// get image from opengl
			glReadPixels(0, 0, video_frame->linesize[0]/4, sequence->height, GL_RGBA, GL_UNSIGNED_BYTE, video_frame->data[0]);

			// change pixel format
			sws_scale(sws_ctx, video_frame->data, video_frame->linesize, 0, video_frame->height, sws_frame->data, sws_frame->linesize);
			sws_frame->pts = qRound(timecode_secs/av_q2d(video_stream->time_base));

			// send to encoder
			if (!encode(fmt_ctx, vcodec_ctx, sws_frame, &video_pkt, video_stream, false)) continueEncode = false;
		}
		if (audio_enabled) {
			// do we need to encode more audio samples?
			while (continueEncode && file_audio_samples <= (timecode_secs*audio_sampling_rate)) {
				int adjusted_read = audio_ibuffer_read%audio_ibuffer_size;
				int copylen = qMin(aframe_bytes, audio_ibuffer_size-adjusted_read);
				memcpy(audio_frame->data[0], audio_ibuffer+adjusted_read, copylen);
				memset(audio_ibuffer+adjusted_read, 0, copylen);
				audio_ibuffer_read += copylen;

				if (copylen < aframe_bytes) {
					// copy remainder
					int remainder_len = aframe_bytes-copylen;
					memcpy(audio_frame->data[0]+copylen, audio_ibuffer, remainder_len);
					memset(audio_ibuffer, 0, remainder_len);
					audio_ibuffer_read += remainder_len;
				}

				// convert to export sample format
				swr_convert_frame(swr_ctx, swr_frame, audio_frame);
				swr_frame->pts = file_audio_samples;

				// send to encoder
				if (!encode(fmt_ctx, acodec_ctx, swr_frame, &audio_pkt, audio_stream, true)) continueEncode = false;

				file_audio_samples += swr_frame->nb_samples;
			}
		}

		// encoding stats
		frame_time = (QDateTime::currentMSecsSinceEpoch()-start_time);
		total_time += frame_time;
		remaining_frames = (end_frame-sequence->playhead);
		avg_time = (total_time/frame_count);
		eta = (remaining_frames*avg_time);

//        qInfo() << "Encoded frame" << sequence->playhead << "- took" << frame_time << "ms (avg:" << avg_time << "ms, remaining:" << remaining_frames << ", ETA:" << eta << ")";

		emit progress_changed(qRound(((double) (sequence->playhead-start_frame) / (double) (end_frame-start_frame)) * 100), eta);
		sequence->playhead++;
		frame_count++;
	}

	if (continueEncode) {
		if (video_enabled) vpkt_alloc = true;
		if (audio_enabled) apkt_alloc = true;
	}

	panel_sequence_viewer->viewer_widget->default_fbo = NULL;
	rendering = false;

	fbo.release();

	if (audio_enabled && continueEncode) {
		// flush swresample
		do {
			swr_convert_frame(swr_ctx, swr_frame, NULL);
			if (swr_frame->nb_samples == 0) break;
			swr_frame->pts = file_audio_samples;
			if (!encode(fmt_ctx, acodec_ctx, swr_frame, &audio_pkt, audio_stream, true)) continueEncode = false;
			file_audio_samples += swr_frame->nb_samples;
		} while (swr_frame->nb_samples > 0);
	}

	bool continueVideo = true;
	bool continueAudio = true;
	if (continueEncode) {
		// flush remaining packets
		while (continueVideo && continueAudio) {
			if (continueVideo && video_enabled) continueVideo = encode(fmt_ctx, vcodec_ctx, NULL, &video_pkt, video_stream, false);
			if (continueAudio && audio_enabled) continueAudio = encode(fmt_ctx, acodec_ctx, NULL, &audio_pkt, audio_stream, true);
		}

		ret = av_write_trailer(fmt_ctx);
		if (ret < 0) {
			qCritical() << "Could not write output file trailer." << ret;
            ed->export_error = tr("could not write output file trailer (%1)").arg(QString::number(ret));
			continueEncode = false;
		}

		emit progress_changed(100, 0);
	}

	avio_closep(&fmt_ctx->pb);

	if (vpkt_alloc) av_packet_unref(&video_pkt);
	if (video_frame != NULL) av_frame_free(&video_frame);
	if (vcodec_ctx != NULL) {
		avcodec_close(vcodec_ctx);
		avcodec_free_context(&vcodec_ctx);
	}

	if (apkt_alloc) av_packet_unref(&audio_pkt);
	if (audio_frame != NULL) av_frame_free(&audio_frame);
	if (acodec_ctx != NULL) {
		avcodec_close(acodec_ctx);
		avcodec_free_context(&acodec_ctx);
	}

	avformat_free_context(fmt_ctx);

	if (sws_ctx != NULL) {
		sws_freeContext(sws_ctx);
		av_frame_free(&sws_frame);
	}
	if (swr_ctx != NULL) {
		swr_free(&swr_ctx);
		av_frame_free(&swr_frame);
	}

	delete [] c_filename;

	panel_sequence_viewer->viewer_widget->context()->doneCurrent();
	panel_sequence_viewer->viewer_widget->context()->moveToThread(qApp->thread());

	rendering = false;
}
