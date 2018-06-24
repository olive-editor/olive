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
    clear_effects();
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

void EffectControls::clear_effects() {
    // clear existing clips
    QVBoxLayout* video_layout = static_cast<QVBoxLayout*>(ui->video_effect_area->layout());
    QVBoxLayout* audio_layout = static_cast<QVBoxLayout*>(ui->audio_effect_area->layout());
    QLayoutItem* item;
    while ((item = video_layout->takeAt(0))) {item->widget()->setParent(NULL);}
    while ((item = audio_layout->takeAt(0))) {item->widget()->setParent(NULL);}
    ui->vcontainer->setVisible(false);
    ui->acontainer->setVisible(false);
}

void EffectControls::load_effects() {
    // load in new clips
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = selected_clips.at(i);
        for (int j=0;j<c->effects.size();j++) {
            CollapsibleWidget* container = c->effects.at(j)->container;
            if (c->track < 0) {
                static_cast<QVBoxLayout*>(ui->video_effect_area->layout())->addWidget(container);
                ui->vcontainer->setVisible(true);
            } else {
                static_cast<QVBoxLayout*>(ui->audio_effect_area->layout())->addWidget(container);
                ui->acontainer->setVisible(true);
            }
        }
    }
}

void EffectControls::reload_clips() {
    // clear_effects();
    load_effects();
}

void EffectControls::set_clips(QVector<Clip*>& clips) {
    clear_effects();

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
