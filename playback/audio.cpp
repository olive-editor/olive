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
long audio_ibuffer_frame = 0;

void init_audio() {
	if (audio_device_set) {
		audio_output->stop();
		delete audio_output;
		audio_device_set = false;
	}

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
            audio_output->setBufferSize(20480);

			// connect
			audio_io_device = audio_output->start();
			audio_device_set = true;

            clear_audio_ibuffer();
		}
	}
}

void clear_audio_ibuffer() {
    memset(audio_ibuffer, 0, audio_ibuffer_size);
    audio_ibuffer_read = 0;
}

int get_buffer_offset_from_frame(long frame) {
    if (frame >= audio_ibuffer_frame) {
        return av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(sequence->audio_layout), ((frame-audio_ibuffer_frame)/sequence->frame_rate)*sequence->audio_frequency, AV_SAMPLE_FMT_S16, 1);
    } else {
        qDebug() << "[WARNING] get_buffer_offset_from_frame called incorrectly";
        return 0;
    }
}
