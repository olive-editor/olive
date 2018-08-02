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
    void init();
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
    ComboBoxEx* blend_mode_box;
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
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);

    LabelSlider* intensity_val;
    LabelSlider* rotation_val;
    LabelSlider* frequency_val;
public slots:
    void init();
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
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);
    QTextEdit* text_val;
    LabelSlider* size_val;
    ColorButton* set_color_button;
    ComboBoxEx* set_font_combobox;
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

// audio effects
class VolumeEffect : public Effect {
public:
    VolumeEffect(Clip* c);
    void init();
    void process_audio(quint8* samples, int nb_bytes);
    Effect* copy(Clip* c);
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);

    LabelSlider* volume_val;
};

class PanEffect : public Effect {
public:
    PanEffect(Clip* c);
    void init();
    void process_audio(quint8* samples, int nb_bytes);
    Effect* copy(Clip* c);
    void load(QXmlStreamReader* stream);
    void save(QXmlStreamWriter* stream);

    LabelSlider* pan_val;
};

#endif // EFFECTS_H
