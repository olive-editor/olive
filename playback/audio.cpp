#include "audio.h"

#include "project/sequence.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "ui_timeline.h"

#include <QAudioOutput>
#include <QtMath>
#include <QDebug>

extern "C" {
	#include <libavcodec/avcodec.h>
}

QAudioOutput* audio_output;
QIODevice* audio_io_device;
bool audio_device_set = false;
QMutex audio_write_lock;

qint8 audio_ibuffer[audio_ibuffer_size];
int audio_ibuffer_read = 0;
long audio_ibuffer_frame = 0;

AudioSenderThread* audio_thread;

void init_audio() {
	stop_audio();

    if (sequence != NULL) {
		QAudioFormat audio_format;
        audio_format.setSampleRate(sequence->audio_frequency);
        audio_format.setChannelCount(av_get_channel_layout_nb_channels(sequence->audio_layout));
		audio_format.setSampleSize(16);
		audio_format.setCodec("audio/pcm");
		audio_format.setByteOrder(QAudioFormat::LittleEndian);
		audio_format.setSampleType(QAudioFormat::SignedInt);

		QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
		if (!info.isFormatSupported(audio_format)) {
			qWarning() << "[WARNING] Couldn't initialize audio. Audio format is not supported by backend";
		} else {
			audio_output = new QAudioOutput(audio_format);
			audio_output->setNotifyInterval(5);

			// connect
			audio_io_device = audio_output->start();
			audio_device_set = true;

			// start sender thread
			audio_thread = new AudioSenderThread();
			QObject::connect(audio_output, SIGNAL(notify()), audio_thread, SLOT(notifyReceiver()));
			audio_thread->start(QThread::TimeCriticalPriority);

            clear_audio_ibuffer();
		}
	}
}

void stop_audio() {
	if (audio_device_set) {
		audio_thread->stop();

		audio_output->stop();
		delete audio_output;
		audio_device_set = false;
	}
}

void clear_audio_ibuffer() {
	memset(audio_ibuffer, 0, audio_ibuffer_size);
    audio_ibuffer_read = 0;
}

int get_buffer_offset_from_frame(long frame) {
	// currently debugging this function. since it has a high potential of failure and isn't actually fatal, we only assert on debug mode
#ifdef QT_DEBUG
	Q_ASSERT(frame >= audio_ibuffer_frame);
	return qFloor(av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(sequence->audio_layout), qRound(((frame-audio_ibuffer_frame)/sequence->frame_rate)*sequence->audio_frequency), AV_SAMPLE_FMT_S16, 1)/4)*4;
#else
	if (frame >= audio_ibuffer_frame) {
		return qFloor(av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(sequence->audio_layout), qRound(((frame-audio_ibuffer_frame)/sequence->frame_rate)*sequence->audio_frequency), AV_SAMPLE_FMT_S16, 1)/4)*4;
	} else {
		qDebug() << "[WARNING] Invalid values passed to get_buffer_offset_from_frame";
		return 0;
	}
#endif
}

AudioSenderThread::AudioSenderThread() : close(false) {
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void AudioSenderThread::stop() {
	close = true;
	cond.wakeAll();
	wait();
}

void AudioSenderThread::notifyReceiver() {
	cond.wakeAll();
}

void AudioSenderThread::run() {
	// start data loop
	send_audio_to_output(0, audio_ibuffer_size);

	lock.lock();
	while (true) {
		cond.wait(&lock);
		if (close) {
			break;
		} else if (panel_timeline->playing) {
			int written_bytes = 0;

			int adjusted_read_index = audio_ibuffer_read%audio_ibuffer_size;
			int max_write = audio_ibuffer_size - adjusted_read_index;
			int actual_write = send_audio_to_output(adjusted_read_index, max_write);
			written_bytes += actual_write;
			if (actual_write == max_write) {
				// got all the bytes, write again
				written_bytes += send_audio_to_output(0, audio_ibuffer_size);
			}
		}
	}
	lock.unlock();
}

int AudioSenderThread::send_audio_to_output(int offset, int max) {
	// send audio to device
	int actual_write = audio_io_device->write((const char*) audio_ibuffer+offset, max);

	int audio_ibuffer_limit = audio_ibuffer_read + actual_write;

	// send samples to audio monitor cache
	if (panel_timeline->ui->audio_monitor->sample_cache_offset == -1) {
		panel_timeline->ui->audio_monitor->sample_cache_offset = sequence->playhead;
	}
	int channel_count = av_get_channel_layout_nb_channels(sequence->audio_layout);
	long sample_cache_playhead = panel_timeline->ui->audio_monitor->sample_cache_offset + (panel_timeline->ui->audio_monitor->sample_cache.size()/channel_count);
	int next_buffer_offset, buffer_offset_adjusted, i;
	int buffer_offset = get_buffer_offset_from_frame(sample_cache_playhead);
	if (samples.size() != channel_count) samples.resize(channel_count);
	samples.fill(0);

	// TODO: I don't like this, but i'm not sure if there's a smarter way to do it
	while (buffer_offset < audio_ibuffer_limit) {
		sample_cache_playhead++;
		next_buffer_offset = qMin(get_buffer_offset_from_frame(sample_cache_playhead), audio_ibuffer_limit);
		while (buffer_offset < next_buffer_offset) {
			for (i=0;i<samples.size();i++) {
				buffer_offset_adjusted = buffer_offset%audio_ibuffer_size;
				samples[i] = qMax(qAbs((qint16) (((audio_ibuffer[buffer_offset_adjusted+1] & 0xFF) << 8) | (audio_ibuffer[buffer_offset_adjusted] & 0xFF))), samples[i]);
				buffer_offset += 2;
			}
		}
		panel_timeline->ui->audio_monitor->sample_cache.append(samples);
		buffer_offset = next_buffer_offset;
	}

	memset(audio_ibuffer+offset, 0, actual_write);

	audio_ibuffer_read = audio_ibuffer_limit;

	return actual_write;
}
