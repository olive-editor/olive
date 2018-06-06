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

uint8_t* audio_cache_A = NULL;
uint8_t* audio_cache_B = NULL;
int audio_cache_size = 0;
bool switch_audio_cache = true;
bool reading_audio_cache_A = false;
int audio_bytes_written = 0;

void init_audio(Sequence* s) {
	if (audio_cache_A != NULL) {
		delete [] audio_cache_A;
		audio_cache_A = NULL;
	}

	if (audio_cache_B != NULL) {
		delete [] audio_cache_B;
		audio_cache_B = NULL;
	}

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

			audio_cache_size = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(s->audio_layout), s->audio_frequency/8, AV_SAMPLE_FMT_S16, 1);
			audio_cache_A = new uint8_t[audio_cache_size];
			audio_cache_B = new uint8_t[audio_cache_size];
			clear_cache(true, true);
		}
	}
}

void clear_cache(bool clear_A, bool clear_B) {
	if (clear_A) {
		memset(audio_cache_A, 0, audio_cache_size);
	}
	if (clear_B) {
		memset(audio_cache_B, 0, audio_cache_size);
	}
}
