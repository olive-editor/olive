#ifndef EFFECTFIELD_H
#define EFFECTFIELD_H

#define EFFECT_FIELD_DOUBLE 0
#define EFFECT_FIELD_COLOR 1
#define EFFECT_FIELD_STRING 2
#define EFFECT_FIELD_BOOL 3
#define EFFECT_FIELD_COMBO 4
#define EFFECT_FIELD_FONT 5
#define EFFECT_FIELD_FILE 6

#include <QObject>
#include <QVariant>
#include <QVector>

#include "keyframe.h"

class EffectRow;
class ComboAction;

class EffectField : public QObject {
	Q_OBJECT
public:
	EffectField(EffectRow* parent, int t, const QString& i);
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

	QString get_string_value(double timecode, bool async = false);
	void set_string_value(const QString &s);

	void add_combo_item(const QString& name, const QVariant &data);
	int get_combo_index(double timecode, bool async = false);
	QVariant get_combo_data(double timecode);
	QString get_combo_string(double timecode);
	void set_combo_index(int index);
	void set_combo_string(const QString& s);

	bool get_bool_value(double timecode, bool async = false);
	void set_bool_value(bool b);

	QString get_font_name(double timecode, bool async = false);
	void set_font_name(const QString& s);

	QColor get_color_value(double timecode, bool async = false);
	void set_color_value(QColor color);

	QString get_filename(double timecode, bool async = false);
	void set_filename(const QString& s);

	QWidget* get_ui_element();
	void set_enabled(bool e);
	QVector<EffectKeyframe> keyframes;
	QWidget* ui_element;

	void make_key_from_change(ComboAction* ca);
private:
	bool hasKeyframes();
private slots:
	void ui_element_change();
signals:
	void changed();
	void toggled(bool);
	void clicked();
};

#endif // EFFECTFIELD_H
