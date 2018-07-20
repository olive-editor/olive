#include "effect.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "effects/effects.h"
#include "panels/project.h"
#include "project/undo.h"

#include <QCheckBox>
#include <QGridLayout>

Effect::Effect(Clip* c, int t, int i) : parent_clip(c), type(t), id(i) {
    container = new CollapsibleWidget();
    if (type == EFFECT_TYPE_VIDEO) {
        container->setText(video_effect_names[i]);
    } else if (type == EFFECT_TYPE_AUDIO) {
        container->setText(audio_effect_names[i]);
    }
    connect(container->enabled_check, SIGNAL(clicked(bool)), this, SLOT(checkbox_command()));
    connect(container->enabled_check, SIGNAL(clicked(bool)), this, SLOT(field_changed()));
    ui = new QWidget();

    ui_layout = new QGridLayout();
    ui->setLayout(ui_layout);
    container->setContents(ui);
}

void Effect::field_changed() {
	panel_viewer->viewer_widget->update();
    project_changed = true;
}

void Effect::checkbox_command() {
    CheckboxCommand* c = new CheckboxCommand(static_cast<QCheckBox*>(sender()));
    undo_stack.push(c);
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
