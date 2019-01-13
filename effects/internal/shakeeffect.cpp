#include "shakeeffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <QOpenGLFunctions>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "panels/timeline.h"

#include "debug.h"

ShakeEffect::ShakeEffect(Clip *c, const EffectMeta *em) : Effect(c, em) {
	enable_coords = true;

    EffectRow* intensity_row = add_row(tr("Intensity"));
	intensity_val = intensity_row->add_field(EFFECT_FIELD_DOUBLE, "intensity");
	intensity_val->set_double_minimum_value(0);

    EffectRow* rotation_row = add_row(tr("Rotation"));
	rotation_val = rotation_row->add_field(EFFECT_FIELD_DOUBLE, "rotation");
	rotation_val->set_double_minimum_value(0);

    EffectRow* frequency_row = add_row(tr("Frequency"));
	frequency_val = frequency_row->add_field(EFFECT_FIELD_DOUBLE, "frequency");
	frequency_val->set_double_minimum_value(0);

	// set defaults
	intensity_val->set_double_default_value(25);
	rotation_val->set_double_default_value(10);
	frequency_val->set_double_default_value(5);

	srand(QDateTime::currentMSecsSinceEpoch());

	for (int i=0;i<RANDOM_VAL_SIZE;i++) {
		random_vals[i] = (double)rand()/RAND_MAX;
	}
}

void ShakeEffect::process_coords(double timecode, GLTextureCoords& coords, int) {
	int lim = RANDOM_VAL_SIZE/6;

	double multiplier = intensity_val->get_double_value(timecode)/lim;
	double rotmult = rotation_val->get_double_value(timecode)/lim/10;
	double x = timecode * frequency_val->get_double_value(timecode);

	double xoff = 0;
	double yoff = 0;
	double rotoff = 0;

	for (int i=0;i<lim;i++) {
		int offset = 6*i;
		xoff += qSin((x+random_vals[offset])*random_vals[offset+1]);
		yoff += qSin((x+random_vals[offset+2])*random_vals[offset+3]);
		rotoff += qSin((x+random_vals[offset+4])*random_vals[offset+5]);
	}

	xoff *= multiplier;
	yoff *= multiplier;
	rotoff *= rotmult;

	coords.vertexTopLeftX += xoff;
	coords.vertexTopRightX += xoff;
	coords.vertexBottomLeftX += xoff;
	coords.vertexBottomRightX += xoff;

	coords.vertexTopLeftY += yoff;
	coords.vertexTopRightY += yoff;
	coords.vertexBottomLeftY += yoff;
	coords.vertexBottomRightY += yoff;

	glRotatef(rotoff, 0, 0, 1);
}
