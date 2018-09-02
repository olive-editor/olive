#ifndef SOLIDEFFECT_H
#define SOLIDEFFECT_H

#include "../effect.h"

class QOpenGLTexture;
#include <QImage>

class SolidEffect : public SuperimposeEffect {
	Q_OBJECT
public:
	SolidEffect(Clip* c);
	EffectField* solid_type;
	EffectField* solid_color_field;
	EffectField* opacity_field;
	void redraw(double timecode);
};

#endif // SOLIDEFFECT_H
