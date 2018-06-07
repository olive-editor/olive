#include "audio.h"

#include "project/sequence.h"

#include <QAudioOutput>
#include <QDebug>

extern "C" {
	#include <libavcodec/avcodec.h>
}

QAudioOutput* audio_output;
QIODevice* audio_io_device;
bool audio_device_set = false;

qint8 audio_ibuffer[audio_ibuffer_size];
int audio_ibuffer_read = 0;
QVector<int> audio_ibuffer_write;

void init_audio(Sequence* s) {
	if (audio_device_set) {
		audio_output->stop();
		delete audio_output;
		audio_device_set = false;
	}

	if (s != NULL) {
		QAudioFormat audio_format;
		audio_format.setSampleRate(s->audio_frequency);
		audio_format.setChannelCount(av_get_channel_layout_nb_channels(s->audio_layout));
		audio_format.setSampleSize(16);
		audio_format.setCodec("audio/pcm");
		audio_format.setByteOrder(QAudioFormat::LittleEndian);
		audio_format.setSampleType(QAudioFormat::SignedInt);

		QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
		if (!info.isFormatSupported(audio_format)) {
			qWarning() << "[WARNING] Couldn't initialize audio. Audio format is not supported by backend";
		} else {
			audio_output = new QAudioOutput(audio_format);
			// connect
			audio_io_device = audio_output->start();
			audio_device_set = true;

            clear_audio_ibuffer();
		}
	}
}

void clear_audio_ibuffer() {
    memset(audio_ibuffer, 0, audio_ibuffer_size);
}
