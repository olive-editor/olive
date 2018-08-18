#ifndef SOLIDEFFECT_H
#define SOLIDEFFECT_H

#include "../effect.h"

class SolidEffect : public Effect {
	Q_OBJECT
public:
	SolidEffect(Clip* c);
	EffectField* solid_type;
	EffectField* solid_color_field;
	EffectField* opacity_field;
	void process_image(QImage &img);
private slots:
	void enable_color();
};

#endif // SOLIDEFFECT_H
