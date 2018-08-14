#ifndef EFFECTS_H
#define EFFECTS_H

#include "project/effect.h"

#include <QPixmap>
#include <QFont>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
class QSpinBox;
class QCheckBox;
class LabelSlider;
class QOpenGLTexture;
class QTextEdit;
class QPushButton;
class ColorButton;
class ComboBoxEx;

enum VideoEffects {
	VIDEO_TRANSFORM_EFFECT,
    VIDEO_SHAKE_EFFECT,
    VIDEO_TEXT_EFFECT,
    VIDEO_SOLID_EFFECT,
	VIDEO_INVERT_EFFECT,
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
	void refresh();
	void process_gl(int* anchor_x, int* anchor_y);

	EffectField* position_x;
	EffectField* position_y;
	EffectField* scale_x;
	EffectField* scale_y;
	EffectField* uniform_scale_field;
	EffectField* rotation;
	EffectField* anchor_x_box;
	EffectField* anchor_y_box;
	EffectField* opacity;
	EffectField* blend_mode_box;
public slots:
	void toggle_uniform_scale(bool enabled);
private:
    int default_anchor_x;
    int default_anchor_y;
};

class ShakeEffect : public Effect {
    Q_OBJECT
public:
    ShakeEffect(Clip* c);
    void process_gl(int* anchor_x, int* anchor_y);
	Effect* copy(Clip *c);

	EffectField* intensity_val;
	EffectField* rotation_val;
	EffectField* frequency_val;
public slots:
	void refresh();
private:
    int shake_progress;
    int shake_limit;
    int next_x;
    int next_y;
    int next_rot;
    int offset_x;
    int offset_y;
    int offset_rot;
    int prev_x;
    int prev_y;
    int prev_rot;
    int perp_x;
    int perp_y;
    double t;
    bool inside;
};

class TextEffect : public Effect {
    Q_OBJECT
public:
    TextEffect(Clip* c);
    ~TextEffect();
    void post_gl();
	Effect* copy(Clip* c);

	EffectField* text_val;
	EffectField* size_val;
	EffectField* set_color_button;
	EffectField* set_font_combobox;
private slots:
    void update_texture();
private:
    void destroy_texture();
    QOpenGLTexture* texture;
    QImage pixmap;
    QFont font;
    int width;
    int height;
};

class SolidEffect : public Effect {
    Q_OBJECT
public:
    SolidEffect(Clip* c);
    void post_gl();
private slots:
    void update_texture();
private:
    QOpenGLTexture* texture;
};

class InvertEffect : public Effect {
	Q_OBJECT
public:
	InvertEffect(Clip* c);
	void process_gl(int *anchor_x, int *anchor_y);
	Effect* copy(Clip *c);

	EffectField* amount_val;
};

// audio effects
class VolumeEffect : public Effect {
public:
    VolumeEffect(Clip* c);
	void refresh();
    void process_audio(quint8* samples, int nb_bytes);
	Effect* copy(Clip* c);

	EffectField* volume_val;
};

class PanEffect : public Effect {
public:
    PanEffect(Clip* c);
	void refresh();
    void process_audio(quint8* samples, int nb_bytes);
	Effect* copy(Clip* c);

	EffectField* pan_val;
};

#endif // EFFECTS_H
