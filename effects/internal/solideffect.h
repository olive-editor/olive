#ifndef SOLIDEFFECT_H
#define SOLIDEFFECT_H

#include "project/effect.h"

class QOpenGLTexture;
#include <QImage>

class SolidEffect : public Effect {
	Q_OBJECT
public:
	SolidEffect(Clip* c, const EffectMeta *em);	
	void redraw(double timecode);
private slots:
    void ui_update(int);
private:
    EffectField* solid_type;
    EffectField* solid_color_field;
    EffectField* opacity_field;
    EffectField* checkerboard_size_field;
};

#endif // SOLIDEFFECT_H
