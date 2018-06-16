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

	clip = NULL;
    set_clip(NULL);
}

void EffectControls::menu_select(QAction* q) {
    clip->effects.append(create_effect(q->data().toInt(), clip));
    set_clip(clip);
}

EffectControls::~EffectControls()
{
	delete ui;
}

void EffectControls::on_pushButton_clicked()
{
    int lim;
    QVector<QString>* effect_names;
    if (clip->track < 0) {
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

void EffectControls::set_clip(Clip* c) {
	// clear ui->scrollAreaWidgetContents
	if (clip != NULL) {
		for (int i=0;i<clip->effects.size();i++) {
            clip->effects.at(i)->container->setParent(NULL);
		}
	}

	ui->pushButton->setEnabled(c != NULL);
	if (c != NULL) {
		for (int i=0;i<c->effects.size();i++) {
            static_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout())->insertWidget(i, c->effects.at(i)->container);
		}
	}
	clip = c;
}
