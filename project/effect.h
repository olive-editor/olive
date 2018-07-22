#ifndef EFFECT_H
#define EFFECT_H

#include <QObject>
#include <QString>
class QWidget;
class CollapsibleWidget;
class QGridLayout;

struct Clip;
class QXmlStreamReader;
class QXmlStreamWriter;

enum EffectTypes { EFFECT_TYPE_INVALID, EFFECT_TYPE_VIDEO, EFFECT_TYPE_AUDIO };

class Effect : public QObject
{
	Q_OBJECT
public:
    Effect(Clip* c, int t, int i); // default constructor
    Clip* parent_clip;
	int type;
    int id;
	QString name;
	CollapsibleWidget* container;
    QGridLayout* ui_layout;
	QWidget* ui;

    bool is_enabled();

    virtual void init();

    virtual Effect* copy(Clip* c);

    virtual void load(QXmlStreamReader* stream);
    virtual void save(QXmlStreamWriter* stream);

	virtual void process_gl(int* anchor_x, int* anchor_y);
    virtual void post_gl();
    virtual void process_audio(quint8* samples, int nb_bytes);
public slots:
	void field_changed();
    void checkbox_command();
};

#endif // EFFECT_H
