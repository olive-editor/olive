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

class EffectField : public QObject {
	Q_OBJECT
public:
    EffectField(EffectRow* parent, int t);
    EffectRow* parent_row;
	int type;

    QVariant get_current_data();
    void set_keyframe_data(int i);
    void get_keyframe_data(long frame, int* before, int *after, double* d);
    void validate_keyframe_data(long frame);
//  QVariant get_keyframe_data(long p);
//	bool is_keyframed(long p);

	double get_double_value(long p);
	void set_double_value(double v);
	void set_double_default_value(double v);
	void set_double_minimum_value(double v);
	void set_double_maximum_value(double v);

	const QString get_string_value(long p);
	void set_string_value(const QString &s);

	void add_combo_item(const QString& name, const QVariant &data);
	int get_combo_index(long p);
	const QVariant get_combo_data(long p);
	const QString get_combo_string(long p);
	void set_combo_index(int index);
	void set_combo_string(const QString& s);

	bool get_bool_value(long p);
	void set_bool_value(bool b);

	const QString get_font_name(long p);
	void set_font_name(const QString& s);

	QColor get_color_value(long p);
	void set_color_value(QColor color);

	QWidget* get_ui_element();
	void set_enabled(bool e);
    QVector<QVariant> keyframe_data;
private:
    QWidget* ui_element;
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
	void set_keyframe(long time);
	void move_keyframe(long from, long to);
	void delete_keyframe(long time);
	QLabel* label;
	Effect* parent_effect;

    bool keyframing;
    QVector<long> keyframe_times;
    QVector<int> keyframe_types;
public slots:
    void set_keyframe_now();
private:
	QGridLayout* ui;
	QString name;
	int ui_row;
	QVector<EffectField*> fields;

	CheckboxEx* keyframe_enable;
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

	virtual void refresh();

    virtual Effect* copy(Clip* c);
	void copy_field_keyframes(Effect *e);

	void load(QXmlStreamReader* stream);
	void save(QXmlStreamWriter* stream);

	bool enable_image;
	bool enable_opengl;

	const char* ffmpeg_filter;

	virtual void process_image(long frame, QImage& img);
	virtual void process_gl(long frame, QOpenGLShaderProgram& shader_prog, int* anchor_x, int* anchor_y);
    virtual void process_audio(quint8* samples, int nb_bytes);
public slots:
	void field_changed();
private:
	QVector<EffectRow*> rows;
	QGridLayout* ui_layout;
	QWidget* ui;
};

#endif // EFFECT_H
