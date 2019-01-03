#ifndef TRANSFORMEFFECT_H
#define TRANSFORMEFFECT_H

#include "project/effect.h"

class TransformEffect : public Effect {
	Q_OBJECT
public:
	TransformEffect(Clip* c, const EffectMeta* em);
	void refresh();
    void process_coords(double timecode, GLTextureCoords& coords, int data);

    void gizmo_draw(double timecode, GLTextureCoords& coords);
public slots:
	void toggle_uniform_scale(bool enabled);
private:
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

    EffectGizmo* top_left_gizmo;
    EffectGizmo* top_center_gizmo;
    EffectGizmo* top_right_gizmo;
    EffectGizmo* bottom_left_gizmo;
    EffectGizmo* bottom_center_gizmo;
    EffectGizmo* bottom_right_gizmo;
    EffectGizmo* left_center_gizmo;
    EffectGizmo* right_center_gizmo;
    EffectGizmo* anchor_gizmo;
    EffectGizmo* rotate_gizmo;
    EffectGizmo* rect_gizmo;

	int default_anchor_x;
    int default_anchor_y;
    double default_pos_x;
    double default_pos_y;
    bool set;
};

#endif // TRANSFORMEFFECT_H
