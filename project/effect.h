#ifndef EFFECT_H
#define EFFECT_H

#include <QObject>
#include <QString>
#include <QVector>
class QWidget;
class CollapsibleWidget;
class QGridLayout;

struct Clip;
class QXmlStreamReader;
class QXmlStreamWriter;
class Effect;

#define EFFECT_TYPE_INVALID 0
#define EFFECT_TYPE_VIDEO 1
#define EFFECT_TYPE_AUDIO 2

#define EFFECT_FIELD_DOUBLE 0
#define EFFECT_FIELD_COLOR 1
#define EFFECT_FIELD_STRING 2
#define EFFECT_FIELD_BOOL 3
#define EFFECT_FIELD_COMBO 4
#define EFFECT_FIELD_FONT 5

class EffectField : QObject {
	Q_OBJECT
public:
	EffectField(Effect* parent, int t);
	int type;

	double get_double_value();
	void set_double_value(double v);
	void set_double_default_value(double v);
	void set_double_minimum_value(double v);
	void set_double_maximum_value(double v);

	const QString get_string_value();
	void set_string_value(const QString &s);

	void add_combo_item(const QString& name, const QVariant &data);
	int get_combo_index();
	const QVariant get_combo_data();
	const QString get_combo_string();
	void set_combo_index(int index);
	void set_combo_string(const QString& s);

	bool get_bool_value();
	void set_bool_value(bool b);

	const QString get_font_name();

	QColor get_color_value();
	void set_color_value(QColor color);

	QWidget* get_ui_element();
	void set_enabled(bool e);
private:
	QWidget* ui_element;
};

class EffectRow {
public:
	EffectRow(Effect* parent, QGridLayout* uilayout, const QString& n, int row);
	~EffectRow();
	EffectField* add_field(int type);
	EffectField* field(int i);
	int fieldCount();
	void set_keyframe(int field, long time);
	void move_keyframe(int field, long from, long to);
	void delete_keyframe(int field, long time);
private:
	Effect* parent_effect;
	QGridLayout* ui;
	QString name;
	int ui_row;
	QVector<EffectField*> fields;
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

    bool is_enabled();

	virtual void refresh();

    virtual Effect* copy(Clip* c);

	void load(QXmlStreamReader* stream);
	void save(QXmlStreamWriter* stream);

	virtual void process_gl(int* anchor_x, int* anchor_y);
    virtual void post_gl();
    virtual void process_audio(quint8* samples, int nb_bytes);
public slots:
	void field_changed();
    void checkbox_command();
private:
	QVector<EffectRow*> rows;
	QGridLayout* ui_layout;
	QWidget* ui;
};

#endif // EFFECT_H
