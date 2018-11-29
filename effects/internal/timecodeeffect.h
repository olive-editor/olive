#ifndef TIMECODEEFFECT_H
#define TIMECODEEFFECT_H

#include "project/effect.h"

#include <QFont>
#include <QImage>
class QOpenGLTexture;

class TimecodeEffect : public Effect {
	Q_OBJECT
public:
    TimecodeEffect(Clip* c, const EffectMeta *em);
    void redraw(double timecode);
    EffectField * scale_val;
    EffectField * color_val;
    EffectField * color_bg_val;
    EffectField * bg_alpha;
    EffectField * offset_x_val;
    EffectField * offset_y_val;
    EffectField * prepend_text;
    EffectField * tc_select;

private:
    QFont font;
    QString display_timecode;
    Clip * c;
};

#endif // TIMECODEEFFECT_H
