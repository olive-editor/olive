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
class QMouseEvent;

struct Clip;
class QXmlStreamReader;
class QXmlStreamWriter;
class Effect;
class EffectRow;
class CheckboxEx;

struct EffectMeta {
	QString name;
	QString category;
	QString filename;
	QString path;
	int internal;
	int type;
	int subtype;
};

extern QVector<EffectMeta> effects;

double log_volume(double linear);
void init_effects();
Effect* create_effect(Clip* c, const EffectMeta *em);
const EffectMeta* get_internal_meta(int internal_id, int type);

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
#define EFFECT_INTERNAL_TIMECODE 8
#define EFFECT_INTERNAL_MASK 9
#define EFFECT_INTERNAL_FILLLEFTRIGHT 10
#define EFFECT_INTERNAL_VST 11
#define EFFECT_INTERNAL_CORNERPIN 12
#define EFFECT_INTERNAL_COUNT 13

#define KEYFRAME_TYPE_LINEAR 0
#define KEYFRAME_TYPE_BEZIER 1
#define KEYFRAME_TYPE_HOLD 2

struct GLTextureCoords {
	int grid_size;

	int vertexTopLeftX;
	int vertexTopLeftY;
	int vertexTopLeftZ;
	int vertexTopRightX;
	int vertexTopRightY;
	int vertexTopRightZ;
	int vertexBottomLeftX;
	int vertexBottomLeftY;
	int vertexBottomLeftZ;
	int vertexBottomRightX;
	int vertexBottomRightY;
	int vertexBottomRightZ;

	float textureTopLeftX;
	float textureTopLeftY;
	float textureTopLeftQ;
	float textureTopRightX;
	float textureTopRightY;
	float textureTopRightQ;
	float textureBottomRightX;
	float textureBottomRightY;
	float textureBottomRightQ;
	float textureBottomLeftX;
	float textureBottomLeftY;
	float textureBottomLeftQ;
};

qint16 mix_audio_sample(qint16 a, qint16 b);

#include "effectfield.h"
#include "effectrow.h"
#include "effectgizmo.h"

class Effect : public QObject {
	Q_OBJECT
public:
	Effect(Clip* c, const EffectMeta* em);
	~Effect();
	Clip* parent_clip;
	const EffectMeta* meta;
	int id;
	QString name;
	CollapsibleWidget* container;

	EffectRow* add_row(const QString &name, bool savable = true, bool keyframable = true);
	EffectRow* row(int i);
	int row_count();

	EffectGizmo* add_gizmo(int type);
	EffectGizmo* gizmo(int i);
	int gizmo_count();

	bool is_enabled();
	void set_enabled(bool b);

	virtual void refresh();

	Effect* copy(Clip* c);
	void copy_field_keyframes(Effect *e);

    virtual void load(QXmlStreamReader& stream);
	virtual void custom_load(QXmlStreamReader& stream);
	virtual void save(QXmlStreamWriter& stream);

	// glsl handling
	bool is_open();
	void open();
	void close();
	bool is_glsl_linked();
	virtual void startEffect();
	virtual void endEffect();

	bool enable_shader;
	bool enable_coords;
	bool enable_superimpose;
	bool enable_image;

	int getIterations();
	void setIterations(int i);

	const char* ffmpeg_filter;

	virtual void process_image(double timecode, uint8_t* data, int size);
	virtual void process_shader(double timecode, GLTextureCoords&);
	virtual void process_coords(double timecode, GLTextureCoords& coords, int data);
	virtual GLuint process_superimpose(double timecode);
	virtual void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

	virtual void gizmo_draw(double timecode, GLTextureCoords& coords);
	void gizmo_move(EffectGizmo* sender, int x_movement, int y_movement, double timecode, bool done);
	void gizmo_world_to_screen();
	bool are_gizmos_enabled();
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

	// enable effect to update constantly
	bool enable_always_update;
private:
	// superimpose effect
	QString script;

	bool isOpen;
	QVector<EffectRow*> rows;
	QVector<EffectGizmo*> gizmos;
	QGridLayout* ui_layout;
	QWidget* ui;
	bool bound;

	// superimpose functions
	virtual void redraw(double timecode);
	bool valueHasChanged(double timecode);
	QVector<QVariant> cachedValues;
	void delete_texture();
	int get_index_in_clip();
	void validate_meta_path();
};

class EffectInit : public QThread {
public:
	EffectInit();
protected:
	void run();
};

#endif // EFFECT_H
