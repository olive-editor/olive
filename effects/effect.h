#ifndef EFFECT_H
#define EFFECT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QColor>
#include <QOpenGLShaderProgram>
class QLabel;
class QWidget;
class CollapsibleWidget;
class QGridLayout;

struct Clip;
class QXmlStreamReader;
class QXmlStreamWriter;
class Effect;
class EffectRow;
class CheckboxEx;
class KeyframeDelete;

enum VideoEffects {
	VIDEO_TRANSFORM_EFFECT,
	VIDEO_SHAKE_EFFECT,
	VIDEO_TEXT_EFFECT,
	VIDEO_SOLID_EFFECT,
	VIDEO_INVERT_EFFECT,
	VIDEO_CHROMAKEY_EFFECT,
	VIDEO_GAUSSIANBLUR_EFFECT,
	VIDEO_CROP_EFFECT,
	VIDEO_FLIP_EFFECT,
	VIDEO_EFFECT_COUNT
};

enum AudioEffects {
	AUDIO_VOLUME_EFFECT,
	AUDIO_PAN_EFFECT,
	AUDIO_NOISE_EFFECT,
	AUDIO_EFFECT_COUNT
};

extern QVector<QString> video_effect_names;
extern QVector<QString> audio_effect_names;
void init_effects();
Effect* create_effect(int effect_id, Clip* c);

#define EFFECT_TYPE_INVALID 0
#define EFFECT_TYPE_VIDEO 1
#define EFFECT_TYPE_AUDIO 2

#define EFFECT_FIELD_DOUBLE 0
#define EFFECT_FIELD_COLOR 1
#define EFFECT_FIELD_STRING 2
#define EFFECT_FIELD_BOOL 3
#define EFFECT_FIELD_COMBO 4
#define EFFECT_FIELD_FONT 5

#define EFFECT_KEYFRAME_LINEAR 0
#define EFFECT_KEYFRAME_HOLD 1
#define EFFECT_KEYFRAME_BEZIER 2

struct GLTextureCoords {
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

class EffectField : public QObject {
	Q_OBJECT
public:
    EffectField(EffectRow* parent, int t);
    EffectRow* parent_row;
	int type;

    QVariant get_current_data();
	double frameToTimecode(long frame);
	long timecodeToFrame(double timecode);
	void set_current_data(const QVariant&);
	void get_keyframe_data(double timecode, int& before, int& after, double& d);
	void validate_keyframe_data(double timecode);
//  QVariant get_keyframe_data(long p);
//	bool is_keyframed(long p);

	double get_double_value(double timecode);
	void set_double_value(double v);
	void set_double_default_value(double v);
	void set_double_minimum_value(double v);
	void set_double_maximum_value(double v);

	const QString get_string_value(double timecode);
	void set_string_value(const QString &s);

	void add_combo_item(const QString& name, const QVariant &data);
	int get_combo_index(double timecode);
	const QVariant get_combo_data(double timecode);
	const QString get_combo_string(double timecode);
	void set_combo_index(int index);
	void set_combo_string(const QString& s);

	bool get_bool_value(double timecode);
	void set_bool_value(bool b);

	const QString get_font_name(double timecode);
	void set_font_name(const QString& s);

	QColor get_color_value(double timecode);
	void set_color_value(QColor color);

	QWidget* get_ui_element();
	void set_enabled(bool e);
    QVector<QVariant> keyframe_data;
	QWidget* ui_element;
private:
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
private:
	bool keyframing;
	QGridLayout* ui;
	QString name;
	int ui_row;
	QVector<EffectField*> fields;

	CheckboxEx* keyframe_enable;

	bool just_made_unsafe_keyframe;
};

class Effect : public QObject {
	Q_OBJECT
public:
	Effect(Clip* c, int t, int i);
	~Effect();
    Clip* parent_clip;
	int type;
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

	bool enable_image;
	bool enable_opengl;

	const char* ffmpeg_filter;

	virtual void process_image(double timecode, uint8_t* data, int width, int height);
	virtual void process_gl(double timecode, GLTextureCoords& coords);
	virtual void clean_gl();
	virtual void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);
public slots:
	void field_changed();
private:
	QVector<EffectRow*> rows;
	QGridLayout* ui_layout;
	QWidget* ui;
};

#endif // EFFECT_H
