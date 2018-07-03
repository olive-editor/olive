#include "effectcontrols.h"
#include "ui_effectcontrols.h"

#include <QMenu>
#include <QDebug>
#include <QVBoxLayout>

#include "effects/effects.h"
#include "project/clip.h"
#include "project/effect.h"
#include "ui/collapsiblewidget.h"


EffectControls::EffectControls(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::EffectControls)
{
	ui->setupUi(this);
    init_effects();
    clear_effects(false);
}

EffectControls::~EffectControls()
{
	delete ui;
}

void EffectControls::menu_select(QAction* q) {
    for (int i=0;i<selected_clips.size();i++) {
        Clip* clip = selected_clips.at(i);
        if ((clip->track < 0 && video_menu) || (clip->track >= 0 && !video_menu)) {
            clip->effects.append(create_effect(q->data().toInt(), clip));
        }
    }
    reload_clips();
}

void EffectControls::show_menu(bool video) {
    video_menu = video;

    int lim;
    QVector<QString>* effect_names;
    if (video) {
        lim = VIDEO_EFFECT_COUNT;
        effect_names = &video_effect_names;
    } else {
        lim = AUDIO_EFFECT_COUNT;
        effect_names = &audio_effect_names;
    }

    QMenu effects_menu(this);

    for (int i=0;i<lim;i++) {
        QAction* action = new QAction(&effects_menu);
        action->setText(effect_names->at(i));
        action->setData(i);
        effects_menu.addAction(action);
    }

    connect(&effects_menu, SIGNAL(triggered(QAction*)), this, SLOT(menu_select(QAction*)));

    effects_menu.exec(QCursor::pos());
}

void EffectControls::clear_effects(bool clear_cache) {
    // clear existing clips
    QVBoxLayout* video_layout = static_cast<QVBoxLayout*>(ui->video_effect_area->layout());
    QVBoxLayout* audio_layout = static_cast<QVBoxLayout*>(ui->audio_effect_area->layout());
    QLayoutItem* item;
    while ((item = video_layout->takeAt(0))) {
        item->widget()->setParent(NULL);
        disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
    }
    while ((item = audio_layout->takeAt(0))) {
        item->widget()->setParent(NULL);
        disconnect(static_cast<CollapsibleWidget*>(item->widget()), SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
    }
    ui->vcontainer->setVisible(false);
    ui->acontainer->setVisible(false);
    if (clear_cache) selected_clips.clear();
}

void EffectControls::deselect_all_effects(QWidget* sender) {
    QVector<Effect*> delete_effects;
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);
        for (int j=0;j<c->effects.size();j++) {
            if (c->effects.at(j)->container != sender) {
                c->effects.at(j)->container->header_click(false, false);
            }
        }
    }
}

void EffectControls::load_effects() {
    // load in new clips
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);
        if (c->track < 0) {
            ui->vcontainer->setVisible(true);
        } else {
            ui->acontainer->setVisible(true);
        }
        for (int j=0;j<c->effects.size();j++) {
            CollapsibleWidget* container = c->effects.at(j)->container;
            if (c->track < 0) {
                static_cast<QVBoxLayout*>(ui->video_effect_area->layout())->addWidget(container);
                ui->vcontainer->setVisible(true);
            } else {
                static_cast<QVBoxLayout*>(ui->audio_effect_area->layout())->addWidget(container);
                ui->acontainer->setVisible(true);
            }
            connect(container, SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
        }
    }
}

void EffectControls::delete_clips() {
    // load in new clips
    QVector<Effect*> delete_effects;
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);
        for (int j=0;j<c->effects.size();j++) {
            Effect* effect = c->effects.at(j);
            if (effect->container->selected) {
                c->effects.removeAt(j);
                delete_effects.append(effect);
            }
        }
    }
    if (delete_effects.size() > 0) {
        reload_clips();
        for (int i=0;i<delete_effects.size();i++) {
            delete delete_effects.at(i);
        }
    }
}

void EffectControls::reload_clips() {
    clear_effects(false);
    load_effects();
}

void EffectControls::set_clips(QVector<Clip*>& clips) {
    clear_effects(true);

    // replace clip vector
    selected_clips = clips;

    load_effects();
}

void EffectControls::on_add_video_effect_button_clicked()
{
    show_menu(true);
}

void EffectControls::on_add_audio_effect_button_clicked()
{
    show_menu(false);
}

bool EffectControls::is_focused() {
    if (this->hasFocus()) return true;
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);
        if (c != NULL) {
            for (int j=0;j<c->effects.size();j++) {
                if (c->effects.at(j)->container->is_focused()) {
                    return true;
                }
            }
        } else {
            qDebug() << "[WARNING] Tried to check focus of a NULL clip";
        }
    }
    return false;
}
