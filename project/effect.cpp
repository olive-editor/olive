#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"

Effect::Effect(Clip* c) : parent_clip(c)
{
	name = "<unnamed effect>";
	type = EFFECT_TYPE_INVALID;
}

void Effect::field_changed() {
	panel_viewer->viewer_widget->update();
}

Effect* Effect::copy() {
    Effect* e = new Effect(parent_clip);
    name = e->name;
    type = e->type;
    return e;
}

void Effect::process_gl(int*, int*) {}
void Effect::process_audio(uint8_t*, int) {}
