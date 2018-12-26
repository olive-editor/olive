#include "effectrow.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>

#include "project/undo.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "panels/viewer.h"
#include "effect.h"
#include "ui/viewerwidget.h"
#include "ui/keyframenavigator.h"

EffectRow::EffectRow(Effect *parent, bool save, QGridLayout *uilayout, const QString &n, int row) :
    parent_effect(parent),
    savable(save),
    keyframing(false),
    ui(uilayout),
    name(n),
    ui_row(row),
    just_made_unsafe_keyframe(false)
{
    label = new QLabel(name);
    ui->addWidget(label, row, 0);

    if (parent_effect->meta->type != EFFECT_TYPE_TRANSITION) {
        keyframe_nav = new KeyframeNavigator();
        connect(keyframe_nav, SIGNAL(goto_previous_key()), this, SLOT(goto_previous_key()));
        connect(keyframe_nav, SIGNAL(toggle_key()), this, SLOT(toggle_key()));
        connect(keyframe_nav, SIGNAL(goto_next_key()), this, SLOT(goto_next_key()));
        connect(keyframe_nav, SIGNAL(set_keyframe_enabled(bool)), this, SLOT(set_keyframe_enabled(bool)));
        ui->addWidget(keyframe_nav, row, 6);
    }
}

bool EffectRow::isKeyframing() {
    return keyframing;
}

void EffectRow::setKeyframing(bool b) {
    if (parent_effect->meta->type != EFFECT_TYPE_TRANSITION) {
        keyframing = b;
        keyframe_nav->enable_keyframes(b);
    }
}

void EffectRow::set_keyframe_enabled(bool enabled) {
    if (enabled) {
        ComboAction* ca = new ComboAction();
        set_keyframe_now(ca);
        undo_stack.push(ca);
    } else {
        if (QMessageBox::question(panel_effect_controls, "Disable Keyframes", "Disabling keyframes will delete all current keyframes. Are you sure you want to do this?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            // clear
            KeyframeDelete* kd = new KeyframeDelete();
            for (int i=keyframe_times.size()-1;i>=0;i--) {
                delete_keyframe(kd, i);
            }
            kd->disable_keyframes_on_row = this;
            undo_stack.push(kd);
            panel_effect_controls->update_keyframes();
        } else {
            setKeyframing(true);
        }
    }
}

void EffectRow::goto_previous_key() {
    long key = LONG_MIN;
    Clip* c = parent_effect->parent_clip;
    for (int i=0;i<keyframe_times.size();i++) {
        long comp = keyframe_times.at(i) - c->clip_in + c->timeline_in;
        if (comp < sequence->playhead) {
            key = qMax(comp, key);
        }
    }
    if (key != LONG_MIN) panel_sequence_viewer->seek(key);
}

void EffectRow::toggle_key() {
    int index = -1;
    Clip* c = parent_effect->parent_clip;
    for (int i=0;i<keyframe_times.size();i++) {
        long comp = c->timeline_in - c->clip_in + keyframe_times.at(i);
        if (comp == sequence->playhead) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        // keyframe doesn't exist, set one
        ComboAction* ca = new ComboAction();
        set_keyframe_now(ca);
        undo_stack.push(ca);
    } else {
        KeyframeDelete* kd = new KeyframeDelete();
        delete_keyframe(kd, index);
        undo_stack.push(kd);
        panel_effect_controls->update_keyframes();
        panel_sequence_viewer->viewer_widget->update();
    }
}

void EffectRow::goto_next_key() {
    long key = LONG_MAX;
    Clip* c = parent_effect->parent_clip;
    for (int i=0;i<keyframe_times.size();i++) {
        long comp = c->timeline_in - c->clip_in + keyframe_times.at(i);
        if (comp > sequence->playhead) {
            key = qMin(comp, key);
        }
    }
    if (key != LONG_MAX) panel_sequence_viewer->seek(key);
}

EffectField* EffectRow::add_field(int type, const QString& id, int colspan) {
    EffectField* field = new EffectField(this, type, id);
    fields.append(field);
    QWidget* element = field->get_ui_element();
    ui->addWidget(element, ui_row, fields.size(), 1, colspan);
    connect(field, SIGNAL(changed()), parent_effect, SLOT(field_changed()));
    return field;
}

EffectRow::~EffectRow() {
    for (int i=0;i<fields.size();i++) {
        delete fields.at(i);
    }
}

void EffectRow::set_keyframe_now(ComboAction* ca) {
    int index = -1;
    long time = sequence->playhead-parent_effect->parent_clip->timeline_in+parent_effect->parent_clip->clip_in;
    for (int i=0;i<keyframe_times.size();i++) {
        if (keyframe_times.at(i) == time) {
            index = i;
            break;
        }
    }

    KeyframeSet* ks = new KeyframeSet(this, index, time, just_made_unsafe_keyframe);

    if (ca != NULL) {
        just_made_unsafe_keyframe = false;
        ca->append(ks);
    } else {
        if (index == -1) just_made_unsafe_keyframe = true;
        ks->redo();
        delete ks;
    }

    panel_effect_controls->update_keyframes();
}

void EffectRow::delete_keyframe_at_time(KeyframeDelete* kd, long time) {
    for (int i=0;i<keyframe_times.size();i++) {
        if (keyframe_times.at(i) == time) {
            delete_keyframe(kd, i);
            break;
        }
    }
}

void EffectRow::delete_keyframe(KeyframeDelete* kd, int index) {
    kd->rows.append(this);
    kd->keyframes.append(index);
}

EffectField* EffectRow::field(int i) {
    return fields.at(i);
}

int EffectRow::fieldCount() {
    return fields.size();
}
