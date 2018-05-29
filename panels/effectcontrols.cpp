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

EffectControls::~EffectControls()
{
	delete ui;
}

void EffectControls::on_pushButton_clicked()
{
	QMenu effects_menu(this);
	for (int i=0;i<video_effect_names.size();i++) {
		effects_menu.addAction(video_effect_names.at(i));
	}
	effects_menu.addSeparator();
	for (int i=0;i<audio_effect_names.size();i++) {
		effects_menu.addAction(audio_effect_names.at(i));
	}
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
