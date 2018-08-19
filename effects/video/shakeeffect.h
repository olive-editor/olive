#ifndef SHAKEEFFECT_H
#define SHAKEEFFECT_H

#include "../effect.h"

class ShakeEffect : public Effect {
	Q_OBJECT
public:
	ShakeEffect(Clip* c);
    void process_gl(long p, QOpenGLShaderProgram& shader_prog, int* anchor_x, int* anchor_y);

	EffectField* intensity_val;
	EffectField* rotation_val;
	EffectField* frequency_val;
public slots:
	void refresh();
private:
	int shake_progress;
	int shake_limit;
	int next_x;
	int next_y;
	int next_rot;
	int offset_x;
	int offset_y;
	int offset_rot;
	int prev_x;
	int prev_y;
	int prev_rot;
	int perp_x;
	int perp_y;
	double t;
	bool inside;
};

#endif // SHAKEEFFECT_H
