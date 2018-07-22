#include "effects/effects.h"

#include "project/clip.h"

#include <QVector>
#include <QDebug>

QVector<QString> video_effect_names;
QVector<QString> audio_effect_names;

void init_effects() {
	video_effect_names.resize(VIDEO_EFFECT_COUNT);
	audio_effect_names.resize(AUDIO_EFFECT_COUNT);

	video_effect_names[VIDEO_TRANSFORM_EFFECT] = "Transform";
    video_effect_names[VIDEO_SHAKE_EFFECT] = "Shake";
    video_effect_names[VIDEO_TEXT_EFFECT] = "Text";
    video_effect_names[VIDEO_SOLID_EFFECT] = "Solid";

	audio_effect_names[AUDIO_VOLUME_EFFECT] = "Volume";
	audio_effect_names[AUDIO_PAN_EFFECT] = "Pan";
}

Effect* create_effect(int effect_id, Clip* c) {
    if (c->track < 0) {
        switch (effect_id) {
        case VIDEO_TRANSFORM_EFFECT: return new TransformEffect(c); break;
        case VIDEO_SHAKE_EFFECT: return new ShakeEffect(c); break;
        case VIDEO_TEXT_EFFECT: return new TextEffect(c); break;
        case VIDEO_SOLID_EFFECT: return new SolidEffect(c); break;
        }
    } else {
        switch (effect_id) {
        case AUDIO_VOLUME_EFFECT: return new VolumeEffect(c); break;
        case AUDIO_PAN_EFFECT: return new PanEffect(c); break;
        }
    }
    qDebug() << "[ERROR] Invalid effect ID";
    return NULL;
}
