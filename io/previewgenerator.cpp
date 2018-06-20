#include "previewgenerator.h"

#include "media.h"

#include <QPainter>
#include <QPixmap>
#include <QDebug>
#include <QtMath>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

PreviewGenerator::PreviewGenerator(QObject* parent) : QThread(parent) {
    media = NULL;
    fmt_ctx = NULL;
}

MediaStream* PreviewGenerator::get_stream_from_file_index(int index) {
    for (int i=0;i<media->video_tracks.size();i++) {
        if (media->video_tracks.at(i)->file_index == index) {
            return media->video_tracks.at(i);
        }
    }
    for (int i=0;i<media->audio_tracks.size();i++) {
        if (media->audio_tracks.at(i)->file_index == index) {
            return media->audio_tracks.at(i);
        }
    }
    return NULL;
}

#define WAVEFORM_RESOLUTION 64

void PreviewGenerator::run() {
    if (media == NULL) {
        qDebug() << "[ERROR] No media was set for preview generation";
    } else {
        if (media->is_sequence) {
            qDebug() << "[ERROR] Cannot run preview generation on a sequence";
        } else {
            SwsContext* sws_ctx;
            SwrContext* swr_ctx;
            AVFrame* temp_frame = av_frame_alloc();
            AVCodecContext** codec_ctx = new AVCodecContext* [fmt_ctx->nb_streams];
            for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
                AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
                codec_ctx[i] = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(codec_ctx[i], fmt_ctx->streams[i]->codecpar);
                avcodec_open2(codec_ctx[i], codec, NULL);

                if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    // initiate qimage
                    MediaStream* ms = get_stream_from_file_index(i);
                    int width = fmt_ctx->streams[i]->duration * av_q2d(fmt_ctx->streams[i]->time_base) * WAVEFORM_RESOLUTION;
                    if (width > 32767) qDebug() << "[WARNING] Due to limitations in Qt's painter, this waveform may not appear correctly (see issue #76)";
                    ms->preview = QImage(width, 80, QImage::Format_RGBA8888);
                    ms->preview_audio_index = 0;
                    ms->preview.fill(Qt::transparent);
                }
            }
            AVPacket packet;
            bool done = true;
            while (av_read_frame(fmt_ctx, &packet) >= 0) {
                MediaStream* s = get_stream_from_file_index(packet.stream_index);
                if (s != NULL && !s->preview_done) {
                    if (avcodec_send_packet(codec_ctx[packet.stream_index], &packet) >= 0) {
                        if (avcodec_receive_frame(codec_ctx[packet.stream_index], temp_frame) >= 0) {
                            if (fmt_ctx->streams[packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                                int dstH = 120;
                                int dstW = dstH * ((float)temp_frame->width/(float)temp_frame->height);
                                uint8_t* data = new uint8_t[dstW*dstH*3];

                                sws_ctx = sws_getContext(
                                            temp_frame->width,
                                            temp_frame->height,
                                            static_cast<AVPixelFormat>(temp_frame->format),
                                            dstW,
                                            dstH,
                                            static_cast<AVPixelFormat>(AV_PIX_FMT_RGB24),
                                            SWS_FAST_BILINEAR,
                                            NULL,
                                            NULL,
                                            NULL
                                        );

                                int linesize[AV_NUM_DATA_POINTERS];
                                linesize[0] = dstW*3;
                                sws_scale(sws_ctx, temp_frame->data, temp_frame->linesize, 0, temp_frame->height, &data, linesize);

                                s->preview = QImage(data, dstW, dstH, linesize[0], QImage::Format_RGB888);

//                                delete [] data;

                                s->preview_done = true;
                                s->preview_lock.unlock();

                                sws_freeContext(sws_ctx);
                            } else if (fmt_ctx->streams[packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                                int interval = qFloor((temp_frame->sample_rate/WAVEFORM_RESOLUTION)/4)*4;
                                swr_ctx = swr_alloc_set_opts(
                                            NULL,
                                            temp_frame->channel_layout,
                                            AV_SAMPLE_FMT_S16,
                                            temp_frame->sample_rate,
                                            temp_frame->channel_layout,
                                            static_cast<AVSampleFormat>(temp_frame->format),
                                            temp_frame->sample_rate,
                                            0,
                                            NULL
                                        );

                                swr_init(swr_ctx);

                                AVFrame* swr_frame = av_frame_alloc();
                                swr_frame->channel_layout = temp_frame->channel_layout;
                                swr_frame->sample_rate = temp_frame->sample_rate;
                                swr_frame->format = AV_SAMPLE_FMT_S16;

                                swr_convert_frame(swr_ctx, swr_frame, temp_frame);

                                int channel_height = s->preview.height()/swr_frame->channels;

                                QPainter p;
                                p.begin(&s->preview);
                                p.setPen(QColor(80, 80, 80));
                                int channel_jump = swr_frame->channels * 2;
                                int divider = (32768.0/(s->preview.height()/2))*swr_frame->channels;
                                for (int i=0;i<swr_frame->nb_samples;i+=interval) {
                                    for (int j=0;j<swr_frame->channels;j++) {
                                        qint16 min = 0;
                                        qint16 max = 0;
                                        for (int k=j*2;k<interval;k+=channel_jump) {
                                            qint16 sample = ((swr_frame->data[0][i+k+1] << 8) | swr_frame->data[0][i+k]);
                                            if (sample > max) {
                                                max = sample;
                                            } else if (sample < min) {
                                                min = sample;
                                            }
                                        }
                                        min /= divider;
                                        max /= divider;

                                        int mid = channel_height*j+(channel_height/2);
                                        p.drawLine(s->preview_audio_index, mid+min, s->preview_audio_index, mid+max);
                                        i += 2;
                                    }
                                    s->preview_audio_index++;
                                }
                                p.end();

                                swr_free(&swr_ctx);
                                av_frame_free(&swr_frame);
                            }
                        } else {
                            qDebug() << "[ERROR] Failed to retrieve frame";
                        }
                    } else {
                        qDebug() << "[ERROR] Failed to send packet";
                    }
                }

                av_packet_unref(&packet);

                // check if we've got all our previews
                if (media->audio_tracks.size() == 0) {
                    done = true;
                    for (int i=0;i<media->video_tracks.size();i++) {
                        if (!media->video_tracks.at(i)->preview_done) {
                            done = false;
                            break;
                        }
                    }
                    if (done) {
                        break;
                    }
                }
            }
            for (int i=0;i<media->audio_tracks.size();i++) {
                media->audio_tracks.at(i)->preview_done = true;
                media->audio_tracks.at(i)->preview_lock.unlock();
            }
            av_frame_free(&temp_frame);
            for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
                avcodec_close(codec_ctx[i]);
            }
            delete [] codec_ctx;
        }
    }

    if (fmt_ctx != NULL) {
        avformat_close_input(&fmt_ctx);
    }
}
