#ifndef EFFECTROW_H
#define EFFECTROW_H

#include <QObject>
#include <QVector>

class Effect;
class QGridLayout;
class EffectField;
class QLabel;
class KeyframeDelete;
class QPushButton;
class ComboAction;
class QHBoxLayout;
class KeyframeNavigator;

class EffectRow : public QObject {
    Q_OBJECT
public:
    EffectRow(Effect* parent, bool save, QGridLayout* uilayout, const QString& n, int row);
    ~EffectRow();
    EffectField* add_field(int type, const QString &id, int colspan = 1);
    EffectField* field(int i);
    int fieldCount();
    void set_keyframe_now(ComboAction *ca);
    void delete_keyframe(KeyframeDelete *kd, int index);
    void delete_keyframe_at_time(KeyframeDelete* kd, long time);
    QLabel* label;
    Effect* parent_effect;
    bool savable;

    bool isKeyframing();
    void setKeyframing(bool);

    QVector<long> keyframe_times;
    QVector<int> keyframe_types;
private slots:
    void set_keyframe_enabled(bool);
    void goto_previous_key();
    void toggle_key();
    void goto_next_key();
private:
    bool keyframing;
    QGridLayout* ui;
    QString name;
    int ui_row;
    QVector<EffectField*> fields;

    KeyframeNavigator* keyframe_nav;

    bool just_made_unsafe_keyframe;
};

#endif // EFFECTROW_H
