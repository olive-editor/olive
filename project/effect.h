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

#define EFFECT_FIELD_DOUBLE 0
#define EFFECT_FIELD_COLOR 1
#define EFFECT_FIELD_STRING 2
#define EFFECT_FIELD_BOOL 3
#define EFFECT_FIELD_COMBO 4
#define EFFECT_FIELD_FONT 5

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
#define EFFECT_INTERNAL_CROSSDISSOLVE 8
#define EFFECT_INTERNAL_LINEARFADE 9
#define EFFECT_INTERNAL_EXPONENTIALFADE	10
#define EFFECT_INTERNAL_LOGARITHMICFADE 11
#define EFFECT_INTERNAL_CORNERPIN 12
#define EFFECT_INTERNAL_COUNT 13

#define KEYFRAME_TYPE_LINEAR 0
#define KEYFRAME_TYPE_SMOOTH 1
#define KEYFRAME_TYPE_BEZIER 2

#define TRAN_TYPE_OPEN 1
#define TRAN_TYPE_CLOSE 2
#define TRAN_TYPE_OPENWLINK 3
#define TRAN_TYPE_CLOSEWLINK 4

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

class EffectField : public QObject {
	Q_OBJECT
public:
    EffectField(EffectRow* parent, int t);
    EffectRow* parent_row;
	int type;
	QString id;

	QVariant get_previous_data();
    QVariant get_current_data();
	double frameToTimecode(long frame);
	long timecodeToFrame(double timecode);
	void set_current_data(const QVariant&);
	void get_keyframe_data(double timecode, int& before, int& after, double& d);
	QVariant validate_keyframe_data(double timecode, bool async = false);

	double get_double_value(double timecode, bool async = false);
	void set_double_value(double v);
	void set_double_default_value(double v);
	void set_double_minimum_value(double v);
	void set_double_maximum_value(double v);

	const QString get_string_value(double timecode, bool async = false);
	void set_string_value(const QString &s);

	void add_combo_item(const QString& name, const QVariant &data);
	int get_combo_index(double timecode, bool async = false);
	const QVariant get_combo_data(double timecode);
	const QString get_combo_string(double timecode);
	void set_combo_index(int index);
	void set_combo_string(const QString& s);

	bool get_bool_value(double timecode, bool async = false);
	void set_bool_value(bool b);

	const QString get_font_name(double timecode, bool async = false);
	void set_font_name(const QString& s);

	QColor get_color_value(double timecode, bool async = false);
	void set_color_value(QColor color);

	QWidget* get_ui_element();
	void set_enabled(bool e);
    QVector<QVariant> keyframe_data;
	QWidget* ui_element;
private:
	bool hasKeyframes();
private slots:
    void uiElementChange();
signals:
	void changed();
	void toggled(bool);
};

class EffectRow : public QObject {
    Q_OBJECT
public:
	EffectRow(Effect* parent, QGridLayout* uilayout, const QString& n, int row);
	~EffectRow();
	EffectField* add_field(int type, int colspan = 1);
	EffectField* field(int i);
    int fieldCount();
	void set_keyframe_now(bool undoable);
	void delete_keyframe(KeyframeDelete *kd, int index);
	void delete_keyframe_at_time(KeyframeDelete* kd, long time);
	QLabel* label;
	Effect* parent_effect;

	bool isKeyframing();
	void setKeyframing(bool);

    QVector<long> keyframe_times;
    QVector<int> keyframe_types;
private slots:
	void set_keyframe_enabled(bool);
	void keyframe_ui_enabled(bool);
	void goto_previous_key();
	void toggle_key();
	void goto_next_key();
private:
	bool keyframing;
	QGridLayout* ui;
	QString name;
	int ui_row;
	QVector<EffectField*> fields;

    QPushButton* keyframe_enable;
	QPushButton* left_key_nav;
	QPushButton* key_addremove;
	QPushButton* right_key_nav;

	bool just_made_unsafe_keyframe;
};

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

	long length; // only used for transitions
	Effect* tlink;

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
	virtual void process_coords(double timecode, GLTextureCoords& coords);
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
