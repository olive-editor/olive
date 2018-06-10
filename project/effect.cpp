#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "effects/effects.h"

Effect::Effect(Clip* c) : parent_clip(c)
{
	name = "<unnamed effect>";
	type = EFFECT_TYPE_INVALID;
}

void Effect::setup_effect(int t, int i) {
    type = t;
    id = i;
    container = new CollapsibleWidget();
    if (type == EFFECT_TYPE_VIDEO) {
        container->setText(video_effect_names[i]);
    } else if (type == EFFECT_TYPE_AUDIO) {
        container->setText(audio_effect_names[i]);
    }
    ui = new QWidget();
}

void Effect::field_changed() {
	panel_viewer->viewer_widget->update();
}

Effect* Effect::copy() {return NULL;}
void Effect::save(QXmlStreamWriter *stream) {}

void Effect::process_gl(int*, int*) {}
void Effect::process_audio(uint8_t*, int) {}
