#include "previewgenerator.h"

#include "project/media.h"
#include "project/footage.h"
#include "panels/viewer.h"
#include "panels/project.h"
#include "io/config.h"
#include "io/path.h"
#include "debug.h"

#include <QPainter>
#include <QPixmap>
#include <QtMath>
#include <QTreeWidgetItem>
#include <QSemaphore>
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QDateTime>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

QSemaphore sem(5); // only 5 preview generators can run at one time

PreviewGenerator::PreviewGenerator(Media* i, Footage* m, bool r) :
	QThread(nullptr),
	fmt_ctx(nullptr),
	media(i),
	footage(m),
	retrieve_duration(false),
	contains_still_image(false),
	replace(r),
	cancelled(false)
{
	data_path = get_data_path() + "/previews";
	QDir data_dir(data_path);
	if (!data_dir.exists()) {
		data_dir.mkpath(".");
	}

	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void PreviewGenerator::parse_media() {
	// detect video/audio streams in file
	for (int i=0;i<int(fmt_ctx->nb_streams);i++) {
		// Find the decoder for the video stream
		if (avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id) == nullptr) {
			qCritical() << "Unsupported codec in stream" << i << "of file" << footage->name;
		} else {
			FootageStream ms;
			ms.preview_done = false;
			ms.file_index = i;
			ms.enabled = true;
			ms.infinite_length = false;

			bool append = false;

			if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
					&& fmt_ctx->streams[i]->codecpar->width > 0
					&& fmt_ctx->streams[i]->codecpar->height > 0) {

				/*dout << "avg_frame_rate was:" << fmt_ctx->streams[i]->avg_frame_rate.num << "/" << fmt_ctx->streams[i]->avg_frame_rate.den;
				dout << "r_frame_rate was:" << fmt_ctx->streams[i]->r_frame_rate.num << "/" << fmt_ctx->streams[i]->r_frame_rate.den;
				dout << "codec_frame_rate was:" << fmt_ctx->streams[i]->codec->framerate.num << "/" << fmt_ctx->streams[i]->codec->framerate.den;
				dout << "nb_frames was:" << fmt_ctx->streams[i]->nb_frames;
				dout << "duration was:" << fmt_ctx->streams[i]->duration << "OR fmt_ctx's duration is:" << fmt_ctx->duration;*/

				// heuristic to determine if video is a still image
				if (fmt_ctx->streams[i]->avg_frame_rate.den == 0
						&& fmt_ctx->streams[i]->codecpar->codec_id != AV_CODEC_ID_DNXHD) { // silly hack but this is the only scenario i've seen this
					if (footage->url.contains('%')) {
						// must be an image sequence
						ms.video_frame_rate = 25;
					} else {
						ms.infinite_length = true;
						contains_still_image = true;
						ms.video_frame_rate = 0;
					}
				} else {
					// using ffmpeg's built-in heuristic
					ms.video_frame_rate = av_q2d(av_guess_frame_rate(fmt_ctx, fmt_ctx->streams[i], nullptr));

					// old heuristic
					/*if (fmt_ctx->streams[i]->r_frame_rate.den == 0) {
						ms.video_frame_rate = av_q2d(fmt_ctx->streams[i]->avg_frame_rate);
					} else {
						ms.video_frame_rate = av_q2d(fmt_ctx->streams[i]->r_frame_rate);
					}*/
				}

				ms.video_width = fmt_ctx->streams[i]->codecpar->width;
				ms.video_height = fmt_ctx->streams[i]->codecpar->height;

				// default value, we get the true value later in generate_waveform()
				ms.video_auto_interlacing = VIDEO_PROGRESSIVE;
				ms.video_interlacing = VIDEO_PROGRESSIVE;

				append = true;
			} else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
				ms.audio_channels = fmt_ctx->streams[i]->codecpar->channels;
				ms.audio_layout = int(fmt_ctx->streams[i]->codecpar->channel_layout);
				ms.audio_frequency = fmt_ctx->streams[i]->codecpar->sample_rate;

				append = true;
			}

			if (append) {
				QVector<FootageStream>& stream_list = (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) ? footage->audio_tracks : footage->video_tracks;
				for (int j=0;j<stream_list.size();j++) {
					if (stream_list.at(j).file_index == i) {
						stream_list[j] = ms;
						append = false;
					}
				}
				if (append) stream_list.append(ms);
			}
		}
	}
	footage->length = fmt_ctx->duration;

	if (fmt_ctx->duration == INT64_MIN) {
		retrieve_duration = true;
	} else {
		finalize_media();
	}
}

bool PreviewGenerator::retrieve_preview(const QString& hash) {
	// returns true if generate_waveform must be run, false if we got all previews from cached files
	if (retrieve_duration) {
		//dout << "[NOTE] " << media->name << "needs to retrieve duration";
		return true;
	}

	bool found = true;
	for (int i=0;i<footage->video_tracks.size();i++) {
		FootageStream& ms = footage->video_tracks[i];
		QString thumb_path = get_thumbnail_path(hash, ms);
		QFile f(thumb_path);
		if (f.exists() && ms.video_preview.load(thumb_path)) {
			//dout << "loaded thumb" << ms->file_index << "from" << thumb_path;
			ms.make_square_thumb();
			ms.preview_done = true;
		} else {
			found = false;
			break;
		}
	}
	for (int i=0;i<footage->audio_tracks.size();i++) {
		FootageStream& ms = footage->audio_tracks[i];
		QString waveform_path = get_waveform_path(hash, ms);
		QFile f(waveform_path);
		if (f.exists()) {
			//dout << "loaded wave" << ms->file_index << "from" << waveform_path;
			f.open(QFile::ReadOnly);
			QByteArray data = f.readAll();
			ms.audio_preview.resize(data.size());
			for (int j=0;j<data.size();j++) {
				// faster way?
				ms.audio_preview[j] = data.at(j);
			}
			ms.preview_done = true;
			f.close();
		} else {
			found = false;
			break;
		}
	}
	if (!found) {
		for (int i=0;i<footage->video_tracks.size();i++) {
			FootageStream& ms = footage->video_tracks[i];
			ms.preview_done = false;
		}
		for (int i=0;i<footage->audio_tracks.size();i++) {
			FootageStream& ms = footage->audio_tracks[i];
			ms.audio_preview.clear();
			ms.preview_done = false;
		}
	}
	return !found;
}

void PreviewGenerator::finalize_media() {
	footage->ready_lock.unlock();
	footage->ready = true;

	if (!cancelled) {
		if (footage->video_tracks.size() == 0) {
			emit set_icon(ICON_TYPE_AUDIO, replace);
		} else if (contains_still_image) {
			emit set_icon(ICON_TYPE_IMAGE, replace);
		} else {
			emit set_icon(ICON_TYPE_VIDEO, replace);
		}

		/*if (!contains_still_image || media->audio_tracks.size() > 0) {
			double frame_rate = 30;
			if (!contains_still_image && media->video_tracks.size() > 0) frame_rate = media->video_tracks.at(0)->video_frame_rate;
			item->setText(1, frame_to_timecode(media->get_length_in_frames(frame_rate), config.timecode_view, frame_rate));

			if (media->video_tracks.size() > 0) {
				item->setText(2, QString::number(frame_rate) + " FPS");
			} else {
				item->setText(2, QString::number(media->audio_tracks.at(0)->audio_frequency) + " Hz");
			}
		}*/
	}
}

void thumb_data_cleanup(void *info) {
	delete [] static_cast<uint8_t*>(info);
}

void PreviewGenerator::generate_waveform() {
	SwsContext* sws_ctx;
	SwrContext* swr_ctx;
	AVFrame* temp_frame = av_frame_alloc();
	AVCodecContext** codec_ctx = new AVCodecContext* [fmt_ctx->nb_streams];
	int64_t* media_lengths = new int64_t[fmt_ctx->nb_streams]{0};
	for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
		codec_ctx[i] = nullptr;
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO || fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[i]->codecpar->codec_id);
			if (codec != nullptr) {
				codec_ctx[i] = avcodec_alloc_context3(codec);
				avcodec_parameters_to_context(codec_ctx[i], fmt_ctx->streams[i]->codecpar);
				avcodec_open2(codec_ctx[i], codec, nullptr);
				if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && codec_ctx[i]->channel_layout == 0) {
					codec_ctx[i]->channel_layout = av_get_default_channel_layout(fmt_ctx->streams[i]->codecpar->channels);
				}
			}
		}
	}

	AVPacket* packet = av_packet_alloc();
	bool done = true;

	bool end_of_file = false;

	// get the ball rolling
	do {
		av_read_frame(fmt_ctx, packet);
	} while (codec_ctx[packet->stream_index] == nullptr);
	avcodec_send_packet(codec_ctx[packet->stream_index], packet);

	while (!end_of_file) {
		while (codec_ctx[packet->stream_index] == nullptr || avcodec_receive_frame(codec_ctx[packet->stream_index], temp_frame) == AVERROR(EAGAIN)) {
			av_packet_unref(packet);
			int read_ret = av_read_frame(fmt_ctx, packet);

			//dout << "read frame for" << footage->name << footage->url << read_ret << "retrieve_duration:" << retrieve_duration << "eof:" << end_of_file << "packet pts:" << packet->pts;

			if (read_ret < 0) {
				end_of_file = true;
				if (read_ret != AVERROR_EOF) qCritical() << "Failed to read packet for preview generation" << read_ret;
				break;
			}
			if (codec_ctx[packet->stream_index] != nullptr) {
				int send_ret = avcodec_send_packet(codec_ctx[packet->stream_index], packet);
				if (send_ret < 0 && send_ret != AVERROR(EAGAIN)) {
					qCritical() << "Failed to send packet for preview generation - aborting" << send_ret;
					end_of_file = true;
					break;
				}
			}
		}
		if (!end_of_file) {
			FootageStream* s = footage->get_stream_from_file_index(fmt_ctx->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO, packet->stream_index);
			if (s != nullptr) {
				if (fmt_ctx->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					if (!s->preview_done) {
						int dstH = config.thumbnail_resolution;
						int dstW = qRound(dstH * (float(temp_frame->width)/float(temp_frame->height)));
						uint8_t* data = new uint8_t[size_t(dstW*dstH*4)];

						sws_ctx = sws_getContext(
								temp_frame->width,
								temp_frame->height,
								static_cast<AVPixelFormat>(temp_frame->format),
								dstW,
								dstH,
								static_cast<AVPixelFormat>(AV_PIX_FMT_RGBA),
								SWS_FAST_BILINEAR,
								nullptr,
								nullptr,
								nullptr
							);

						int linesize[AV_NUM_DATA_POINTERS];
						linesize[0] = dstW*4;
						sws_scale(sws_ctx, temp_frame->data, temp_frame->linesize, 0, temp_frame->height, &data, linesize);

						s->video_preview = QImage(data, dstW, dstH, linesize[0], QImage::Format_RGBA8888, thumb_data_cleanup);
						s->make_square_thumb();

						// is video interlaced?
						s->video_auto_interlacing = (temp_frame->interlaced_frame) ? ((temp_frame->top_field_first) ? VIDEO_TOP_FIELD_FIRST : VIDEO_BOTTOM_FIELD_FIRST) : VIDEO_PROGRESSIVE;
						s->video_interlacing = s->video_auto_interlacing;

						s->preview_done = true;

						sws_freeContext(sws_ctx);

						if (!retrieve_duration) {
							avcodec_close(codec_ctx[packet->stream_index]);
							codec_ctx[packet->stream_index] = nullptr;
						}
					}
					media_lengths[packet->stream_index]++;
				} else if (fmt_ctx->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
					int interval = qFloor((temp_frame->sample_rate/config.waveform_resolution)/4)*4;

					AVFrame* swr_frame = av_frame_alloc();
					swr_frame->channel_layout = temp_frame->channel_layout;
					swr_frame->sample_rate = temp_frame->sample_rate;
					swr_frame->format = AV_SAMPLE_FMT_S16P;

					swr_ctx = swr_alloc_set_opts(
								nullptr,
								temp_frame->channel_layout,
								static_cast<AVSampleFormat>(swr_frame->format),
								temp_frame->sample_rate,
								temp_frame->channel_layout,
								static_cast<AVSampleFormat>(temp_frame->format),
								temp_frame->sample_rate,
								0,
								nullptr
							);

					swr_init(swr_ctx);

					swr_convert_frame(swr_ctx, swr_frame, temp_frame);

					// TODO implement a way to terminate this if the user suddenly closes the project while the waveform is being generated
					int sample_size = av_get_bytes_per_sample(static_cast<AVSampleFormat>(swr_frame->format));
					int nb_bytes = swr_frame->nb_samples * sample_size;
					int byte_interval = interval * sample_size;
					for (int i=0;i<nb_bytes;i+=byte_interval) {
						for (int j=0;j<swr_frame->channels;j++) {
							qint16 min = 0;
							qint16 max = 0;
							for (int k=0;k<byte_interval;k+=sample_size) {
								if (i+k < nb_bytes) {
									qint16 sample = ((swr_frame->data[j][i+k+1] << 8) | swr_frame->data[j][i+k]);
									if (sample > max) {
										max = sample;
									} else if (sample < min) {
										min = sample;
									}
								} else {
									break;
								}
							}
							s->audio_preview.append(min >> 8);
							s->audio_preview.append(max >> 8);
							if (cancelled) break;
						}
					}

					swr_free(&swr_ctx);
					av_frame_unref(swr_frame);
					av_frame_free(&swr_frame);

					if (cancelled) {
						end_of_file = true;
						break;
					}
				}
			}

			// check if we've got all our previews
			if (retrieve_duration) {
				done = false;
			} else if (footage->audio_tracks.size() == 0) {
				done = true;
				for (int i=0;i<footage->video_tracks.size();i++) {
					if (!footage->video_tracks.at(i).preview_done) {
						done = false;
						break;
					}
				}
				if (done) {
					end_of_file = true;
					break;
				}
			}
			av_packet_unref(packet);
		}
	}
	for (int i=0;i<footage->audio_tracks.size();i++) {
		footage->audio_tracks[i].preview_done = true;
	}
	av_frame_free(&temp_frame);
	av_packet_free(&packet);
	for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
		if (codec_ctx[i] != nullptr) {
			avcodec_close(codec_ctx[i]);
			avcodec_free_context(&codec_ctx[i]);
		}
	}
	if (retrieve_duration) {
		footage->length = 0;
		int maximum_stream = 0;
		for (unsigned int i=0;i<fmt_ctx->nb_streams;i++) {
			if (media_lengths[i] > media_lengths[maximum_stream]) {
				maximum_stream = i;
			}
		}
		footage->length = double(media_lengths[maximum_stream]) / av_q2d(fmt_ctx->streams[maximum_stream]->avg_frame_rate) * AV_TIME_BASE; // TODO redo with PTS
		finalize_media();
	}
	delete [] media_lengths;
	delete [] codec_ctx;
}

QString PreviewGenerator::get_thumbnail_path(const QString& hash, const FootageStream& ms) {
	return data_path + "/" + hash + "t" + QString::number(ms.file_index);
}

QString PreviewGenerator::get_waveform_path(const QString& hash, const FootageStream& ms) {
	return data_path + "/" + hash + "w" + QString::number(ms.file_index);
}

void PreviewGenerator::run() {
	Q_ASSERT(footage != nullptr);
	Q_ASSERT(media != nullptr);

	QByteArray ba = footage->url.toUtf8();
	char* filename = new char[ba.size()+1];
	strcpy(filename, ba.data());

	QString errorStr;
	bool error = false;
	int errCode = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr);
	if(errCode != 0) {
		char err[1024];
		av_strerror(errCode, err, 1024);
		errorStr = tr("Could not open file - %1").arg(err);
		error = true;
	} else {
		errCode = avformat_find_stream_info(fmt_ctx, nullptr);
		if (errCode < 0) {
			char err[1024];
			av_strerror(errCode, err, 1024);
			errorStr = tr("Could not find stream information - %1").arg(err);
			error = true;
		} else {
			av_dump_format(fmt_ctx, 0, filename, 0);
			parse_media();

			// see if we already have data for this
			QFileInfo file_info(footage->url);
			QString cache_file = footage->url.mid(footage->url.lastIndexOf('/')+1) + QString::number(file_info.size()) + QString::number(file_info.lastModified().toMSecsSinceEpoch());
			//dout << "using hash" << cache_file;
			QString hash = QCryptographicHash::hash(cache_file.toUtf8(), QCryptographicHash::Md5).toHex();

			if (retrieve_preview(hash)) {
				sem.acquire();

				generate_waveform();

				// save preview to file
				for (int i=0;i<footage->video_tracks.size();i++) {
					FootageStream& ms = footage->video_tracks[i];
					ms.video_preview.save(get_thumbnail_path(hash, ms), "PNG");
					//dout << "saved" << ms->file_index << "thumbnail to" << get_thumbnail_path(hash, ms);
				}
				for (int i=0;i<footage->audio_tracks.size();i++) {
					FootageStream& ms = footage->audio_tracks[i];
					QFile f(get_waveform_path(hash, ms));
					f.open(QFile::WriteOnly);
					f.write(ms.audio_preview.constData(), ms.audio_preview.size());
					f.close();
					//dout << "saved" << ms->file_index << "waveform to" << get_waveform_path(hash, ms);
				}

				sem.release();
			}
		}
		avformat_close_input(&fmt_ctx);
	}

	if (error) {
		media->update_tooltip(errorStr);
		emit set_icon(ICON_TYPE_ERROR, replace);
		footage->invalid = true;
		footage->ready_lock.unlock();
	} else {
		media->update_tooltip();
	}

	delete [] filename;
	footage->preview_gen = nullptr;
}

void PreviewGenerator::cancel() {
	cancelled = true;
}
