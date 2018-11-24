#ifndef TRANSFORMEFFECT_H
#define TRANSFORMEFFECT_H

#include "project/effect.h"

class TransformEffect : public Effect {
	Q_OBJECT
public:
	TransformEffect(Clip* c, const EffectMeta* em);
	void refresh();
    void process_coords(double timecode, GLTextureCoords& coords, int data);
    void process_gizmos();

	EffectField* position_x;
	EffectField* position_y;
	EffectField* scale_x;
	EffectField* scale_y;
	EffectField* uniform_scale_field;
	EffectField* rotation;
	EffectField* anchor_x_box;
	EffectField* anchor_y_box;
	EffectField* opacity;
	EffectField* blend_mode_box;
public slots:
	void toggle_uniform_scale(bool enabled);
private:
	int default_anchor_x;
	int default_anchor_y;
};

#endif // TRANSFORMEFFECT_H
