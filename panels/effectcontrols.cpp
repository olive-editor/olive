#include "effectcontrols.h"
#include "ui_effectcontrols.h"

#include <QMenu>
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
#include "debug.h"

EffectControls::EffectControls(QWidget *parent) :
	QDockWidget(parent),
	multiple(false),
    zoom(1),
    ui(new Ui::EffectControls)
{
	ui->setupUi(this);
    init_effects();
	init_transitions();
    clear_effects(false);
	ui->headers->viewer = panel_sequence_viewer;
	ui->headers->snapping = false;

	ui->effects_area->parent_widget = ui->scrollArea;
	ui->effects_area->keyframe_area = ui->keyframeView;
	ui->effects_area->header = ui->headers;

	ui->keyframeView->header = ui->headers;

	ui->label_2->setVisible(false);

	connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), ui->headers, SLOT(set_scroll(int)));
	connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), ui->keyframeView, SLOT(set_x_scroll(int)));
	connect(ui->verticalScrollBar, SIGNAL(valueChanged(int)), ui->keyframeView, SLOT(set_y_scroll(int)));
	connect(ui->verticalScrollBar, SIGNAL(valueChanged(int)), ui->scrollArea->verticalScrollBar(), SLOT(setValue(int)));
	connect(ui->scrollArea->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->verticalScrollBar, SLOT(setValue(int)));
}

EffectControls::~EffectControls() {
	delete ui;
}

bool EffectControls::keyframe_focus() {
	return ui->headers->hasFocus() || ui->keyframeView->hasFocus();
}

void EffectControls::set_zoom(bool in) {
	zoom *= (in) ? 2 : 0.5;
	update_keyframes();
}

void EffectControls::menu_select(QAction* q) {
    ComboAction* ca = new ComboAction();
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->clips.at(selected_clips.at(i));
        if ((c->track < 0) == video_menu) {
			if (transition_menu) {
				if (c->opening_transition == NULL) {
                    ca->append(new AddTransitionCommand(c, q->data().toInt(), TA_OPENING_TRANSITION));
				}
				if (c->closing_transition == NULL) {
                    ca->append(new AddTransitionCommand(c, q->data().toInt(), TA_CLOSING_TRANSITION));
				}
			} else {
                ca->append(new AddEffectCommand(c, reinterpret_cast<EffectMeta*>(q->data().value<quintptr>())));
			}
        }
    }
    undo_stack.push(ca);
	if (transition_menu) {
		update_ui(true);
	} else {
		reload_clips();
		panel_sequence_viewer->viewer_widget->update();
    }
}

void EffectControls::update_keyframes() {
	ui->headers->update_zoom(zoom);
	ui->keyframeView->update();
}

void EffectControls::delete_selected_keyframes() {
	ui->keyframeView->delete_selected_keyframes();
}

void EffectControls::show_effect_menu(bool video, bool transitions) {
    /*video_menu = video;
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

    effects_menu.exec(QCursor::pos());*/
    video_menu = video;
    transition_menu = transitions;

    QVector<EffectMeta>& effect_list = (video) ? video_effects : audio_effects;
    QMenu effects_menu(this);
    for (int i=0;i<effect_list.size();i++) {
        const EffectMeta& em = effect_list.at(i);
        QAction* action = new QAction(&effects_menu);
        action->setText(em.name);
        action->setData(reinterpret_cast<quintptr>(&em));

        // TODO alphabetical ordering
        QMenu* parent = &effects_menu;
        if (!em.category.isEmpty()) {
            bool found = false;
            for (int j=0;j<effects_menu.actions().size();j++) {
                QAction* action = effects_menu.actions().at(j);
                if (action->menu() != NULL) {
                    if (action->menu()->title() == em.category) {
                        parent = action->menu();
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                parent = new QMenu(&effects_menu);
                parent->setTitle(em.category);
                effects_menu.addMenu(parent);
            }
        }

        parent->addAction(action);
    }

    QMenu test_menu(this);
    test_menu.setTitle("HEY NOW");
    test_menu.addAction("YOU'RE AN ALL STAR");
    effects_menu.addMenu(&test_menu);

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
    ui->label_2->setVisible(false);
    ui->vcontainer->setVisible(false);
    ui->acontainer->setVisible(false);
	ui->headers->setVisible(false);
	ui->keyframeView->setEnabled(false);
    if (clear_cache) selected_clips.clear();
}

void EffectControls::deselect_all_effects(QWidget* sender) {
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->clips.at(selected_clips.at(i));
        for (int j=0;j<c->effects.size();j++) {
            if (c->effects.at(j)->container != sender) {
                c->effects.at(j)->container->header_click(false, false);
            }
        }
    }
}

void EffectControls::load_effects() {
	ui->label_2->setVisible(multiple);

	if (!multiple) {
		// load in new clips
		for (int i=0;i<selected_clips.size();i++) {
            Clip* c = sequence->clips.at(selected_clips.at(i));
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
			ui->verticalScrollBar->setMaximum(qMax(0, ui->effects_area->height() - ui->keyframeView->height() - ui->headers->height()));
			ui->keyframeView->setEnabled(true);
			ui->headers->setVisible(true);
			ui->keyframeView->update();
		}
	}
}

void EffectControls::delete_effects() {
    // load in new clips
    EffectDeleteCommand* command = new EffectDeleteCommand();
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->clips.at(selected_clips.at(i));
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
		panel_sequence_viewer->viewer_widget->update();
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

void EffectControls::resizeEvent(QResizeEvent*) {
	ui->verticalScrollBar->setMaximum(qMax(0, ui->effects_area->height() - ui->keyframeView->height() - ui->headers->height()));
}

bool EffectControls::is_focused() {
    if (this->hasFocus()) return true;
    for (int i=0;i<selected_clips.size();i++) {
        Clip* c = sequence->clips.at(selected_clips.at(i));
        if (c != NULL) {
            for (int j=0;j<c->effects.size();j++) {
                if (c->effects.at(j)->container->is_focused()) {
                    return true;
                }
            }
        } else {
			dout << "[WARNING] Tried to check focus of a NULL clip";
        }
    }
    return false;
}

EffectsArea::EffectsArea(QWidget* parent) : QWidget(parent) {}

void EffectsArea::resizeEvent(QResizeEvent*) {
	parent_widget->setMinimumWidth(sizeHint().width());
}
