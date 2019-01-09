#ifndef EFFECTROW_H
#define EFFECTROW_H

#include <QObject>
#include <QVector>

class Effect;
class QGridLayout;
class EffectField;
class QLabel;
class QPushButton;
class ComboAction;
class QHBoxLayout;
class KeyframeNavigator;
class ClickableLabel;

class EffectRow : public QObject {
	Q_OBJECT
public:
	EffectRow(Effect* parent, bool save, QGridLayout* uilayout, const QString& n, int row, bool keyframable = true);
	~EffectRow();
	EffectField* add_field(int type, const QString &id, int colspan = 1);
	void add_widget(QWidget *w);
	EffectField* field(int i);
	int fieldCount();
	void set_keyframe_now(ComboAction *ca);
	void delete_keyframe_at_time(ComboAction *ca, long time);
	ClickableLabel* label;
	Effect* parent_effect;
	bool savable;
	const QString& get_name();

	bool isKeyframing();
	void setKeyframing(bool);
public slots:
	void goto_previous_key();
	void toggle_key();
	void goto_next_key();
	void focus_row();
private slots:
	void set_keyframe_enabled(bool);
private:
	bool keyframing;
	QGridLayout* ui;
	QString name;
	int ui_row;
	QVector<EffectField*> fields;

	KeyframeNavigator* keyframe_nav;

	bool just_made_unsafe_keyframe;
	QVector<int> unsafe_keys;
	QVector<QVariant> unsafe_old_data;
	QVector<bool> key_is_new;

	int column_count;
};

#endif // EFFECTROW_H
