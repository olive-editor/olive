#ifndef EFFECT_H
#define EFFECT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QColor>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMutex>
#include <QThread>
class QLabel;
class QWidget;
class CollapsibleWidget;
class QGridLayout;
class QPushButton;

struct Clip;
class QXmlStreamReader;
class QXmlStreamWriter;
class Effect;
class EffectRow;
class CheckboxEx;
class KeyframeDelete;

struct EffectMeta {
    QString name;
    QString category;
    QString filename;
	int internal;
	int type;
};

extern QVector<EffectMeta> video_effects;
extern QVector<EffectMeta> audio_effects;

double log_volume(double linear);
void init_effects();
Effect* create_effect(Clip* c, const EffectMeta *em);
const EffectMeta* get_internal_meta(int internal_id);

extern QMutex effects_loaded;

#define EFFECT_TYPE_INVALID 0
#define EFFECT_TYPE_VIDEO 1
#define EFFECT_TYPE_AUDIO 2
#define EFFECT_TYPE_EFFECT 3
#define EFFECT_TYPE_TRANSITION 4

#define EFFECT_KEYFRAME_LINEAR 0
#define EFFECT_KEYFRAME_HOLD 1
#define EFFECT_KEYFRAME_BEZIER 2

#define EFFECT_INTERNAL_TRANSFORM 0
#define EFFECT_INTERNAL_TEXT 1
#define EFFECT_INTERNAL_SOLID 2
#define EFFECT_INTERNAL_NOISE 3
#define EFFECT_INTERNAL_VOLUME 4
#define EFFECT_INTERNAL_PAN 5
#define EFFECT_INTERNAL_TONE 6
#define EFFECT_INTERNAL_SHAKE 7




#define EFFECT_INTERNAL_CORNERPIN 12
#define EFFECT_INTERNAL_COUNT 13

#define KEYFRAME_TYPE_LINEAR 0
#define KEYFRAME_TYPE_SMOOTH 1
#define KEYFRAME_TYPE_BEZIER 2

struct GLTextureCoords {
    int grid_size;

	int vertexTopLeftX;
	int vertexTopLeftY;
	int vertexTopRightX;
	int vertexTopRightY;
	int vertexBottomLeftX;
	int vertexBottomLeftY;
	int vertexBottomRightX;
	int vertexBottomRightY;

	double textureTopLeftX;
	double textureTopLeftY;
	double textureTopRightX;
	double textureTopRightY;
	double textureBottomRightX;
	double textureBottomRightY;
	double textureBottomLeftX;
	double textureBottomLeftY;
};

qint16 mix_audio_sample(qint16 a, qint16 b);

#include "effectfield.h"
#include "effectrow.h"

class Effect : public QObject {
	Q_OBJECT
public:
	Effect(Clip* c, const EffectMeta* em);
	~Effect();
    Clip* parent_clip;
	const EffectMeta* meta;
    long length; // used only for transitions
    int id;
	QString name;
	CollapsibleWidget* container;

	EffectRow* add_row(const QString &name);
	EffectRow* row(int i);
	int row_count();

    bool is_enabled();
	void set_enabled(bool b);

	virtual void refresh();

	Effect* copy(Clip* c);
	void copy_field_keyframes(Effect *e);

	void load(QXmlStreamReader& stream);
    void save(QXmlStreamWriter& stream);

	// glsl handling
	void open();
	void close();
	virtual void startEffect();
	virtual void endEffect();

	bool enable_shader;
	bool enable_coords;
	bool enable_superimpose;

	int getIterations();
	void setIterations(int i);

	const char* ffmpeg_filter;

	virtual void process_shader(double timecode, GLTextureCoords&);
    virtual void process_coords(double timecode, GLTextureCoords& coords, int data);
	virtual GLuint process_superimpose(double timecode);
	virtual void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
public slots:
	void field_changed();
private slots:
	void show_context_menu(const QPoint&);
	void delete_self();
	void move_up();
	void move_down();
protected:
	// glsl effect
	QOpenGLShaderProgram* glslProgram;
	QString vertPath;
	QString fragPath;

	// superimpose effect
	QImage img;
	QOpenGLTexture* texture;
private:
	// superimpose effect
	QString script;

	bool isOpen;
	QVector<EffectRow*> rows;
	QGridLayout* ui_layout;
    QWidget* ui;
	bool bound;
    bool enable_always_update;

	// superimpose functions
	virtual void redraw(double timecode);
	bool valueHasChanged(double timecode);
	QVector<QVariant> cachedValues;
	void delete_texture();
	int get_index_in_clip();
};

class EffectInit : public QThread {
public:
	EffectInit();
protected:
	void run();
};

#endif // EFFECT_H
