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

    effects_menu = new QMenu(this);
    for (int i=0;i<VIDEO_EFFECT_COUNT;i++) {
        QAction* action = new QAction();
        action->setText(video_effect_names.at(i));
        action->setData(i);
        effects_menu->addAction(action);
    }
    effects_menu->addSeparator();
    for (int i=0;i<AUDIO_EFFECT_COUNT;i++) {
        QAction* action = new QAction();
        action->setText(audio_effect_names.at(i));
        action->setData(i);
        effects_menu->addAction(action);
    }
    connect(effects_menu, SIGNAL(triggered(QAction*)), this, SLOT(menu_select(QAction*)));
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
    effects_menu->exec(QCursor::pos());
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
