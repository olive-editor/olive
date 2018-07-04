#include "exportthread.h"

#include "project/sequence.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "playback/playback.h"
#include "playback/audio.h"
#include "dialogs/exportdialog.h"

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswresample/swresample.h>
	#include <libswscale/swscale.h>
}

#include <QDebug>
#include <QApplication>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QPainter>

bool encode(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVPacket* packet, AVStream* stream) {
	int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0/* && ret != AVERROR(EAGAIN) && frame != NULL*/) {
		qDebug() << "[ERROR] Failed to send frame to encoder." << ret;
		return false;
	} else {
		while (ret >= 0) {
			ret = avcodec_receive_packet(codec_ctx, packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				// do nothing, encoder needs more input
			} else if (ret < 0) {
				qDebug() << "[ERROR] Failed to receive packet from encoder." << ret;
				return false;
			} else {
				packet->stream_index = stream->index;
				av_interleaved_write_frame(fmt_ctx, packet);
			}
		}
	}
	return true;
}

void ExportThread::run() {
    panel_timeline->pause();
//    av_log_set_level(AV_LOG_DEBUG);

	// TODO make customizable
	long start = 0;
	long end = sequence->getEndFrame();

    if (!panel_viewer->viewer_widget->context()->makeCurrent(&surface)) {
        qDebug() << "[ERROR] Make current failed";
        return;
	}

	AVFormatContext* fmt_ctx = NULL;
	QByteArray ba = filename.toLatin1();
	char* c_filename = new char[ba.size()+1];
	strcpy(c_filename, ba.data());

    AVOutputFormat* ofmt = av_guess_format(NULL, c_filename, NULL);
    avformat_alloc_output_context2(&fmt_ctx, ofmt, NULL, c_filename);
	if (!fmt_ctx) {
		qDebug() << "[ERROR] Could not create output context";
    } else {
		AVStream* video_stream;
		AVCodec* vcodec;
		AVCodecContext* vcodec_ctx;
		AVFrame* video_frame;
        AVFrame* sws_frame;
        SwsContext* sws_ctx = NULL;
        AVStream* audio_stream;
        AVCodec* acodec;
        AVFrame* audio_frame;
        AVFrame* swr_frame;
        AVCodecContext* acodec_ctx;
		AVPacket video_pkt;
        AVPacket audio_pkt;
        SwrContext* swr_ctx = NULL;
        int aframe_bytes;
		int ret;

        fail = false;

        if (video_enabled) {
            vcodec = avcodec_find_encoder((enum AVCodecID) video_codec);
            if (!vcodec) {
                qDebug() << "[ERROR] Could not find video encoder";
                fail = true;
            } else {
                video_stream = avformat_new_stream(fmt_ctx, vcodec);
                video_stream->id = 0;

                if (!video_stream) {
                    qDebug() << "[ERROR] Could not allocate output streams";
                    fail = true;
                } else {
					vcodec_ctx = avcodec_alloc_context3(vcodec);

					if (!vcodec_ctx) {
						qDebug() << "[ERROR] Could not find allocate video encoding context";
						fail = true;
					} else {
                        vcodec_ctx->codec_id = (enum AVCodecID) video_codec;
                        vcodec_ctx->width = video_width;
                        vcodec_ctx->height = video_height;
                        vcodec_ctx->sample_aspect_ratio = av_d2q(video_width/video_height, INT_MAX);
						vcodec_ctx->pix_fmt = vcodec->pix_fmts[0]; // maybe be breakable code
                        vcodec_ctx->framerate = av_d2q(video_frame_rate, INT_MAX);
						vcodec_ctx->bit_rate = video_bitrate * 1000000;
						vcodec_ctx->time_base = av_inv_q(vcodec_ctx->framerate);

                        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
                            vcodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                        }

                        vcodec_ctx->gop_size = 12; // ? does this help?

						ret = avcodec_open2(vcodec_ctx, vcodec, NULL);
						if (ret < 0) {
							qDebug() << "[ERROR] Could not open output video encoder." << ret;
							fail = true;
						} else {
							ret = avcodec_parameters_from_context(video_stream->codecpar, vcodec_ctx);
							if (ret < 0) {
								qDebug() << "[ERROR] Could not copy video encoder parameters to output stream." << ret;
								fail = true;
							} else {
								// initialize raw video frame
								video_frame = av_frame_alloc();
								av_frame_make_writable(video_frame);
								video_frame->format = AV_PIX_FMT_RGBA;
								video_frame->width = video_width;
								video_frame->height = video_height;
								av_frame_get_buffer(video_frame, 0);

								av_init_packet(&video_pkt);

								sws_ctx = sws_getContext(
											video_width,
											video_height,
                                            AV_PIX_FMT_RGBA,
											video_width,
											video_height,
											AV_PIX_FMT_YUV420P,
											SWS_FAST_BILINEAR,
											NULL,
											NULL,
											NULL
										);

                                sws_frame = av_frame_alloc();
                                sws_frame->format = AV_PIX_FMT_YUV420P;
                                sws_frame->width = video_frame->width;
                                sws_frame->height = video_frame->height;
                                av_frame_get_buffer(sws_frame, 0);
							}
						}
					}
				}
			}
		}

        if (audio_enabled && !fail) {
            acodec = avcodec_find_encoder((enum AVCodecID) audio_codec);
            if (!acodec) {
                qDebug() << "[ERROR] Could not find audio encoder";
                fail = true;
            } else {
                audio_stream = avformat_new_stream(fmt_ctx, acodec);
                audio_stream->id = 1;
                if (!audio_stream) {
                    qDebug() << "[ERROR] Could not allocate output streams";
                    fail = true;
                } else {
                    acodec_ctx = avcodec_alloc_context3(acodec);
                    if (!acodec_ctx) {
                        qDebug() << "[ERROR] Could not find allocate audio encoding context";
                        fail = true;
                    } else {
                        acodec_ctx->sample_rate = audio_sampling_rate;
                        acodec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;  // change this to support surround/mono sound in the future (this is what the user sets the output audio to)
                        acodec_ctx->channels = av_get_channel_layout_nb_channels(acodec_ctx->channel_layout);
                        acodec_ctx->sample_fmt = acodec->sample_fmts[0];
                        acodec_ctx->bit_rate = audio_bitrate * 1000;
                        acodec_ctx->time_base = {1, audio_sampling_rate};

                        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
                            acodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                        }

                        ret = avcodec_open2(acodec_ctx, acodec, NULL);
                        if (ret < 0) {
                            qDebug() << "[ERROR] Could not open output audio encoder." << ret;
                            fail = true;
                        } else {
                            ret = avcodec_parameters_from_context(audio_stream->codecpar, acodec_ctx);
                            if (ret < 0) {
                                qDebug() << "[ERROR] Could not copy audio encoder parameters to output stream." << ret;
                                fail = true;
                            } else {
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
                                if (audio_frame->nb_samples == 0) audio_frame->nb_samples = 2048; // should possibly be smaller?
                                audio_frame->format = AV_SAMPLE_FMT_S16;
                                audio_frame->channel_layout = AV_CH_LAYOUT_STEREO; // change this to support surround/mono sound in the future (this is whatever format they're held in the internal buffer)
                                audio_frame->channels = av_get_channel_layout_nb_channels(audio_frame->channel_layout);
                                ret = av_frame_get_buffer(audio_frame, 0);
                                if (ret < 0) {
                                    qDebug() << "[ERROR] Could not allocate audio buffer." << ret;
                                }
                                // av_samples_alloc(&audio_frame->data[0], NULL, av_get_channel_layout_nb_channels(audio_frame->channel_layout), audio_frame->nb_samples, acodec_ctx->sample_fmt, 0);
                                av_frame_make_writable(audio_frame);
                                aframe_bytes = av_samples_get_buffer_size(NULL, audio_frame->channels, audio_frame->nb_samples, static_cast<AVSampleFormat>(audio_frame->format), 0);

                                // init converted audio frame
                                swr_frame = av_frame_alloc();
                                swr_frame->channel_layout = acodec_ctx->channel_layout;
                                swr_frame->channels = acodec_ctx->channels;
                                swr_frame->sample_rate = acodec_ctx->sample_rate;
                                swr_frame->format = acodec_ctx->sample_fmt;
                                av_frame_make_writable(swr_frame);

                                av_init_packet(&audio_pkt);
                            }
                        }
                    }
                }
            }
        }

        if (!fail) {
			av_dump_format(fmt_ctx, 0, c_filename, 1);

            ret = avio_open(&fmt_ctx->pb, c_filename, AVIO_FLAG_WRITE);
			if (ret < 0) {
				qDebug() << "[ERROR] Could not open output file." << ret;
			} else {
				ret = avformat_write_header(fmt_ctx, NULL);
				if (ret < 0) {
					qDebug() << "[ERROR] Could not write output file header." << ret;
				} else {
                    panel_timeline->seek(start);

					QOpenGLFramebufferObject fbo(video_width, video_height, QOpenGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_RECTANGLE);
					fbo.bind();
					QOpenGLPaintDevice fbo_dev(video_width, video_height);
					QPainter painter(&fbo_dev);
                    painter.beginNativePainting();

                    panel_viewer->viewer_widget->initializeGL();
                    panel_viewer->viewer_widget->flip = true;

                    long file_audio_samples = 0;

					while (panel_timeline->playhead < end && !fail) {
                        panel_viewer->viewer_widget->paintGL();

						double timecode_secs = (double) panel_timeline->playhead / sequence->frame_rate;
						if (video_enabled) {
                            // get image from opengl
                            glReadPixels(0, 0, video_width, video_height, GL_RGBA, GL_UNSIGNED_BYTE, video_frame->data[0]);

                            // change pixel format
                            sws_scale(sws_ctx, video_frame->data, video_frame->linesize, 0, video_frame->height, sws_frame->data, sws_frame->linesize);
							sws_frame->pts = round(timecode_secs/av_q2d(video_stream->time_base));

							// send to encoder
							if (!encode(fmt_ctx, vcodec_ctx, sws_frame, &video_pkt, video_stream)) fail = true;
						}
                        if (audio_enabled && !fail) {
                            // do we need to encode more audio samples?
                            while (file_audio_samples <= (timecode_secs*audio_sampling_rate)) {
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
                                if (!encode(fmt_ctx, acodec_ctx, swr_frame, &audio_pkt, audio_stream)) fail = true;

                                file_audio_samples += swr_frame->nb_samples;
                            }
                        }
						emit progress_changed(((float) panel_timeline->playhead / (float) end) * 100);
						panel_timeline->playhead++;
					}

                    panel_viewer->viewer_widget->flip = false;

					painter.endNativePainting();
					fbo.release();

                    if (!fail) {
                        // flush remaining packets
                        if (video_enabled) {
                            while (encode(fmt_ctx, vcodec_ctx, NULL, &video_pkt, video_stream)) {}
                        }
                        if (audio_enabled) {
                            // flush swresample
                            do {
                                swr_convert_frame(swr_ctx, swr_frame, NULL);
                                swr_frame->pts = file_audio_samples;
                                if (!encode(fmt_ctx, acodec_ctx, swr_frame, &audio_pkt, audio_stream)) fail = true;
                                file_audio_samples += swr_frame->nb_samples;
                            } while (swr_frame->nb_samples > 0);

                            // flush encoder
                            while (encode(fmt_ctx, acodec_ctx, NULL, &audio_pkt, audio_stream)) {}
                        }
                    }

                    if (!fail) {
						ret = av_write_trailer(fmt_ctx);
						if (ret < 0) {
							qDebug() << "[ERROR] Could not write output file trailer." << ret;
						}

						emit progress_changed(100);
					}
				}
			}

            if (video_enabled) {
                avcodec_close(vcodec_ctx);
                av_packet_unref(&video_pkt);
                av_frame_free(&video_frame);
                avcodec_free_context(&vcodec_ctx);
            }

            if (audio_enabled) {
                avcodec_close(acodec_ctx);
                av_packet_unref(&audio_pkt);
                av_frame_free(&audio_frame);
                avcodec_free_context(&acodec_ctx);
            }

			avio_closep(&fmt_ctx->pb);
			avformat_free_context(fmt_ctx);

		//	qDebug() << "Render took: %ih %im %is\n", render_time/3600000, render_time/60000, render_time/1000;

            if (sws_ctx != NULL) {
                sws_freeContext(sws_ctx);
                av_frame_free(&sws_frame);
            }
            if (swr_ctx != NULL) {
                swr_free(&swr_ctx);
                av_frame_free(&swr_frame);
            }
		}
	}

	delete [] c_filename;

	panel_viewer->viewer_widget->context()->doneCurrent();
	panel_viewer->viewer_widget->context()->moveToThread(qApp->thread());
	panel_viewer->viewer_widget->multithreaded = true;
    panel_viewer->viewer_widget->force_audio = false;
    panel_viewer->viewer_widget->enable_paint = true;
}
