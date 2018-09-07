#include "effectcontrols.h"
#include "ui_effectcontrols.h"

#include <QMenu>
#include <QDebug>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>

#include "panels/panels.h"
#include "effects/effect.h"
#include "effects/transition.h"
#include "project/clip.h"
#include "effects/effect.h"
#include "ui/collapsiblewidget.h"
#include "project/sequence.h"
#include "project/undo.h"
#include "panels/project.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"

EffectControls::EffectControls(QWidget *parent) :
	QDockWidget(parent),
    zoom(1),
    ui(new Ui::EffectControls)
{
	ui->setupUi(this);
    init_effects();
	init_transitions();
    clear_effects(false);
	ui->headers->snapping = false;

	ui->effects_area->parent_widget = ui->scrollArea;
	ui->effects_area->keyframe_area = ui->keyframeView;
	ui->effects_area->header = ui->headers;

	ui->keyframeView->header = ui->headers;

	connect(ui->keyframeScroller->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->scrollArea->verticalScrollBar(), SLOT(setValue(int)));
	connect(ui->keyframeScroller->horizontalScrollBar(), SIGNAL(valueChanged(int)), ui->keyframeHeaderScroller->horizontalScrollBar(), SLOT(setValue(int)));
}

EffectControls::~EffectControls() {
	delete ui;
}

bool EffectControls::keyframe_focus() {
	return ui->headers->hasFocus() || ui->keyframeView->hasFocus();
}

void EffectControls::set_zoom(bool in) {
	if (in) {
		zoom *= 2;
	} else {
		zoom *= 0.5;
	}
	update_keyframes();
}

void EffectControls::menu_select(QAction* q) {
    TimelineAction* ta = new TimelineAction();
    for (int i=0;i<selected_clips.size();i++) {
		Clip* c = sequence->get_clip(selected_clips.at(i));
        if ((c->track < 0) == video_menu) {
			if (transition_menu) {
				if (c->opening_transition == NULL) {
					ta->add_transition(sequence, selected_clips.at(i), q->data().toInt(), TA_OPENING_TRANSITION);
				}
				if (c->closing_transition == NULL) {
					ta->add_transition(sequence, selected_clips.at(i), q->data().toInt(), TA_CLOSING_TRANSITION);
				}
			} else {
				ta->add_effect(sequence, selected_clips.at(i), q->data().toInt());
			}
        }
    }
    undo_stack.push(ta);
	if (transition_menu) {
		panel_timeline->redraw_all_clips(true);
	} else {
		reload_clips();
		panel_viewer->viewer_widget->update();
	}
}

void EffectControls::update_keyframes() {
	if (ui->headers->isVisible()) ui->headers->update_header(zoom);
	ui->keyframeView->update();
}

void EffectControls::delete_selected_keyframes() {
	ui->keyframeView->delete_selected_keyframes();
}

void EffectControls::show_effect_menu(bool video, bool transitions) {
    video_menu = video;
	transition_menu = transitions;

    int lim;
    QVector<QString>* effect_names;
	if (transitions) {
		if (video) {
			lim = VIDEO_TRANSITION_COUNT;
			effect_names = &video_transition_names;
		} else {
			lim = AUDIO_TRANSITION_COUNT;
			effect_names = &audio_transition_names;
		}
	} else {
		if (video) {
			lim = VIDEO_EFFECT_COUNT;
			effect_names = &video_effect_names;
		} else {
			lim = AUDIO_EFFECT_COUNT;
			effect_names = &audio_effect_names;
		}
	}

    QMenu effects_menu(this);

    for (int i=0;i<lim;i++) {
        QAction* action = new QAction(&effects_menu);
        action->setText(effect_names->at(i));
        action->setData(i);

        // sort alphabetically
        bool added = false;
        for (int j=0;j<effects_menu.actions().size();j++) {
            QAction* comp_action = effects_menu.actions().at(j);
            if (comp_action->text() > effect_names->at(i)) {
                effects_menu.insertAction(comp_action, action);
                added = true;
                break;
            }
        }
        if (!added) effects_menu.addAction(action);
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
	ui->headers->setVisible(false);
	ui->keyframeView->setEnabled(false);
    if (clear_cache) selected_clips.clear();
}

void EffectControls::deselect_all_effects(QWidget* sender) {
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->get_clip(selected_clips.at(i));
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
		Clip* c = sequence->get_clip(selected_clips.at(i));
		QVBoxLayout* layout;
		if (c->track < 0) {
			ui->vcontainer->setVisible(true);
			layout = static_cast<QVBoxLayout*>(ui->video_effect_area->layout());
		} else {
			ui->acontainer->setVisible(true);
			layout = static_cast<QVBoxLayout*>(ui->audio_effect_area->layout());
		}
        for (int j=0;j<c->effects.size();j++) {
			Effect* e = c->effects.at(j);
			CollapsibleWidget* container = e->container;
			layout->addWidget(container);
            connect(container, SIGNAL(deselect_others(QWidget*)), this, SLOT(deselect_all_effects(QWidget*)));
		}
    }
	if (selected_clips.size() > 0) {
		ui->keyframeView->setMinimumHeight(ui->effects_area->height());
		ui->keyframeView->setEnabled(true);
		ui->headers->setVisible(true);
		ui->keyframeView->update();
	}
}

void EffectControls::delete_effects() {
    // load in new clips
    EffectDeleteCommand* command = new EffectDeleteCommand();
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->get_clip(selected_clips.at(i));
        for (int j=0;j<c->effects.size();j++) {
            Effect* effect = c->effects.at(j);
            if (effect->container->selected) {
                command->clips.append(c);
                command->fx.append(j);
            }
        }
    }
    if (command->clips.size() > 0) {
        undo_stack.push(command);
		panel_viewer->viewer_widget->update();
    } else {
        delete command;
    }
}

void EffectControls::reload_clips() {
    clear_effects(false);
    load_effects();
}

void EffectControls::set_clips(QVector<int>& clips) {
    clear_effects(true);

    // replace clip vector
    selected_clips = clips;

    load_effects();
}

void EffectControls::on_add_video_effect_button_clicked()
{
	show_effect_menu(true, false);
}

void EffectControls::on_add_audio_effect_button_clicked()
{
	show_effect_menu(false, false);
}

void EffectControls::on_add_video_transition_button_clicked()
{
	show_effect_menu(true, true);
}

void EffectControls::on_add_audio_transition_button_clicked()
{
	show_effect_menu(false, true);
}

bool EffectControls::is_focused() {
    if (this->hasFocus()) return true;
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->get_clip(selected_clips.at(i));
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

EffectsArea::EffectsArea(QWidget* parent) : QWidget(parent) {}

void EffectsArea::resizeEvent(QResizeEvent*) {
	parent_widget->setMinimumWidth(sizeHint().width());
	keyframe_area->setMinimumHeight(height() - header->height());
}
