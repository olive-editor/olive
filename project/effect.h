#ifndef EFFECT_H
#define EFFECT_H

#include <QObject>
#include <QString>
class QWidget;
class CollapsibleWidget;

struct Clip;

enum EffectTypes { EFFECT_TYPE_INVALID, EFFECT_TYPE_VIDEO, EFFECT_TYPE_AUDIO };

class Effect : public QObject
{
	Q_OBJECT
public:
    Effect(Clip* c); // default constructor
	int type;
	QString name;
	CollapsibleWidget* container;
	QWidget* ui;
    Clip* parent_clip;

    virtual Effect* copy();

	virtual void process_gl(int* anchor_x, int* anchor_y);
	virtual void process_audio(uint8_t* samples, int nb_bytes);

public slots:
	void field_changed();
};

#endif // EFFECT_H
