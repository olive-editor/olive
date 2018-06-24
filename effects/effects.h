#ifndef EFFECTS_H
#define EFFECTS_H

#include "project/effect.h"

#include <QXmlStreamWriter>
class QSpinBox;
class QCheckBox;
class QComboBox;
class LabelSlider;

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
    Effect* copy(Clip* c);
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);

    LabelSlider* position_x;
    LabelSlider* position_y;
    LabelSlider* scale_x;
    LabelSlider* scale_y;
	QCheckBox* uniform_scale_box;
    LabelSlider* rotation;
    LabelSlider* anchor_x_box;
    LabelSlider* anchor_y_box;
    LabelSlider* opacity;
    QComboBox* blend_mode_box;
public slots:
	void toggle_uniform_scale(bool enabled);
private:
    int default_anchor_x;
    int default_anchor_y;
};

// audio effects
class VolumeEffect : public Effect {
public:
    VolumeEffect(Clip* c);
    void process_audio(quint8* samples, int nb_bytes);
    Effect* copy(Clip* c);
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);

    LabelSlider* volume_val;
};

class PanEffect : public Effect {
public:
    PanEffect(Clip* c);
    void process_audio(quint8* samples, int nb_bytes);
    Effect* copy(Clip* c);
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);

    LabelSlider* pan_val;
};

#endif // EFFECTS_H
