#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "effects/effects.h"

#include <QCheckBox>

Effect::Effect(Clip* c) : parent_clip(c) {
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

bool Effect::is_enabled() {
    return container->enabled_check->isChecked();
}

Effect* Effect::copy(Clip*) {return NULL;}
void Effect::load(QXmlStreamReader*) {}
void Effect::save(QXmlStreamWriter*) {}
void Effect::process_gl(int*, int*) {}
void Effect::post_gl() {}
void Effect::process_audio(uint8_t*, int) {}
