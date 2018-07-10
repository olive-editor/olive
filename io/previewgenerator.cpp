//
// TODO: Fold this file into a multithreaded "file processor"
//

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

#define WAVEFORM_RESOLUTION 64

void PreviewGenerator::run() {
    Q_ASSERT(media != NULL);

    SwsContext* sws_ctx;
    SwrContext* swr_ctx;
    AVFrame* temp_frame = av_frame_alloc();
    AVCodecContext** codec_ctx = new AVCodecContext* [fmt_ctx->nb_streams];
    for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
        codec_ctx[i] = NULL;
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO || fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
            codec_ctx[i] = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(codec_ctx[i], fmt_ctx->streams[i]->codecpar);
            avcodec_open2(codec_ctx[i], codec, NULL);

            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && codec_ctx[i]->channel_layout == 0) {
                codec_ctx[i]->channel_layout = guess_layout_from_channels(fmt_ctx->streams[i]->codecpar->channels);
            }
        }
    }
    AVPacket packet;
    bool done = true;

    bool end_of_file = false;

    // get the ball rolling
    av_read_frame(fmt_ctx, &packet);
    avcodec_send_packet(codec_ctx[packet.stream_index], &packet);

    while (!end_of_file) {
        while (codec_ctx[packet.stream_index] == NULL || avcodec_receive_frame(codec_ctx[packet.stream_index], temp_frame) == AVERROR(EAGAIN)) {
            av_packet_unref(&packet);
            int read_ret = av_read_frame(fmt_ctx, &packet);
            if (read_ret < 0) {
                end_of_file = true;
                if (read_ret != AVERROR_EOF) qDebug() << "[ERROR] Failed to read packet for preview generation";
                break;
            } else if (codec_ctx[packet.stream_index] != NULL) {
                int send_ret = avcodec_send_packet(codec_ctx[packet.stream_index], &packet);
                if (send_ret < 0) {
                    qDebug() << "[ERROR] Failed to send packet for preview generation - aborting";
                    end_of_file = true;
                    break;
                }
            }
        }
        if (!end_of_file) {
            MediaStream* s = media->get_stream_from_file_index(packet.stream_index);
            if (s != NULL && !s->preview_done) {
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

                    s->video_preview = QImage(data, dstW, dstH, linesize[0], QImage::Format_RGB888);

                    //                                delete [] data;

                    s->preview_done = true;

                    sws_freeContext(sws_ctx);

                    avcodec_close(codec_ctx[packet.stream_index]);
                    codec_ctx[packet.stream_index] = NULL;
                } else if (fmt_ctx->streams[packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    int interval = qFloor((temp_frame->sample_rate/WAVEFORM_RESOLUTION)/4)*4;

                    AVFrame* swr_frame = av_frame_alloc();
                    swr_frame->channel_layout = temp_frame->channel_layout;
                    swr_frame->sample_rate = temp_frame->sample_rate;
                    swr_frame->format = AV_SAMPLE_FMT_S16;

                    swr_ctx = swr_alloc_set_opts(
                                NULL,
                                temp_frame->channel_layout,
                                static_cast<AVSampleFormat>(swr_frame->format),
                                temp_frame->sample_rate,
                                temp_frame->channel_layout,
                                static_cast<AVSampleFormat>(temp_frame->format),
                                temp_frame->sample_rate,
                                0,
                                NULL
                                );

                    swr_init(swr_ctx);

                    swr_convert_frame(swr_ctx, swr_frame, temp_frame);

                    int channel_skip = swr_frame->channels * 2;
                    for (int i=0;i<swr_frame->nb_samples;i+=interval) {
                        for (int j=0;j<swr_frame->channels;j++) {
                            qint16 min = 0;
                            qint16 max = 0;
                            for (int k=j*2;k<interval;k+=channel_skip) {
                                qint16 sample = ((swr_frame->data[0][i+k+1] << 8) | swr_frame->data[0][i+k]);
                                if (sample > max) {
                                    max = sample;
                                } else if (sample < min) {
                                    min = sample;
                                }
                            }
                            min /= 256;
                            max /= 256;
                            s->audio_preview.append(min);
                            s->audio_preview.append(max);
                        }
                    }

                    swr_free(&swr_ctx);
                    av_frame_free(&swr_frame);
                }

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
                        end_of_file = true;
                        break;
                    }
                }
            }
            av_packet_unref(&packet);
        }
    }
    for (int i=0;i<media->audio_tracks.size();i++) {
        media->audio_tracks.at(i)->preview_done = true;
    }
    av_frame_free(&temp_frame);
    for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
        if (codec_ctx[i] != NULL) {
            avcodec_close(codec_ctx[i]);
        }
    }
    delete [] codec_ctx;

    avformat_close_input(&fmt_ctx);
}
