#ifndef EFFECTS_H
#define EFFECTS_H

#include "project/effect.h"

class QSpinBox;
class QCheckBox;

enum VideoEffects {
	VIDEO_TRANSFORM_EFFECT,
	VIDEO_EFFECT_COUNT
};

enum AudioEffects {
	AUDIO_VOLUME_EFFECT,
	AUDIO_PAN_EFFECT,
	AUDIO_EFFECT_COUNT
};

extern QVector<QString> video_effect_names;
extern QVector<QString> audio_effect_names;
void init_effects();
Effect* create_effect(int effect_id, Clip* c);

// video effects
class TransformEffect : public Effect {
	Q_OBJECT
public:
    TransformEffect(Clip* c);
	void process_gl(int* anchor_x, int* anchor_y);
    Effect* copy();

	QSpinBox* position_x;
	QSpinBox* position_y;
	QSpinBox* scale_x;
	QSpinBox* scale_y;
	QCheckBox* uniform_scale_box;
	QSpinBox* rotation;
	QSpinBox* anchor_x_box;
	QSpinBox* anchor_y_box;
	QSpinBox* opacity;
public slots:
	void toggle_uniform_scale(bool enabled);
};

// audio effects
class VolumeEffect : public Effect {
public:
    VolumeEffect(Clip* c);
    void process_audio(uint8_t* samples, int nb_bytes);
    Effect* copy();

	QSpinBox* volume_val;
};

class PanEffect : public Effect {
public:
    PanEffect(Clip* c);
    void process_audio(uint8_t* samples, int nb_bytes);
    Effect* copy();

	QSpinBox* pan_val;
};

#endif // EFFECTS_H
